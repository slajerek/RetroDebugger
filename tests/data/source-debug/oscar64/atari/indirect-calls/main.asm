; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 d4 __ LDY #$d4
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
2019 : c0 d5 __ CPY #$d5
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
2032 : 20 80 20 JSR $2080 ; (main.s1 + 0)
2035 : 4c 35 20 JMP $2035 ; (startup + 53)
2038 : 60 __ __ RTS
--------------------------------------------------------------------
main: ; main()->i16
;  19, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
.s1:
2080 : 38 __ __ SEC
2081 : a5 a1 __ LDA SP + 0 
2083 : e9 03 __ SBC #$03
2085 : 85 a1 __ STA SP + 0 
2087 : b0 02 __ BCS $208b ; (main.s4 + 0)
2089 : c6 a2 __ DEC SP + 1 
.s4:
;  21, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
208b : a9 01 __ LDA #$01
208d : 8d d4 20 STA $20d4 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
2090 : a9 00 __ LDA #$00
2092 : 8d ff bb STA $bbff ; (i + 0)
.l5:
;  23, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
2095 : ad d4 20 LDA $20d4 ; (result + 0)
2098 : a0 02 __ LDY #$02
209a : 91 a1 __ STA (SP + 0),y 
209c : ad ff bb LDA $bbff ; (i + 0)
209f : 0a __ __ ASL
20a0 : aa __ __ TAX
20a1 : bd d0 20 LDA $20d0,x ; (operations[0] + 0)
20a4 : 85 99 __ STA ACCU + 0 
20a6 : bd d1 20 LDA $20d1,x ; (operations[0] + 1)
20a9 : 85 9a __ STA ACCU + 1 
20ab : 20 cd 20 JSR $20cd ; (bcexec + 0)
20ae : a5 99 __ LDA ACCU + 0 
20b0 : 8d d4 20 STA $20d4 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
20b3 : ee ff bb INC $bbff ; (i + 0)
20b6 : ad ff bb LDA $bbff ; (i + 0)
20b9 : c9 02 __ CMP #$02
20bb : 90 d8 __ BCC $2095 ; (main.l5 + 0)
.l6:
20bd : b0 fe __ BCS $20bd ; (main.l6 + 0)
--------------------------------------------------------------------
twice: ; twice(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
20bf : a0 02 __ LDY #$02
20c1 : b1 a1 __ LDA (SP + 0),y 
20c3 : 0a __ __ ASL
.s3:
20c4 : 60 __ __ RTS
--------------------------------------------------------------------
plusOne: ; plusOne(u8)->u8
;  11, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
.s4:
;  12, "/private/tmp/rd-source-debug/oscar64/atari/indirect-calls/main.c"
20c5 : a0 02 __ LDY #$02
20c7 : b1 a1 __ LDA (SP + 0),y 
20c9 : 18 __ __ CLC
20ca : 69 01 __ ADC #$01
.s3:
20cc : 60 __ __ RTS
--------------------------------------------------------------------
bcexec: ; bcexec
20cd : 6c 99 00 JMP ($0099)
--------------------------------------------------------------------
operations:
20d0 : __ __ __ BYT bf 20 c5 20                                     : . . 
--------------------------------------------------------------------
result:
20d4 : __ __ __ BSS	1
