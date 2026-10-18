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
080e : 8e b3 08 STX $08b3 ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 b4 __ LDY #$b4
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
082a : c0 b5 __ CPY #$b5
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
;  21, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
.s4:
;  22, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
0880 : a9 01 __ LDA #$01
0882 : 85 0f __ STA P2 
0884 : 20 99 08 JSR $0899 ; (top.s4 + 0)
0887 : 8d b4 08 STA $08b4 ; (result + 0)
;  23, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
088a : ad b4 08 LDA $08b4 ; (result + 0)
088d : 20 af 08 JSR $08af ; (leaf.s4 + 0)
0890 : 20 af 08 JSR $08af ; (leaf.s4 + 0)
0893 : 8d b4 08 STA $08b4 ; (result + 0)
.l5:
0896 : 4c 96 08 JMP $0896 ; (main.l5 + 0)
--------------------------------------------------------------------
top: ; top(u8)->u8
;  17, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
.s4:
;  18, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
0899 : a5 0f __ LDA P2 ; (v + 0)
089b : 85 0e __ STA P1 
089d : 20 a4 08 JSR $08a4 ; (middle.s4 + 0)
08a0 : 18 __ __ CLC
08a1 : 69 01 __ ADC #$01
.s3:
08a3 : 60 __ __ RTS
--------------------------------------------------------------------
middle: ; middle(u8)->u8
;  12, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
.s4:
;  13, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
08a4 : a5 0e __ LDA P1 ; (v + 0)
08a6 : 20 af 08 JSR $08af ; (leaf.s4 + 0)
08a9 : 8d ff 9f STA $9fff ; (w + 0)
;  14, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
08ac : 4c af 08 JMP $08af ; (leaf.s4 + 0)
--------------------------------------------------------------------
leaf: ; leaf(u8)->u8
;   8, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
.s4:
;   9, "/tmp/rd-source-debug/oscar64/c64/nested-calls/main.c"
08af : 18 __ __ CLC
08b0 : 69 01 __ ADC #$01
.s3:
08b2 : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
08b3 : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
result:
08b4 : __ __ __ BSS	1
