/* interrupts: tick() is the interrupt's work. The test installs a fast
   timer IRQ whose handler calls it, so mapped code runs between any two
   instructions of main and work(). Source steps must never stop in tick(). */
  // Commodore 64 PRG executable file
.file [name="main.prg", type="prg", segments="Program"]
.segmentdef Program [segments="Basic, Code, Data"]
.segmentdef Basic [start=$0801]
.segmentdef Code [start=$80d]
.segmentdef Data [startAfter="Code"]
.segment Basic
:BasicUpstart(__start)
  .label result = 2
.segment Code
__start: {
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:6
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:22
    lda #0
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:23
    jsr work
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:24
    jsr work
  __b1:
    jmp __b1
}
work: {
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:16
    ldx.z result
    inx
    stx.z result
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:17
    txa
    clc
    adc #2
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/interrupts/main.c:18
    rts
}
