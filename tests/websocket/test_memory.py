"""Memory endpoint tests: main memory + drive 1541 memory."""

import time


# ── main memory ────────────────────────────────────────────────────────────

def test_read_main_memory_returns_requested_size(fresh_cpu):
    """readBlock should return exactly `size` bytes."""
    data = fresh_cpu.read_memory(0x0400, 256)
    assert len(data) == 256, f"Expected 256 bytes, got {len(data)}"


def test_write_then_read_round_trips(fresh_cpu):
    """writeBlock followed by readBlock should return the same data."""
    rd = fresh_cpu
    rd.pause()
    payload = bytes(range(64))
    rd.write_memory(0xC100, payload)
    readback = rd.read_memory(0xC100, len(payload))
    assert readback == payload, f"Round-trip mismatch:\n  wrote: {payload.hex()}\n  read:  {readback.hex()}"


def test_loaded_fixture_sentinels(loaded_fixture):
    """After running known_state.prg, sentinel bytes at $C000 must match."""
    data = loaded_fixture.read_memory(0xC000, 4)
    assert data == bytes([0xAA, 0xBB, 0xCC, 0xDD]), \
        f"Sentinels at $C000: got {data.hex()}, expected aabbccdd"


def test_loaded_fixture_screen_cleared(loaded_fixture):
    """After running known_state.prg, screen RAM should be space ($20)."""
    screen = loaded_fixture.read_memory(0x0400, 1000)
    assert all(b == 0x20 for b in screen), \
        f"Screen not cleared: first non-space at offset {next(i for i, b in enumerate(screen) if b != 0x20)}"


def test_ram_readblock_excludes_io(fresh_cpu):
    """Reading the I/O area ($D000-$DFFF) via /ram/ should see RAM-under-I/O, not chip registers.

    /cpu/memory/ uses the CPU view (sees I/O); /ram/ reads raw RAM bytes underneath.
    Test by writing 0xEE to $D020 via RAM (won't change VIC border), then reading both."""
    rd = fresh_cpu
    rd.pause()
    # Write a sentinel to $D020 via /ram/ — RAM under I/O
    rd.write_memory(0xD020, b"\xEE", with_io=False)
    # Read via /ram/ — should see our sentinel
    ram_view = rd.read_memory(0xD020, 1, with_io=False)
    # Read via /cpu/memory/ — sees VIC register value (border color, low nibble)
    cpu_view = rd.read_memory(0xD020, 1, with_io=True)
    assert ram_view == b"\xEE", f"/ram/ view of $D020: got {ram_view.hex()}, expected ee"
    # CPU view: VIC border register only uses low 4 bits; high nibble undefined.
    # Just assert they differ (proves the two views are distinct).
    assert ram_view != cpu_view, \
        f"/ram/ and /cpu/memory/ returned same byte for $D020 — I/O distinction broken"


# ── drive 1541 memory ──────────────────────────────────────────────────────

def test_read_drive_memory_returns_requested_size(fresh_cpu):
    """Drive 1541 readBlock should return exactly `size` bytes."""
    rd = fresh_cpu
    rd.pause()
    data = rd.read_drive_memory(0x00, 16)
    assert len(data) == 16, f"Expected 16 bytes from drive, got {len(data)}"


def test_drive_via1_readable(fresh_cpu):
    """VIA1 at $1800 (with_io=True) should produce some defined byte values."""
    rd = fresh_cpu
    rd.pause()
    data = rd.read_drive_memory(0x1800, 4, with_io=True)
    assert len(data) == 4
    # Bytes can be any value, but the call must succeed without exception.


def test_drive_memory_write_then_read(fresh_cpu):
    """Round-trip writeBlock/readBlock against drive RAM."""
    rd = fresh_cpu
    rd.pause()
    payload = bytes([0x11, 0x22, 0x33, 0x44])
    rd.write_drive_memory(0x0500, payload)
    readback = rd.read_drive_memory(0x0500, len(payload))
    assert readback == payload, \
        f"Drive round-trip mismatch:\n  wrote: {payload.hex()}\n  read:  {readback.hex()}"
