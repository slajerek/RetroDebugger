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
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:11
    lda #0
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:12
    jsr a
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:12
    jsr b
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:13
    lda.z result
    asl
    sta.z result
    jsr a
  __b1:
    jmp __b1
}
a: {
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:7
    ldx.z result
    inx
    stx.z result
    rts
}
b: {
    // /private/tmp/rd-source-debug/kickc/c64/same-line/main.c:8
    lda.z result
    clc
    adc #2
    sta.z result
    rts
}
