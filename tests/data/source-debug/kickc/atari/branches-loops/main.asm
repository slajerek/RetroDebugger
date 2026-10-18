/* branches-loops: if/else, for, while, switch and an early return. Step Over
   follows the taken path line by line and comes back to the loop head on
   every iteration. */
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
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:21
    lda #0
    sta.z result
    tax
  __b2:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:22
    cpx #3
    bcc __b3
  __b4:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:24
    lda.z result
    cmp #1+1
    bcs __b5
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:26
    lda #1
    cmp.z result
    beq __b1
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:29
    lda #5
    jsr classify
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:29
    sta.z result
  __b8:
    jmp __b8
  __b1:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:27
    lda #3
    jsr classify
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:27
    sta.z result
    jmp __b8
  __b5:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:25
    lda.z result
    sec
    sbc #2
    sta.z result
    jmp __b4
  __b3:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:23
    txa
    clc
    adc.z result
    sta.z result
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:22
    inx
    jmp __b2
}
// __register(A) char classify(__register(A) char v)
classify: {
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:8
    cmp #3
    bcs __b1
    lda #1
    rts
  __b1:
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:11
    cmp #3
    beq __b2
    lda #3
    rts
  __b2:
    lda #2
    // /private/tmp/rd-source-debug/kickc/atari/branches-loops/main.c:17
    rts
}
