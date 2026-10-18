// KickAssembler
#import "lib.asm"
*= $0801
start:  lda #$01        ; load
        sta $d020
!loop:  dex
        bne !loop-
        jsr add.sub
/* block
   comment */ .byte %1010, 12, "AB"
label2  rts
