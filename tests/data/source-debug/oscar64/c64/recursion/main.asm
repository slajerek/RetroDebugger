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
080e : 8e d8 08 STX $08d8 ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 d9 __ LDY #$d9
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
082a : c0 da __ CPY #$da
082c : d0 f9 __ BNE $0827 ; (startup + 38)
082e : a9 00 __ LDA #$00
0830 : a2 f7 __ LDX #$f7
0832 : d0 03 __ BNE $0837 ; (startup + 54)
0834 : 95 00 __ STA $00,x 
0836 : e8 __ __ INX
0837 : e0 f7 __ CPX #$f7
0839 : d0 f9 __ BNE $0834 ; (startup + 51)
083b : a9 fe __ LDA #$fe
083d : 85 23 __ STA SP + 0 
083f : a9 9f __ LDA #$9f
0841 : 85 24 __ STA SP + 1 
0843 : 20 80 08 JSR $0880 ; (main.s1 + 0)
0846 : a9 4c __ LDA #$4c
0848 : 85 54 __ STA $54 
084a : a9 00 __ LDA #$00
084c : 85 13 __ STA P6 
084e : a9 19 __ LDA #$19
0850 : 85 16 __ STA P9 
0852 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  13, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
.s1:
0880 : 38 __ __ SEC
0881 : a5 23 __ LDA SP + 0 
0883 : e9 03 __ SBC #$03
0885 : 85 23 __ STA SP + 0 
0887 : b0 02 __ BCS $088b ; (main.s4 + 0)
0889 : c6 24 __ DEC SP + 1 
.s4:
;  14, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
088b : a9 03 __ LDA #$03
088d : a0 02 __ LDY #$02
088f : 91 23 __ STA (SP + 0),y 
0891 : 20 9c 08 JSR $089c ; (sum.s1 + 0)
0894 : a5 1b __ LDA ACCU + 0 
0896 : 8d d9 08 STA $08d9 ; (result + 0)
.l5:
0899 : 4c 99 08 JMP $0899 ; (main.l5 + 0)
--------------------------------------------------------------------
sum: ; sum(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
.s1:
089c : 38 __ __ SEC
089d : a5 23 __ LDA SP + 0 
089f : e9 05 __ SBC #$05
08a1 : 85 23 __ STA SP + 0 
08a3 : b0 02 __ BCS $08a7 ; (sum.s1 + 11)
08a5 : c6 24 __ DEC SP + 1 
08a7 : a0 03 __ LDY #$03
08a9 : a5 53 __ LDA T1 + 0 
08ab : 91 23 __ STA (SP + 0),y 
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
08ad : a0 07 __ LDY #$07
08af : b1 23 __ LDA (SP + 0),y 
08b1 : f0 11 __ BEQ $08c4 ; (sum.s5 + 0)
.s6:
08b3 : 85 53 __ STA T1 + 0 
;  10, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
08b5 : 38 __ __ SEC
08b6 : e9 01 __ SBC #$01
08b8 : a0 02 __ LDY #$02
08ba : 91 23 __ STA (SP + 0),y 
08bc : 20 9c 08 JSR $089c ; (sum.s1 + 0)
08bf : 18 __ __ CLC
08c0 : a5 1b __ LDA ACCU + 0 
08c2 : 65 53 __ ADC T1 + 0 
.s5:
;   9, "/private/tmp/rd-source-debug/oscar64/c64/recursion/main.c"
08c4 : 85 1b __ STA ACCU + 0 
.s3:
08c6 : a0 03 __ LDY #$03
08c8 : b1 23 __ LDA (SP + 0),y 
08ca : 85 53 __ STA T1 + 0 
08cc : 18 __ __ CLC
08cd : a5 23 __ LDA SP + 0 
08cf : 69 05 __ ADC #$05
08d1 : 85 23 __ STA SP + 0 
08d3 : 90 02 __ BCC $08d7 ; (sum.s3 + 17)
08d5 : e6 24 __ INC SP + 1 
08d7 : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
08d8 : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
result:
08d9 : __ __ __ BSS	1
