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
080e : 8e 9b 08 STX $089b ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 9c __ LDY #$9c
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
082a : c0 9d __ CPY #$9d
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
;  22, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
.s4:
;  23, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
0880 : a9 01 __ LDA #$01
0882 : 8d 9c 08 STA $089c ; (result + 0)
;  18, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
0885 : 0e 9c 08 ASL $089c ; (result + 0)
;  19, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
0888 : ad 9c 08 LDA $089c ; (result + 0)
088b : 20 97 08 JSR $0897 ; (addOne.s4 + 0)
088e : 8d 9c 08 STA $089c ; (result + 0)
;  25, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
0891 : 0e 9c 08 ASL $089c ; (result + 0)
.l5:
0894 : 4c 94 08 JMP $0894 ; (main.l5 + 0)
--------------------------------------------------------------------
addOne: ; addOne(u8)->u8
;  13, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
.s4:
;  14, "/private/tmp/rd-source-debug/oscar64/c64-O2/inline/main-O2.c"
0897 : 18 __ __ CLC
0898 : 69 01 __ ADC #$01
.s3:
089a : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
089b : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
result:
089c : __ __ __ BSS	1
