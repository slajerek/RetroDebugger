"""Chip register tests: VIC-II, CIA1, CIA2, SID.

Confirmed response shape from RetroDebugger 3.1:
  {"result": {"registers": [[reg_num, value], ...]}, "status": 200}
  where registers is a list of [reg_num, value] pairs (nlohmann/json serialization
  of a C++ unordered_map produces a JSON array of [key, value] pairs).
"""


def test_vic_read_returns_requested_registers(fresh_cpu):
    """VIC read with a list of register numbers returns the requested registers.

    API: c64/vic/read  params: registers (list of ints, 0-based from $D000)
    Response shape: {"result": {"registers": [[reg_num, val], ...]}, "status": 200}
    """
    rd = fresh_cpu
    rd.pause()
    # $D020 is VIC border color (reg 0x20), $D021 is background color (reg 0x21).
    resp, _ = rd.call(f"{rd.platform}/vic/read", {"registers": [0x20, 0x21]})
    assert "result" in resp, f"vic/read returned no result: {resp}"
    result = resp["result"]
    assert "registers" in result, \
        f"vic/read result missing 'registers' key: {result}"
    regs = result["registers"]
    # Server returns [[reg_num, value], ...] pairs.
    assert isinstance(regs, list) and len(regs) >= 2, \
        f"vic/read shape: {result}"
    returned = {pair[0] for pair in regs}
    assert {0x20, 0x21} <= returned, f"vic/read missing requested regs: got {returned}"


def test_vic_write_then_read_round_trips(fresh_cpu):
    """Writing $07 (yellow) to VIC border, then reading back, should return $07."""
    rd = fresh_cpu
    rd.pause()
    # Write border = 7 (register 0x20, key as string per API — iterates params["registers"].items())
    rd.call(f"{rd.platform}/vic/write", {"registers": {"0x20": 7}})
    # Read back
    resp, _ = rd.call(f"{rd.platform}/vic/read", {"registers": [0x20]})
    assert "result" in resp, f"vic/read returned no result after write: {resp}"
    regs = resp["result"]["registers"]
    # The server returns registers as a list of [reg_num, value] pairs, e.g. [[32, 247]].
    # (C++ unordered_map serialized via nlohmann/json produces a JSON array of pairs.)
    # Also tolerate a plain dict {str: int} in case the serializer changes.
    if isinstance(regs, dict):
        val = next(iter(regs.values()))
    elif isinstance(regs, list) and regs and isinstance(regs[0], list):
        # [[reg_num, value], ...] — take the value from the first pair
        val = regs[0][1]
    elif isinstance(regs, list):
        val = regs[0]
    else:
        val = regs  # scalar fallback
    assert val is not None, f"vic/read registers is empty: {regs}"
    assert (val & 0x0F) == 7, f"VIC $D020 readback: got {val}, expected 7 (border yellow)"


def test_cia1_read_returns_requested_registers(fresh_cpu):
    """CIA1 read with num=0 returns the requested register values.

    API: c64/cia/read  params: num (0|1, optional, default 0), registers (list of ints)
    Note: the param is 'num', NOT 'cia' — confirmed from CDebuggerServerApiVice.cpp line 195.
    Response shape: {"result": {"registers": [[reg_num, val], ...]}, "status": 200}
    """
    rd = fresh_cpu
    rd.pause()
    resp, _ = rd.call(f"{rd.platform}/cia/read", {"num": 0, "registers": [0x00, 0x01]})
    assert "result" in resp, f"cia/read returned no result: {resp}"
    result = resp["result"]
    assert "registers" in result, \
        f"cia/read result missing 'registers' key: {result}"
    regs = result["registers"]
    # Server returns [[reg_num, value], ...] pairs.
    assert isinstance(regs, list) and len(regs) >= 2, \
        f"cia/read shape: {result}"
    returned = {pair[0] for pair in regs}
    assert {0x00, 0x01} <= returned, f"cia/read missing requested regs: got {returned}"


def test_sid_read_returns_requested_registers(fresh_cpu):
    """SID read with num=0 returns the requested register values.

    API: c64/sid/read  params: num (0-based, optional, default 0), registers (list of ints)
    Note: the param is 'num', NOT 'sid' — confirmed from CDebuggerServerApiVice.cpp line 289.
    Response shape: {"result": {"registers": [[reg_num, val], ...]}, "status": 200}
    """
    rd = fresh_cpu
    rd.pause()
    # $D418 = SID volume/filter mode (reg 0x18); $D41B = SID osc3 read (reg 0x1B).
    resp, _ = rd.call(f"{rd.platform}/sid/read", {"num": 0, "registers": [0x18, 0x1B]})
    assert "result" in resp, f"sid/read returned no result: {resp}"
    result = resp["result"]
    assert "registers" in result, \
        f"sid/read result missing 'registers' key: {result}"
    regs = result["registers"]
    # Server returns [[reg_num, value], ...] pairs.
    assert isinstance(regs, list) and len(regs) >= 2, \
        f"sid/read shape: {result}"
    returned = {pair[0] for pair in regs}
    assert {0x18, 0x1B} <= returned, f"sid/read missing requested regs: got {returned}"
