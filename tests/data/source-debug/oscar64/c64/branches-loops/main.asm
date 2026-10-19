; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
0801 : 0b __ __ INV
0802 : 08 __ __ PHP
0803 : 0a __ __ ASL
0804 : 00 __ __ BRK
0805 : 9e __ __ INV
0806 : 32 __ __ INV
0807 : 30 36 __ BMI $083f ; (startup + 62)
0809 : 31 00 __ AND ($00),y 
080b : 00 __ __ BRK
080c : 00 __ __ BRK
080d : ba __ __ TSX
080e : 8e cf 08 STX $08cf ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 d0 __ LDY #$d0
0815 : a9 00 __ LDA #$00
0817 : 85 19 __ STA IP + 0 
0819 : 86 1a __ STX IP + 1 
081b : e0 08 __ CPX #$08
081d : f0 0b __ BEQ $082a ; (startup + 41)
081f : 91 19 __ STA (IP + 0),y 
0821 : c8 __ __ INY
0822 : d0 fb __ BNE $081f ; (startup + 30)
0824 : e8 __ __ INX
0825 : d0 f2 __ BNE $0819 ; (startup + 24)
0827 : 91 19 __ STA (IP + 0),y 
0829 : c8 __ __ INY
082a : c0 d1 __ CPY #$d1
082c : d0 f9 __ BNE $0827 ; (startup + 38)
082e : a9 00 __ LDA #$00
0830 : a2 f7 __ LDX #$f7
0832 : d0 03 __ BNE $0837 ; (startup + 54)
0834 : 95 00 __ STA $00,x 
0836 : e8 __ __ INX
0837 : e0 f7 __ CPX #$f7
0839 : d0 f9 __ BNE $0834 ; (startup + 51)
083b : a9 fd __ LDA #$fd
083d : 85 23 __ STA SP + 0 
083f : a9 9f __ LDA #$9f
0841 : 85 24 __ STA SP + 1 
0843 : 20 80 08 JSR $0880 ; (main.s4 + 0)
0846 : a9 4c __ LDA #$4c
0848 : 85 54 __ STA $54 
084a : a9 00 __ LDA #$00
084c : 85 13 __ STA P6 
084e : a9 19 __ LDA #$19
0850 : 85 16 __ STA P9 
0852 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  19, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
.s4:
;  21, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
0880 : a9 00 __ LDA #$00
0882 : 8d d0 08 STA $08d0 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
0885 : 8d ff 9f STA $9fff ; (i + 0)
;  23, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
0888 : 18 __ __ CLC
.l12:
0889 : ad ff 9f LDA $9fff ; (i + 0)
088c : aa __ __ TAX
;  22, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
088d : e8 __ __ INX
088e : 8e ff 9f STX $9fff ; (i + 0)
;  23, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
0891 : 6d d0 08 ADC $08d0 ; (result + 0)
0894 : 8d d0 08 STA $08d0 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
0897 : e0 03 __ CPX #$03
0899 : 90 ee __ BCC $0889 ; (main.l12 + 0)
.l5:
;  24, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
089b : ad d0 08 LDA $08d0 ; (result + 0)
089e : c9 02 __ CMP #$02
;  25, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08a0 : ad d0 08 LDA $08d0 ; (result + 0)
;  24, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08a3 : 90 08 __ BCC $08ad ; (main.s7 + 0)
.s6:
;  25, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08a5 : e9 02 __ SBC #$02
08a7 : 8d d0 08 STA $08d0 ; (result + 0)
08aa : 4c 9b 08 JMP $089b ; (main.l5 + 0)
.s7:
;  26, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08ad : c9 01 __ CMP #$01
08af : d0 04 __ BNE $08b5 ; (main.s10 + 0)
.s8:
;  27, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08b1 : a9 03 __ LDA #$03
08b3 : d0 02 __ BNE $08b7 ; (main.s11 + 0)
.s10:
;  29, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08b5 : a9 05 __ LDA #$05
.s11:
08b7 : 20 c0 08 JSR $08c0 ; (classify.s4 + 0)
08ba : 8d d0 08 STA $08d0 ; (result + 0)
.l9:
08bd : 4c bd 08 JMP $08bd ; (main.l9 + 0)
--------------------------------------------------------------------
classify: ; classify(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08c0 : c9 03 __ CMP #$03
08c2 : b0 03 __ BCS $08c7 ; (classify.s6 + 0)
.s5:
;   9, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08c4 : a9 01 __ LDA #$01
.s3:
08c6 : 60 __ __ RTS
.s6:
;  10, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08c7 : d0 03 __ BNE $08cc ; (classify.s8 + 0)
.s7:
;  12, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08c9 : a9 02 __ LDA #$02
08cb : 60 __ __ RTS
.s8:
;  16, "/private/tmp/rd-source-debug/oscar64/c64/branches-loops/main.c"
08cc : a9 03 __ LDA #$03
08ce : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
08cf : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
result:
08d0 : __ __ __ BSS	1
