; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 b3 __ LDY #$b3
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
2019 : c0 b4 __ CPY #$b4
201b : d0 f9 __ BNE $2016 ; (startup + 22)
201d : a9 00 __ LDA #$00
201f : a2 e0 __ LDX #$e0
2021 : d0 03 __ BNE $2026 ; (startup + 38)
2023 : 95 00 __ STA $00,x 
2025 : e8 __ __ INX
2026 : e0 e0 __ CPX #$e0
2028 : d0 f9 __ BNE $2023 ; (startup + 35)
202a : a9 fd __ LDA #$fd
202c : 85 a1 __ STA SP + 0 
202e : a9 bb __ LDA #$bb
2030 : 85 a2 __ STA SP + 1 
2032 : 20 80 20 JSR $2080 ; (main.s4 + 0)
2035 : 4c 35 20 JMP $2035 ; (startup + 53)
2038 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  21, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
.s4:
;  22, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
2080 : a9 01 __ LDA #$01
2082 : 85 8d __ STA P2 
2084 : 20 99 20 JSR $2099 ; (top.s4 + 0)
2087 : 8d b3 20 STA $20b3 ; (result + 0)
;  23, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
208a : ad b3 20 LDA $20b3 ; (result + 0)
208d : 20 af 20 JSR $20af ; (leaf.s4 + 0)
2090 : 20 af 20 JSR $20af ; (leaf.s4 + 0)
2093 : 8d b3 20 STA $20b3 ; (result + 0)
.l5:
2096 : 4c 96 20 JMP $2096 ; (main.l5 + 0)
--------------------------------------------------------------------
top: ; top(u8)->u8
;  17, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
.s4:
;  18, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
2099 : a5 8d __ LDA P2 ; (v + 0)
209b : 85 8c __ STA P1 
209d : 20 a4 20 JSR $20a4 ; (middle.s4 + 0)
20a0 : 18 __ __ CLC
20a1 : 69 01 __ ADC #$01
.s3:
20a3 : 60 __ __ RTS
--------------------------------------------------------------------
middle: ; middle(u8)->u8
;  12, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
.s4:
;  13, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
20a4 : a5 8c __ LDA P1 ; (v + 0)
20a6 : 20 af 20 JSR $20af ; (leaf.s4 + 0)
20a9 : 8d ff bb STA $bbff ; (w + 0)
;  14, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
20ac : 4c af 20 JMP $20af ; (leaf.s4 + 0)
--------------------------------------------------------------------
leaf: ; leaf(u8)->u8
;   8, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
.s4:
;   9, "/tmp/rd-source-debug/oscar64/atari/nested-calls/main.c"
20af : 18 __ __ CLC
20b0 : 69 01 __ ADC #$01
.s3:
20b2 : 60 __ __ RTS
--------------------------------------------------------------------
result:
20b3 : __ __ __ BSS	1
