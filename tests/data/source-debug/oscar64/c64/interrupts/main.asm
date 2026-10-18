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
080e : 8e a6 08 STX $08a6 ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 a7 __ LDY #$a7
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
082a : c0 ab __ CPY #$ab
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
;  20, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
.s4:
;  21, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0880 : a9 98 __ LDA #$98
0882 : 8d a8 08 STA $08a8 ; (interruptWork + 0)
0885 : a9 08 __ LDA #$08
0887 : 8d a9 08 STA $08a9 ; (interruptWork + 1)
;  22, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
088a : a9 00 __ LDA #$00
088c : 8d aa 08 STA $08aa ; (result + 0)
;  23, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
088f : 20 9c 08 JSR $089c ; (work.s4 + 0)
;  24, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0892 : 20 9c 08 JSR $089c ; (work.s4 + 0)
.l5:
0895 : 4c 95 08 JMP $0895 ; (main.l5 + 0)
--------------------------------------------------------------------
tick: ; tick()->void
;   8, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
.s4:
;   9, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0898 : ee a7 08 INC $08a7 ; (ticks + 0)
.s3:
;  10, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
089b : 60 __ __ RTS
--------------------------------------------------------------------
work: ; work()->void
;  15, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
.s4:
;  16, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
089c : ee aa 08 INC $08aa ; (result + 0)
;  17, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
089f : ee aa 08 INC $08aa ; (result + 0)
08a2 : ee aa 08 INC $08aa ; (result + 0)
.s3:
;  18, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
08a5 : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
08a6 : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
ticks:
08a7 : __ __ __ BSS	1
--------------------------------------------------------------------
interruptWork:
08a8 : __ __ __ BSS	2
--------------------------------------------------------------------
result:
08aa : __ __ __ BSS	1
