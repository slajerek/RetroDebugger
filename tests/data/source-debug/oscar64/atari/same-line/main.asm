; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 9f __ LDY #$9f
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
2019 : c0 a0 __ CPY #$a0
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
2032 : 20 80 20 JSR $2080 ; (main.s4 + 0)
2035 : 4c 35 20 JMP $2035 ; (startup + 53)
2038 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  10, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
.s4:
;  11, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
2080 : a9 00 __ LDA #$00
2082 : 8d 9f 20 STA $209f ; (result + 0)
;  12, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
2085 : 20 94 20 JSR $2094 ; (a.s4 + 0)
2088 : 20 98 20 JSR $2098 ; (b.s4 + 0)
;  13, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
208b : 0e 9f 20 ASL $209f ; (result + 0)
208e : 20 94 20 JSR $2094 ; (a.s4 + 0)
.l5:
2091 : 4c 91 20 JMP $2091 ; (main.l5 + 0)
--------------------------------------------------------------------
a: ; a()->void
;   7, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
.s4:
;   7, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
2094 : ee 9f 20 INC $209f ; (result + 0)
.s3:
2097 : 60 __ __ RTS
--------------------------------------------------------------------
b: ; b()->void
;   8, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
.s4:
;   8, "/tmp/rd-source-debug/oscar64/atari/same-line/main.c"
2098 : ee 9f 20 INC $209f ; (result + 0)
209b : ee 9f 20 INC $209f ; (result + 0)
.s3:
209e : 60 __ __ RTS
--------------------------------------------------------------------
result:
209f : __ __ __ BSS	1
