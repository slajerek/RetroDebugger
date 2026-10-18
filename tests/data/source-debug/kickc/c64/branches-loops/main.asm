/* branches-loops: if/else, for, while, switch and an early return. Step Over
   follows the taken path line by line and comes back to the loop head on
   every iteration. */
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
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:21
    lda #0
    sta.z result
    tax
  __b2:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:22
    cpx #3
    bcc __b3
  __b4:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:24
    lda.z result
    cmp #1+1
    bcs __b5
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:26
    lda #1
    cmp.z result
    beq __b1
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:29
    lda #5
    jsr classify
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:29
    sta.z result
  __b8:
    jmp __b8
  __b1:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:27
    lda #3
    jsr classify
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:27
    sta.z result
    jmp __b8
  __b5:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:25
    lda.z result
    sec
    sbc #2
    sta.z result
    jmp __b4
  __b3:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:23
    txa
    clc
    adc.z result
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:22
    inx
    jmp __b2
}
// __register(A) char classify(__register(A) char v)
classify: {
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:8
    cmp #3
    bcs __b1
    lda #1
    rts
  __b1:
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:11
    cmp #3
    beq __b2
    lda #3
    rts
  __b2:
    lda #2
    // /private/tmp/rd-source-debug/kickc/c64/branches-loops/main.c:17
    rts
}
