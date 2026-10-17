// basic-flow in KickAssembler for the C64: the control flow of main.c by hand
// (add = A + X), so asm-level source stepping has a fixture without a compiler.

BasicUpstart2(start)

        * = $0810 "Code"
start:  lda #1
        sta result
        jsr doSomething
        lda result
        ldx #2
        jsr add
        sta result
loop:   jmp loop            // stable debugger stop point

doSomething:
        lda result
        ldx #3
        jsr add
        sta result
        rts

add:    stx tmp
        clc
        adc tmp
        rts

result: .byte 0
tmp:    .byte 0
