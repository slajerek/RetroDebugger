"""retrodebugger.py — WebSocket client for slajerek's RetroDebugger.

The debugger exposes a WebSocket endpoint at ws://localhost:3563/stream
(default — enable via Library/RetroDebugger/settings.hjson:
  RunDebuggerServerWebSockets: true).

Wire format:
  Request:  JSON {"fn": "...", "params": {...}, "token": "..."} text frame.
            For requests carrying binary data (e.g. cpu/memory/writeBlock):
            JSON + null byte + binary bytes, sent as a single binary frame.
  Response: JSON text frame, OR JSON + null byte + binary data (for reads).

Endpoint paths discovered from slajerek/RetroDebugger source
(src/Remote/CDebuggerServerApi.cpp + src/Emulators/vice/.../CDebuggerServerApiVice.cpp):

  Generic (platform = "c64"):
    c64/reset/{hard,soft}
    c64/detachEverything
    c64/warp/set
    c64/pause              c64/continue
    c64/step/{cycle,instruction,subroutine}
    c64/cpu/status                       — JSON: pc, a, x, y, sp, p, ...
    c64/cpu/makejmp                      — params: address
    c64/cpu/counters/read                — JSON: cycle, instruction, frame
    c64/cpu/memory/{readBlock,writeBlock}
    c64/ram/{clear,readBlock,writeBlock}
    c64/input/{key,joystick}/{down,up}
    c64/cpu/breakpoint/{add,remove}      — params: addr (+ optional comparison/value/access)
    c64/cpu/memory/breakpoint/{add,remove}
    c64/vic/{read,write,breakpoint/{add,remove}}
    c64/cia/{read,write}
    c64/sid/{read,write}
    c64/segment/{read,write}             — params: segment (e.g. "Code")
    c64/savePrg

  Drive 1541 specific:
    c64/drive1541/cpu/memory/{readBlock,writeBlock}  — drive RAM + VIA I/O
    c64/drive1541/ram/{clear,readBlock,writeBlock}   — pure drive RAM
    c64/drive1541/via/{read,write}                   — params: drive=1 (or 2)

Top-level (no platform prefix):
    load                                 — params: path

NOTE: drive CPU PC isn't exposed in the JSON API. To watch drive 6502
state, either step C64 (which advances both CPUs in lockstep) and read
drive RAM/VIA between steps, or read drive RAM at known drive-code
addresses + disassemble to find loops.
"""

import asyncio
import json
import struct
import uuid
from typing import Optional

import websockets


