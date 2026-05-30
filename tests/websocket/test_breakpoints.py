"""Breakpoint endpoint tests: CPU PC breakpoints, memory access breakpoints.

Note: _drain_events() was removed in the Phase 0.5 hardening pass. The client's
_call() now uses token-correlated request/response matching (PART A), so async
event frames are automatically skipped and never pollute response slots.
"""

import time

PARK_ADDR = 0x0837  # `park` label in tests/fixtures/known_state.prg


def test_cpu_breakpoint_fires_on_pc_match(fresh_cpu):
    """Set a breakpoint at a KERNAL address; reset; verify CPU pauses near it."""
    rd = fresh_cpu
    # $FCE2 = KERNAL reset vector. CPU passes through here right after reset.
    # Pause first so we can install bp cleanly.
    rd.pause()
    # Clear any stale breakpoints left by previous test runs (session-scoped rd).
    rd.call(f"{rd.platform}/cpu/breakpoint/remove", {"addr": 0xFCE2})
    rd.call(f"{rd.platform}/cpu/memory/breakpoint/remove", {"addr": 0xC000})
    rd.add_breakpoint(0xFCE2)

    try:
        # Resume execution so the reset actually starts the CPU running
        # (reset from a paused state keeps the CPU paused in 3.1).
        rd.cont()
        # Reset triggers PC to land on $FCE2; with bp set, CPU should pause there.
        rd.reset(hard=True)
        time.sleep(0.3)

        # Token-correlated _call() skips async event frames automatically.
        pc = rd.cpu_status()["result"]["pc"]
        # PC might be exactly $FCE2 or a few instructions past if bp fires post-fetch.
        assert 0xFCE2 <= pc <= 0xFCF0, \
            f"After reset with bp at $FCE2, PC = {pc:#06x}, expected close to $FCE2"
    finally:
        # Always remove — even on assertion failure — to avoid leaking into next test.
        rd.call(f"{rd.platform}/cpu/breakpoint/remove", {"addr": 0xFCE2})


def test_memory_write_breakpoint_fires_on_store(loaded_fixture):
    """Set a write-bp at $C000. Re-run fixture. CPU must pause at the STA that hits $C000."""
    rd = loaded_fixture
    # CPU is paused inside park loop. Add write-bp at $C000.
    # 3.1 requires comparison + value params even for "any write" — use '>= 0'.
    rd.add_memory_breakpoint(0xC000, access="write", comparison=">=", value=0)

    try:
        # Re-trigger the write by re-jumping to fixture start.
        # makejmp is queued in 3.1, so we need to step before it takes effect.
        rd.call(f"{rd.platform}/cpu/makejmp", {"address": 0x0810})
        rd.step_instruction()  # consume the queued jmp
        rd.cont()
        time.sleep(0.1)

        # Token-correlated _call() skips async event frames automatically —
        # no drain needed. cpu_status() gets its own response directly.
        pc = rd.cpu_status()["result"]["pc"]
        # The STA $C000 instruction lives between $0820 and $0830 (after screen clear).
        # Just check we paused before reaching park ($0837).
        assert pc < PARK_ADDR, \
            f"Write bp at $C000 did not fire — PC ran to {pc:#06x}, past park at {PARK_ADDR:#06x}"
    finally:
        # Always remove — even on assertion failure — to avoid leaking into next test.
        rd.call(f"{rd.platform}/cpu/memory/breakpoint/remove", {"addr": 0xC000})


def test_breakpoint_remove_does_not_error(fresh_cpu):
    """Removing a non-existent breakpoint should not error.

    In RD 3.1, removing a non-existent breakpoint returns 406 (Not Acceptable).
    Accept any reasonable 2xx/4xx status — the connection must survive.
    """
    response = fresh_cpu.remove_breakpoint(0xDEAD)
    # RD 3.1 returns 406 for non-existent breakpoint removal; also accept 200/400/404.
    assert response.get("status") in (200, 400, 404, 406), \
        f"remove_breakpoint unexpected status: {response}"
