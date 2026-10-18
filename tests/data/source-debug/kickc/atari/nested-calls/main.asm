/* nested-calls: three levels of calls, a call used as another call's
   argument, and the same function called twice from one line. Step Into
   walks down one level at a time, Step Out climbs one level at a time, and
   Step Over on a line runs the whole chain below it. */
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
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:6
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:22
    jsr top
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:22
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:23
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:23
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:23
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:23
    sta.z result
  __b1:
    jmp __b1
}
// __register(A) char top(char v)
top: {
    .label v = 1
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:18
    jsr middle
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:18
    clc
    adc #1
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:19
    rts
}
// __register(A) char leaf(__register(A) char v)
leaf: {
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:9
    clc
    adc #1
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:10
    rts
}
// __register(A) char middle(char v)
middle: {
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:13
    lda #top.v
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:13
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:14
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:14
    // /private/tmp/rd-source-debug/kickc/atari/nested-calls/main.c:15
    rts
}
