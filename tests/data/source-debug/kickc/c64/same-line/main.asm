/* same-line: several statements and calls on one source line. A source step
   must stop at each statement in column order, and Step Over on "a(); b();"
   must not stop inside a() or b(). */
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
    lda #0
    sta.z result
    jsr a
    jsr b
    lda.z result
    asl
    sta.z result
    jsr a
  __b1:
    jmp __b1
}
a: {
    ldx.z result
    inx
    stx.z result
    rts
}
b: {
    lda.z result
    clc
    adc #2
    sta.z result
    rts
}
