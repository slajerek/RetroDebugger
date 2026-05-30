// known_state.asm — RetroDebugger WebSocket regression-suite fixture.
//
// Loaded via the `load` endpoint at $0801 (BASIC SYS line).
// Entry point at $0810 via `makejmp`.
//
// Post-run state (after letting it reach `park`):
//   $C000 = $AA
//   $C001 = $BB
//   $C002 = $CC
//   $C003 = $DD
//   $0400-$07E7 cleared to $20 (space char) — screen RAM
//   PC parked in the infinite loop at label `park`
//
// Build: kickass known_state.asm -o known_state.prg

*=$0801 "BASIC Upstart"
BasicUpstart2(start)

*=$0810 "Code"
start:
    // Clear visible screen RAM ($0400-$07FF) to space
    lda #$20
    ldx #$00
clear_loop:
    sta $0400, x
    sta $0500, x
    sta $0600, x
    sta $0700, x
    inx
    bne clear_loop

    // Write known sentinel bytes to $C000-$C003
    lda #$AA
    sta $C000
    lda #$BB
    sta $C001
    lda #$CC
    sta $C002
    lda #$DD
    sta $C003

park:
    jmp park
