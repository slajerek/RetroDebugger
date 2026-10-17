/* basic-flow: the shared minimal source of spec #9.2. Step Into enters add()
   and doSomething(), Step Over runs them, Step Out returns to the caller, and
   the final loop is a stable stop point. Portable across Oscar64, cc65, KickC. */
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
