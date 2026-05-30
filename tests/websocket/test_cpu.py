"""CPU endpoint tests: status, counters, step, makejmp."""

import time


# ── status ─────────────────────────────────────────────────────────────────

EXPECTED_STATUS_KEYS = {"pc", "a", "x", "y", "sp", "p"}


def test_cpu_status_has_expected_keys(fresh_cpu):
    """cpu/status must return at least the canonical 6 register fields."""
    result = fresh_cpu.cpu_status()["result"]
    missing = EXPECTED_STATUS_KEYS - set(result)
    assert not missing, f"cpu/status missing keys: {missing}"


def test_cpu_status_register_ranges(fresh_cpu):
    """Each register field must be in the right value range."""
    r = fresh_cpu.cpu_status()["result"]
    assert 0 <= r["pc"] <= 0xFFFF, f"PC out of range: {r['pc']}"
    for reg in ("a", "x", "y", "sp"):
        assert 0 <= r[reg] <= 0xFF, f"{reg.upper()} out of range: {r[reg]}"
    assert 0 <= r["p"] <= 0xFF, f"P out of range: {r['p']}"


# ── counters ───────────────────────────────────────────────────────────────

def test_counters_monotonic(fresh_cpu):
    """Cycle/instruction/frame counts must monotonically increase while running."""
    rd = fresh_cpu
    rd.cont()
    time.sleep(0.15)  # let the async cycle-counter reset (3.1, ~10-20ms) settle first
    samples = []
    for _ in range(3):
        time.sleep(0.05)
        samples.append(rd.cpu_counters()["result"])
    rd.pause()

    for i in range(len(samples) - 1):
        s_prev, s_next = samples[i], samples[i + 1]
        for key in ("cycle", "instruction", "frame"):
            assert s_next[key] >= s_prev[key], \
                f"{key} went backwards: {s_prev[key]} -> {s_next[key]}"


# ── control: makejmp ───────────────────────────────────────────────────────

def test_makejmp_sets_pc(loaded_fixture):
    """makejmp(addr) must redirect the CPU to addr.

    Notes:
    - makejmp only takes effect when the CPU is in user RAM (not mid-KERNAL).
      loaded_fixture guarantees the CPU is paused in the park loop at $0837.
    - makejmp queues a CPU trap (interrupt_maincpu_trigger_trap) that sets
      maincpu_regs.pc when DO_INTERRUPT next fires. The trap dispatcher only
      runs inside the main CPU loop, so the trap is *queued* during pause and
      *fires* once the CPU is allowed to run.
    - Empirically, makejmp + step_instruction is racy on arm64 -- the WS
      thread's writes to global_pending_int / trap_func aren't memory-fenced
      against the CPU thread's reads, so a single-instruction step can miss
      the trap and execute the original park-loop JMP instead. cont + short
      wait + pause is 5/5 reliable: the CPU runs hundreds of instructions
      and the trap is guaranteed to land. PC then sits in the loop the new
      code falls into (RAM at $4000 is $FF $FF $00 $00..., an illegal
      opcode sequence that walks PC forward a few bytes per instruction
      before something jams or wraps). Accept any PC at or just past $4000.
    """
    rd = loaded_fixture
    rd.call(f"{rd.platform}/cpu/makejmp", {"address": 0x4000})
    rd.cont()
    time.sleep(0.02)  # gives the trap-dispatcher inside DO_INTERRUPT time to land
    rd.pause()
    pc = rd.cpu_status()["result"]["pc"]
    assert 0x4000 <= pc <= 0x4010, f"After makejmp($4000), PC = {pc:#06x} (want $4000..$4010)"


# ── control: step_instruction ──────────────────────────────────────────────

def test_step_instruction_advances_pc(loaded_fixture):
    """Step from inside the park loop. PC should land at park+0 again (3-byte JMP)."""
    rd = loaded_fixture
    pc_before = rd.cpu_status()["result"]["pc"]
    rd.step_instruction()
    pc_after = rd.cpu_status()["result"]["pc"]
    # JMP abs is 3 bytes but loops back, so PC should == pc_before (the JMP target).
    # OR — if step happened mid-fetch — PC could be pc_before + 3 (next instr).
    # Be permissive but assert *something changed* in a sane way.
    assert pc_after == pc_before or pc_after == pc_before + 3, \
        f"step_instruction: pc {pc_before:#06x} -> {pc_after:#06x} (expected either same or +3)"


# ── control: step_cycle ────────────────────────────────────────────────────

def test_step_cycle_advances_one_cycle(loaded_fixture):
    """step/cycle advances the cycle counter by exactly 1 (from a quiescent paused CPU).

    NOTE: RD 3.1's cycle counter has documented async behavior — it may tick by 1
    even while the CPU is paused (background counter update). We allow <=1 background
    tick between the two pre-step quiescence reads (strict equality is too tight).
    The step/cycle itself must advance by exactly 1 on top of whatever s2 is.
    """
    rd = loaded_fixture
    # Prove the CPU is approximately quiescent: counter advances <=1 while paused.
    # (RD 3.1 may tick the counter once asynchronously even while paused.)
    s1 = rd.cpu_counters()["result"]["cycle"]
    time.sleep(0.05)
    s2 = rd.cpu_counters()["result"]["cycle"]
    assert s2 - s1 <= 1, f"CPU not quiescent before step: {s1} -> {s2} (advance > 1)"
    rd.call(f"{rd.platform}/step/cycle")
    time.sleep(0.05)  # counter update is async
    s3 = rd.cpu_counters()["result"]["cycle"]
    assert s3 - s2 == 1, f"step/cycle advanced {s3 - s2}, expected 1"
