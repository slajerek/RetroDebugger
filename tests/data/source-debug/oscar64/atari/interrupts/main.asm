; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 98 __ LDY #$98
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
2019 : c0 99 __ CPY #$99
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
;  17, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
.s4:
;  18, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2080 : a9 00 __ LDA #$00
2082 : 8d 98 20 STA $2098 ; (result + 0)
;  19, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2085 : 20 8e 20 JSR $208e ; (work.s4 + 0)
;  20, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2088 : 20 8e 20 JSR $208e ; (work.s4 + 0)
.l5:
208b : 4c 8b 20 JMP $208b ; (main.l5 + 0)
--------------------------------------------------------------------
work: ; work()->void
;  12, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
.s4:
;  13, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
208e : ee 98 20 INC $2098 ; (result + 0)
;  14, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2091 : ee 98 20 INC $2098 ; (result + 0)
2094 : ee 98 20 INC $2098 ; (result + 0)
.s3:
;  15, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2097 : 60 __ __ RTS
--------------------------------------------------------------------
result:
2098 : __ __ __ BSS	1
