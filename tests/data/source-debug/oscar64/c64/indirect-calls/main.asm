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
080e : 8e d0 08 STX $08d0 ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 d5 __ LDY #$d5
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
082a : c0 d6 __ CPY #$d6
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
;  19, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
.s1:
0880 : 38 __ __ SEC
0881 : a5 23 __ LDA SP + 0 
0883 : e9 03 __ SBC #$03
0885 : 85 23 __ STA SP + 0 
0887 : b0 02 __ BCS $088b ; (main.s4 + 0)
0889 : c6 24 __ DEC SP + 1 
.s4:
;  21, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
088b : a9 01 __ LDA #$01
088d : 8d d5 08 STA $08d5 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
0890 : a9 00 __ LDA #$00
0892 : 8d ff 9f STA $9fff ; (i + 0)
.l5:
;  23, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
0895 : ad d5 08 LDA $08d5 ; (result + 0)
0898 : a0 02 __ LDY #$02
089a : 91 23 __ STA (SP + 0),y 
089c : ad ff 9f LDA $9fff ; (i + 0)
089f : 0a __ __ ASL
08a0 : aa __ __ TAX
08a1 : bd d1 08 LDA $08d1,x ; (operations[0] + 0)
08a4 : 85 1b __ STA ACCU + 0 
08a6 : bd d2 08 LDA $08d2,x ; (operations[0] + 1)
08a9 : 85 1c __ STA ACCU + 1 
08ab : 20 cd 08 JSR $08cd ; (bcexec + 0)
08ae : a5 1b __ LDA ACCU + 0 
08b0 : 8d d5 08 STA $08d5 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
08b3 : ee ff 9f INC $9fff ; (i + 0)
08b6 : ad ff 9f LDA $9fff ; (i + 0)
08b9 : c9 02 __ CMP #$02
08bb : 90 d8 __ BCC $0895 ; (main.l5 + 0)
.l6:
08bd : b0 fe __ BCS $08bd ; (main.l6 + 0)
--------------------------------------------------------------------
twice: ; twice(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
08bf : a0 02 __ LDY #$02
08c1 : b1 23 __ LDA (SP + 0),y 
08c3 : 0a __ __ ASL
.s3:
08c4 : 60 __ __ RTS
--------------------------------------------------------------------
plusOne: ; plusOne(u8)->u8
;  11, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
.s4:
;  12, "/private/tmp/rd-source-debug/oscar64/c64/indirect-calls/main.c"
08c5 : a0 02 __ LDY #$02
08c7 : b1 23 __ LDA (SP + 0),y 
08c9 : 18 __ __ CLC
08ca : 69 01 __ ADC #$01
.s3:
08cc : 60 __ __ RTS
--------------------------------------------------------------------
bcexec: ; bcexec
08cd : 6c 1b 00 JMP ($001b)
--------------------------------------------------------------------
spentry:
08d0 : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
operations:
08d1 : __ __ __ BYT bf 08 c5 08                                     : ....
--------------------------------------------------------------------
result:
08d5 : __ __ __ BSS	1
