; ca65
.segment "CODE"
_main:  lda #<msg
@loop:  sta $0400,x
        inx
        bne @loop
        rts
msg:    .byte "hello", 0
