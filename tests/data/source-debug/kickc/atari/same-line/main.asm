/* same-line: several statements and calls on one source line. A source step
   must stop at each statement in column order, and Step Over on "a(); b();"
   must not stop inside a() or b(). */
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
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:11
    lda #0
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:12
    jsr a
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:12
    jsr b
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:13
    lda.z result
    asl
    sta.z result
    jsr a
  __b1:
    jmp __b1
}
a: {
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:7
    ldx.z result
    inx
    stx.z result
    rts
}
b: {
    // /private/tmp/rd-source-debug/kickc/atari/same-line/main.c:8
    lda.z result
    clc
    adc #2
    sta.z result
    rts
}
