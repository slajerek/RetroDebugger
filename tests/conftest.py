"""Shared pytest fixtures for the RetroDebugger WebSocket regression suite.

Assumes a RetroDebugger instance is running with the WebSocket server enabled
on ws://127.0.0.1:3563/stream. See plan's Phase 0.5 prerequisites.
"""

import sys
import time
from pathlib import Path

import pytest

# Make `tests/retrodebugger.py` importable from `tests/websocket/test_*.py`.
sys.path.insert(0, str(Path(__file__).parent))
from retrodebugger import RetroDebugger  # noqa: E402

FIXTURE_DIR = Path(__file__).parent / "fixtures"


@pytest.fixture
def fixture_dir():
    """Return the path to the tests/fixtures directory."""
    return FIXTURE_DIR


@pytest.fixture(scope="session")
def rd():
    """One WebSocket connection shared across the session."""
    client = RetroDebugger()
    yield client
    client.close()


@pytest.fixture(autouse=True)
def _warp_off(rd):
    """Guarantee warp is off at the start of every test (no leak from a prior test)."""
    rd.call(f"{rd.platform}/warp/set", {"warp": False})
    yield


@pytest.fixture
def fresh_cpu(rd):
    """Hard-reset before each test; settle KERNAL."""
    rd.reset(hard=True)
    time.sleep(0.5)
    return rd


@pytest.fixture
def loaded_fixture(fresh_cpu):
    """Reset, load known_state.prg, run to the park loop ($0837), leave paused there.

    Protocol: CPU is running KERNAL after fresh_cpu. We load the PRG (running),
    wait 0.2s for KERNAL to settle, then makejmp(0810) to redirect execution to
    the fixture entry point while the CPU is running. After cont() + 0.15s the
    fixture code (screen clear ~6ms, sentinel writes ~10µs, JMP park) has long
    finished. We then pause and assert PC is in the park loop.

    NOTE: The polling-loop approach (pause/check/cont) is NOT used here because
    repeated pause/cont fragmentation prevents KERNAL's IRQ RTI from completing
    and returning to user code. A single uninterrupted 0.15s run is reliable.
    """
    rd = fresh_cpu
    resp, _ = rd.call("load", {"path": str(FIXTURE_DIR / "known_state.prg")})
    assert resp.get("status") == 200, f"fixture load failed: {resp}"
    time.sleep(0.5)  # let KERNAL settle post-load while CPU is running
    rd.call(f"{rd.platform}/cpu/makejmp", {"address": 0x0810})
    rd.cont()
    time.sleep(0.15)  # fixture runs (screen clear ~6ms, park ~instant); IRQ can complete
    rd.pause()
    PARK = 0x0837
    pc = rd.cpu_status()["result"]["pc"]
    assert PARK <= pc <= PARK + 2, f"fixture did not reach park: pc={pc:#06x}"
    return rd
