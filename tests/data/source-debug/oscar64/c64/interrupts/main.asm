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
080e : 8e 98 08 STX $0898 ; (spentry + 0)
0811 : a2 08 __ LDX #$08
0813 : a0 99 __ LDY #$99
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
082a : c0 9a __ CPY #$9a
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
;  17, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
.s4:
;  18, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0880 : a9 00 __ LDA #$00
0882 : 8d 99 08 STA $0899 ; (result + 0)
;  19, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0885 : 20 8e 08 JSR $088e ; (work.s4 + 0)
;  20, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0888 : 20 8e 08 JSR $088e ; (work.s4 + 0)
.l5:
088b : 4c 8b 08 JMP $088b ; (main.l5 + 0)
--------------------------------------------------------------------
work: ; work()->void
;  12, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
.s4:
;  13, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
088e : ee 99 08 INC $0899 ; (result + 0)
;  14, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0891 : ee 99 08 INC $0899 ; (result + 0)
0894 : ee 99 08 INC $0899 ; (result + 0)
.s3:
;  15, "/tmp/rd-source-debug/oscar64/c64/interrupts/main.c"
0897 : 60 __ __ RTS
--------------------------------------------------------------------
spentry:
0898 : __ __ __ BYT 00                                              : .
--------------------------------------------------------------------
result:
0899 : __ __ __ BSS	1
