; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 b0 __ LDY #$b0
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
2019 : c0 b1 __ CPY #$b1
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
;  15, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
.s4:
;  16, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
2080 : a9 01 __ LDA #$01
2082 : 8d b0 20 STA $20b0 ; (result + 0)
;  17, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
2085 : 20 9a 20 JSR $209a ; (doSomething.s4 + 0)
;  18, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
2088 : ad b0 20 LDA $20b0 ; (result + 0)
208b : 85 8b __ STA P0 
208d : a9 02 __ LDA #$02
208f : 85 8c __ STA P1 
2091 : 20 aa 20 JSR $20aa ; (add.s4 + 0)
2094 : 8d b0 20 STA $20b0 ; (result + 0)
.l5:
2097 : 4c 97 20 JMP $2097 ; (main.l5 + 0)
--------------------------------------------------------------------
doSomething: ; doSomething()->void
;  11, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
.s4:
;  12, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
209a : ad b0 20 LDA $20b0 ; (result + 0)
209d : 85 8b __ STA P0 
209f : a9 03 __ LDA #$03
20a1 : 85 8c __ STA P1 
20a3 : 20 aa 20 JSR $20aa ; (add.s4 + 0)
20a6 : 8d b0 20 STA $20b0 ; (result + 0)
.s3:
;  13, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
20a9 : 60 __ __ RTS
--------------------------------------------------------------------
add: ; add(u8,u8)->u8
;   7, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
.s4:
;   8, "/tmp/rd-source-debug/oscar64/atari/basic-flow/main.c"
20aa : 18 __ __ CLC
20ab : a5 8c __ LDA P1 ; (right + 0)
20ad : 65 8b __ ADC P0 ; (left + 0)
.s3:
20af : 60 __ __ RTS
--------------------------------------------------------------------
result:
20b0 : __ __ __ BSS	1
