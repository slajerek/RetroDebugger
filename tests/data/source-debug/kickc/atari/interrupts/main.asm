/* interrupts: tick() is the interrupt's work. The test installs a fast
   timer IRQ whose handler calls it, so mapped code runs between any two
   instructions of main and work(). Source steps must never stop in tick(). */
  // Atari XL/XE executable XEX file with a single segment
// https://www.atarimax.com/jindroush.atari.org/afmtexe.html
.plugin "dk.camelot64.kickass.xexplugin.AtariXex"
.file [name="main.xex", type="bin", segments="XexFile"]
.segmentdef XexFile [segments="Program", modify="XexFormat", _RunAddr=__start]
.segmentdef Program [segments="Code, Data"]
.segmentdef Code [start=$2000]
.segmentdef Data [startAfter="Code"]
  .label result = $80
.segment Code
__start: {
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:6
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:22
    lda #0
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:23
    jsr work
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:24
    jsr work
  __b1:
    jmp __b1
}
work: {
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:16
    ldx.z result
    inx
    stx.z result
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:17
    txa
    clc
    adc #2
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/interrupts/main.c:18
    rts
}
