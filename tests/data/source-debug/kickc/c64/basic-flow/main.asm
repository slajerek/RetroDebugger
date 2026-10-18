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
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:5
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:16
    lda #1
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:17
    jsr doSomething
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:18
    ldx.z result
    lda #2
    jsr add
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:18
    sta.z result
  __b1:
    jmp __b1
}
doSomething: {
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:12
    ldx.z result
    lda #3
    jsr add
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:12
    sta.z result
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:13
    rts
}
// __register(A) char add(__register(X) char left, __register(A) char right)
add: {
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:8
    stx.z $ff
    clc
    adc.z $ff
    // /private/tmp/rd-source-debug/kickc/c64/basic-flow/main.c:9
    rts
}