class RetroDebugger:
    """Sync wrapper around an async websocket session."""

    def __init__(self, host: str = "127.0.0.1", port: int = 3563, platform: str = "c64"):
        self.host = host
        self.port = port
        self.platform = platform
        self._loop = asyncio.new_event_loop()
        self._ws = self._loop.run_until_complete(self._connect())

    async def _connect(self):
        return await websockets.connect(
            f"ws://{self.host}:{self.port}/stream",
            max_size=16 * 1024 * 1024,
        )

    def close(self):
        self._loop.run_until_complete(self._ws.close())
        self._loop.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def call(self, fn: str, params: Optional[dict] = None, binary: Optional[bytes] = None) -> tuple[dict, bytes]:
        """Send a call, return (json_response, binary_payload)."""
        return self._loop.run_until_complete(self._call(fn, params, binary))

    async def _call(self, fn, params, binary):
        token = uuid.uuid4().hex
        msg = {"fn": fn, "token": token}
        if params is not None:
            msg["params"] = params
        text = json.dumps(msg)
        if binary is not None:
            payload = text.encode() + b"\x00" + binary
            await self._ws.send(payload)
        else:
            await self._ws.send(text)
        # Read frames until we find the one whose token matches our request.
        # Async event frames (no/!=token) are skipped. Bounded to avoid hangs.
        for _ in range(64):
            resp = await asyncio.wait_for(self._ws.recv(), timeout=5.0)
            if isinstance(resp, bytes):
                nul = resp.find(b"\x00")
                if nul >= 0:
                    j, payload_bytes = json.loads(resp[:nul].decode()), resp[nul + 1:]
                else:
                    j, payload_bytes = json.loads(resp.decode()), b""
            else:
                j, payload_bytes = json.loads(resp), b""
            if j.get("token") == token:
                return j, payload_bytes
        raise RuntimeError(f"No response with matching token for fn={fn!r}")

    # ── Convenience wrappers ──────────────────────────────────────────────

    def pause(self):
        return self.call(f"{self.platform}/pause")[0]

    def cont(self):
        return self.call(f"{self.platform}/continue")[0]

    def step_instruction(self):
        return self.call(f"{self.platform}/step/instruction")[0]

    def cpu_status(self) -> dict:
        """C64 CPU state: pc, a, x, y, sp, p, etc."""
        return self.call(f"{self.platform}/cpu/status")[0]

    def cpu_counters(self) -> dict:
        return self.call(f"{self.platform}/cpu/counters/read")[0]

    def read_memory(self, address: int, size: int, with_io: bool = True) -> bytes:
        """Read C64 memory. with_io=True via CPU (sees I/O); False from RAM."""
        path = "cpu/memory/readBlock" if with_io else "ram/readBlock"
        resp, payload = self.call(f"{self.platform}/{path}",
                                  {"address": address, "size": size})
        return payload

    def write_memory(self, address: int, data: bytes, with_io: bool = True):
        path = "cpu/memory/writeBlock" if with_io else "ram/writeBlock"
        resp, _ = self.call(f"{self.platform}/{path}",
                            {"address": address}, binary=data)
        return resp

    def read_drive_memory(self, address: int, size: int, with_io: bool = True) -> bytes:
        """Read 1541 drive memory. with_io=True covers VIA I/O ($1800/$1C00)."""
        path = "drive1541/cpu/memory/readBlock" if with_io else "drive1541/ram/readBlock"
        resp, payload = self.call(f"{self.platform}/{path}",
                                  {"address": address, "size": size})
        return payload

    def drive_cpu_status(self) -> dict:
        """Drive 1541 CPU state: pc, a, x, y, sp, p, lastValidPC, headTrackPosition.
        Requires our patched RetroDebugger build (adds c64/drive1541/cpu/status)."""
        return self.call(f"{self.platform}/drive1541/cpu/status")[0]

    def attach_disk(self, path: str):
        """Attach a .d64 to drive 8.
        Requires our patched RetroDebugger build (adds c64/drive1541/disk/attach)."""
        return self.call(f"{self.platform}/drive1541/disk/attach", {"path": path})[0]

    def detach_disk(self):
        """Detach disk from drive 8."""
        return self.call(f"{self.platform}/drive1541/disk/detach")[0]

    def write_drive_memory(self, address: int, data: bytes, with_io: bool = True):
        path = "drive1541/cpu/memory/writeBlock" if with_io else "drive1541/ram/writeBlock"
        resp, _ = self.call(f"{self.platform}/{path}",
                            {"address": address}, binary=data)
        return resp

    def add_breakpoint(self, addr: int) -> int:
        resp, _ = self.call(f"{self.platform}/cpu/breakpoint/add", {"addr": addr})
        return resp.get("breakpointId", -1)

    def remove_breakpoint(self, addr: int):
        return self.call(f"{self.platform}/cpu/breakpoint/remove", {"addr": addr})[0]

    def add_memory_breakpoint(self, addr: int, access: str = "write",
                              comparison: Optional[str] = None,
                              value: Optional[int] = None) -> int:
        """Memory access breakpoint. access in {read,write}; optional value match."""
        params = {"addr": addr, "access": access}
        if comparison is not None:
            params["comparison"] = comparison
        if value is not None:
            params["value"] = value
        resp, _ = self.call(f"{self.platform}/cpu/memory/breakpoint/add", params)
        return resp.get("breakpointId", -1)

    def reset(self, hard: bool = True):
        op = "hard" if hard else "soft"
        return self.call(f"{self.platform}/reset/{op}")[0]
