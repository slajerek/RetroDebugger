/* basic-flow: the shared minimal source of spec #9.2. Step Into enters add()
   and doSomething(), Step Over runs them, Step Out returns to the caller, and
   the final loop is a stable stop point. Portable across Oscar64, cc65, KickC. */
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
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:16
    lda #1
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:17
    jsr doSomething
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:18
    ldx.z result
    lda #2
    jsr add
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:18
    sta.z result
  __b1:
    jmp __b1
}
doSomething: {
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:12
    ldx.z result
    lda #3
    jsr add
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:12
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:13
    rts
}
// __register(A) char add(__register(X) char left, __register(A) char right)
add: {
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:8
    stx.z $ff
    clc
    adc.z $ff
    // /private/tmp/rd-source-debug/kickc/atari/basic-flow/main.c:9
    rts
}
