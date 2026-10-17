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
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    lda #1
    sta.z result
    jsr doSomething
    ldx.z result
    lda #2
    jsr add
    sta.z result
  __b1:
    jmp __b1
}
doSomething: {
    ldx.z result
    lda #3
    jsr add
    sta.z result
    rts
}
// __register(A) char add(__register(X) char left, __register(A) char right)
add: {
    stx.z $ff
    clc
    adc.z $ff
    rts
}
