; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 a6 __ LDY #$a6
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
2019 : c0 aa __ CPY #$aa
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
;  20, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
.s4:
;  21, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2080 : a9 98 __ LDA #$98
2082 : 8d a7 20 STA $20a7 ; (interruptWork + 0)
2085 : a9 20 __ LDA #$20
2087 : 8d a8 20 STA $20a8 ; (interruptWork + 1)
;  22, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
208a : a9 00 __ LDA #$00
208c : 8d a9 20 STA $20a9 ; (result + 0)
;  23, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
208f : 20 9c 20 JSR $209c ; (work.s4 + 0)
;  24, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2092 : 20 9c 20 JSR $209c ; (work.s4 + 0)
.l5:
2095 : 4c 95 20 JMP $2095 ; (main.l5 + 0)
--------------------------------------------------------------------
tick: ; tick()->void
;   8, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
.s4:
;   9, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
2098 : ee a6 20 INC $20a6 ; (ticks + 0)
.s3:
;  10, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
209b : 60 __ __ RTS
--------------------------------------------------------------------
work: ; work()->void
;  15, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
.s4:
;  16, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
209c : ee a9 20 INC $20a9 ; (result + 0)
;  17, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
209f : ee a9 20 INC $20a9 ; (result + 0)
20a2 : ee a9 20 INC $20a9 ; (result + 0)
.s3:
;  18, "/tmp/rd-source-debug/oscar64/atari/interrupts/main.c"
20a5 : 60 __ __ RTS
--------------------------------------------------------------------
ticks:
20a6 : __ __ __ BSS	1
--------------------------------------------------------------------
interruptWork:
20a7 : __ __ __ BSS	2
--------------------------------------------------------------------
result:
20a9 : __ __ __ BSS	1
