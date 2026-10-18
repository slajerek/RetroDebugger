; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 d8 __ LDY #$d8
2004 : a9 00 __ LDA #$00
2006 : 85 97 __ STA IP + 0 
2008 : 86 98 __ STX IP + 1 
200a : e0 20 __ CPX #$20
200c : f0 0b __ BEQ $2019 ; (startup + 25)
200e : 91 97 __ STA (IP + 0),y 
2010 : c8 __ __ INY
2011 : d0 fb __ BNE $200e ; (startup + 14)
2013 : e8 __ __ INX
2014 : d0 f2 __ BNE $2008 ; (startup + 8)
2016 : 91 97 __ STA (IP + 0),y 
2018 : c8 __ __ INY
2019 : c0 d9 __ CPY #$d9
201b : d0 f9 __ BNE $2016 ; (startup + 22)
201d : a9 00 __ LDA #$00
201f : a2 e0 __ LDX #$e0
2021 : d0 03 __ BNE $2026 ; (startup + 38)
2023 : 95 00 __ STA $00,x 
2025 : e8 __ __ INX
2026 : e0 e0 __ CPX #$e0
2028 : d0 f9 __ BNE $2023 ; (startup + 35)
202a : a9 fe __ LDA #$fe
202c : 85 a1 __ STA SP + 0 
202e : a9 bb __ LDA #$bb
2030 : 85 a2 __ STA SP + 1 
2032 : 20 80 20 JSR $2080 ; (main.s1 + 0)
2035 : 4c 35 20 JMP $2035 ; (startup + 53)
2038 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  13, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
.s1:
2080 : 38 __ __ SEC
2081 : a5 a1 __ LDA SP + 0 
2083 : e9 03 __ SBC #$03
2085 : 85 a1 __ STA SP + 0 
2087 : b0 02 __ BCS $208b ; (main.s4 + 0)
2089 : c6 a2 __ DEC SP + 1 
.s4:
;  14, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
208b : a9 03 __ LDA #$03
208d : a0 02 __ LDY #$02
208f : 91 a1 __ STA (SP + 0),y 
2091 : 20 9c 20 JSR $209c ; (sum.s1 + 0)
2094 : a5 99 __ LDA ACCU + 0 
2096 : 8d d8 20 STA $20d8 ; (result + 0)
.l5:
2099 : 4c 99 20 JMP $2099 ; (main.l5 + 0)
--------------------------------------------------------------------
sum: ; sum(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
.s1:
209c : 38 __ __ SEC
209d : a5 a1 __ LDA SP + 0 
209f : e9 05 __ SBC #$05
20a1 : 85 a1 __ STA SP + 0 
20a3 : b0 02 __ BCS $20a7 ; (sum.s1 + 11)
20a5 : c6 a2 __ DEC SP + 1 
20a7 : a0 03 __ LDY #$03
20a9 : a5 c5 __ LDA T1 + 0 
20ab : 91 a1 __ STA (SP + 0),y 
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
20ad : a0 07 __ LDY #$07
20af : b1 a1 __ LDA (SP + 0),y 
20b1 : f0 11 __ BEQ $20c4 ; (sum.s6 + 0)
.s5:
20b3 : 85 c5 __ STA T1 + 0 
;  10, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
20b5 : 38 __ __ SEC
20b6 : e9 01 __ SBC #$01
20b8 : a0 02 __ LDY #$02
20ba : 91 a1 __ STA (SP + 0),y 
20bc : 20 9c 20 JSR $209c ; (sum.s1 + 0)
20bf : 18 __ __ CLC
20c0 : a5 99 __ LDA ACCU + 0 
20c2 : 65 c5 __ ADC T1 + 0 
.s6:
;   9, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
20c4 : 85 99 __ STA ACCU + 0 
.s3:
;  10, "/private/tmp/rd-source-debug/oscar64/atari/recursion/main.c"
20c6 : a0 03 __ LDY #$03
20c8 : b1 a1 __ LDA (SP + 0),y 
20ca : 85 c5 __ STA T1 + 0 
20cc : 18 __ __ CLC
20cd : a5 a1 __ LDA SP + 0 
20cf : 69 05 __ ADC #$05
20d1 : 85 a1 __ STA SP + 0 
20d3 : 90 02 __ BCC $20d7 ; (sum.s3 + 17)
20d5 : e6 a2 __ INC SP + 1 
20d7 : 60 __ __ RTS
--------------------------------------------------------------------
result:
20d8 : __ __ __ BSS	1
