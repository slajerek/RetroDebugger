"""Top-level load endpoint tests: PRG, D64."""

import time

import pytest


def test_load_prg_places_bytes_at_load_address(fresh_cpu, fixture_dir):
    """`load` of known_state.prg should put the BASIC SYS line at $0801."""
    rd = fresh_cpu
    resp, _ = rd.call("load", {"path": str(fixture_dir / "known_state.prg")})
    assert resp.get("status") == 200, f"load failed: {resp}"
    time.sleep(0.2)
    # $0801 is BASIC pointer; first bytes are SYS-line tokens, not zero.
    head = rd.read_memory(0x0801, 16)
    assert any(b != 0 for b in head), \
        f"After load, memory at $0801 is empty: {head.hex()}"


def test_load_prg_then_makejmp_and_run(fresh_cpu, fixture_dir):
    """End-to-end: load + makejmp + cont reaches park with sentinels in place."""
    rd = fresh_cpu
    resp, _ = rd.call("load", {"path": str(fixture_dir / "known_state.prg")})
    assert resp.get("status") == 200, f"load failed: {resp}"
    time.sleep(0.2)
    rd.call(f"{rd.platform}/cpu/makejmp", {"address": 0x0810})
    rd.cont()
    time.sleep(0.05)
    rd.pause()
    sentinels = rd.read_memory(0xC000, 4)
    assert sentinels == bytes([0xAA, 0xBB, 0xCC, 0xDD]), \
        f"E2E run produced wrong sentinels: {sentinels.hex()}"


def test_load_nonexistent_file_does_not_crash(fresh_cpu):
    """Loading a missing file should return an error response, not crash the bridge.

    The 3.1 baseline crashed the WS bridge with an EXC_BAD_ACCESS in glBindTexture
    on the WSDebugServer thread: CMainMenuHelper::LoadFile dispatched into
    CMainMenuHelper::LoadPRG which in turn called recentlyOpenedFiles->Add and
    ShowMessageError -> guiMain->ShowNotification, ultimately ending up in the
    render backend on the wrong thread. The 3.10 build gates the load endpoint
    with a stat(2) up front (see CDebuggerServerWebSockets.cpp), returning
    HTTP_NOT_FOUND before any UI-touching code runs.
    """
    rd = fresh_cpu
    resp, _ = rd.call("load", {"path": "/nonexistent/file.prg"})
    # Should be a 4xx response, but the connection must survive.
    assert "status" in resp, f"load of nonexistent file returned no status: {resp}"
    assert resp["status"] == 404, f"expected 404 for missing file, got {resp}"
    # Verify we can still talk to RD.
    status = rd.cpu_status()
    assert status["status"] == 200, "WebSocket dead after bad load"
