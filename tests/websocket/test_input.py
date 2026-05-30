"""Input endpoint tests: keyboard key down/up, joystick down/up.

KERNAL keyboard buffer:
  $00C6 — number of chars in buffer (0..10)
  $0277-$0280 — buffer

API shapes discovered from src/Remote/CDebuggerServerApi.cpp:
  key/down and key/up use params: {"keyCode": <int>}
    where keyCode is the mtKeyCode — for letter keys this is the
    lowercase ASCII value (e.g. ord('a') = 97 for the A key);
    uppercase sends the key with LEFT_SHIFT (e.g. ord('A') = 65).
    keyboard_key_pressed() in VICE maps this via the C64 key matrix
    (C64KeyMap, registered via keyboard_parse_set_pos_row).
    Status 200 = key found in keymap and latch set.
    Status 406 = key not in keymap.

  joystick/down and joystick/up use params: {"port": <int>, "axis": <str>}
    axis accepts directional names: "n"/"north", "s"/"south",
    "e"/"east", "w"/"west", "ne", "nw", "se", "sw",
    "fire", "fireB", "select", "start".
    port is 0-indexed (driver adds 1 internally for VICE).

KNOWN LIMITATION (Phase 4 finding):
  The keyboard/down and keyboard/up endpoints call VICE's keyboard_key_pressed()
  from the WebSocket handler thread. This function calls alarm_set() on
  maincpu_alarm_context without holding the emulator mutex. The VICE alarm
  context data structures are not thread-safe, creating a race with the main
  VICE emulation loop. As a result, the keyboard alarm may not be set correctly
  and the KERNAL keyboard buffer remains empty even when the API returns 200.
  See src/Remote/CDebuggerServerApi.cpp (key/down handler) vs.
  src/Emulators/vice/ViceInterface/CDebuggerServerApiVice.cpp (other endpoints
  that do use LockMutex/UnlockMutex). Fix: wrap key/down and key/up handlers in
  LockMutex/UnlockMutex in CDebuggerServerApiVice.cpp, or dispatch the
  keyboard call to the VICE main thread.
"""

import pytest
import time


@pytest.mark.slow
def test_key_down_accepted_by_api(fresh_cpu):
    """key/down with a valid keyCode (ASCII letter) must reach the C64 screen.

    The API uses params: {"keyCode": <int>} where the value is the mtKeyCode.
    For letter keys this is the lowercase ASCII value (ord('a') = 97).

    End-to-end check: we type 'a','b','c','d' into a fresh BASIC prompt and
    verify the four screen-code bytes ($01 $02 $03 $04 = "ABCD" in C64
    uppercase mode) appear at the cursor row of the screen RAM. Watching
    the KERNAL keyboard buffer at $00C6 directly is unreliable because
    BASIC's GETIN main loop drains that buffer faster than we can poll
    over the WebSocket -- the buffer count is almost always 0 by the time
    we ask. The screen is the durable observable.
    """
    rd = fresh_cpu
    rd.cont()
    time.sleep(2.0)  # let the kernal cold-start finish + BASIC paint READY. + cursor

    # The READY. prompt + cursor sit at row 6 of the splash (screen offset $0F0+).
    # Snapshot that row only -- the rest of the screen has splash text and a
    # blinking cursor we don't want to chase.
    row_addr = 0x0400 + 6 * 40
    row_before = rd.read_memory(row_addr, 40)

    # Type a, b, c, d -- check API accepts each keycode.
    for ch in "abcd":
        resp_down, _ = rd.call(f"{rd.platform}/input/key/down", {"keyCode": ord(ch)})
        assert resp_down.get("status", 200) == 200, \
            f"key/down rejected valid keyCode {ord(ch)}: {resp_down}"
        time.sleep(0.1)
        resp_up, _ = rd.call(f"{rd.platform}/input/key/up", {"keyCode": ord(ch)})
        assert resp_up.get("status", 200) == 200, \
            f"key/up rejected valid keyCode {ord(ch)}: {resp_up}"
        time.sleep(0.1)

    time.sleep(0.5)  # let BASIC draw the chars
    rd.pause()

    row_after = rd.read_memory(row_addr, 40)
    # The first 4 cells of row 6 should now contain screen codes $01..$04 (A B C D
    # in C64 uppercase mode). Allow row_before to differ on cell[0] only (the
    # blinking cursor flipped between space $20 and a filled cell).
    assert row_after[:4] == b"\x01\x02\x03\x04", \
        f"Row 6 head = {row_after[:4].hex()}, expected 01020304 (ABCD).  Before: {row_before[:4].hex()}"


def test_joystick_down_does_not_error(fresh_cpu):
    """Joystick input call should succeed.

    API: {"port": <int>, "axis": <str>}
    axis accepts compass names: "n"/"north", "s"/"south", "e"/"east", "w"/"west",
    "ne", "nw", "se", "sw", "fire", "fireB", "select", "start".
    port is 0-indexed (VICE adds 1 internally).
    """
    rd = fresh_cpu
    resp, _ = rd.call(f"{rd.platform}/input/joystick/down", {"port": 2, "axis": "n"})
    # Status 200 means accepted
    assert resp.get("status", 200) == 200, f"joystick/down failed: {resp}"
    rd.call(f"{rd.platform}/input/joystick/up", {"port": 2, "axis": "n"})
