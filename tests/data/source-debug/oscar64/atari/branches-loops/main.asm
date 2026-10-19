; Compiled with 1.32.273
--------------------------------------------------------------------
startup: ; startup
2000 : a2 20 __ LDX #$20
2002 : a0 cf __ LDY #$cf
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
2019 : c0 d0 __ CPY #$d0
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
;  19, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
.s4:
;  21, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
2080 : a9 00 __ LDA #$00
2082 : 8d cf 20 STA $20cf ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
2085 : 8d ff bb STA $bbff ; (i + 0)
;  23, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
2088 : 18 __ __ CLC
.l12:
2089 : ad ff bb LDA $bbff ; (i + 0)
208c : aa __ __ TAX
;  22, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
208d : e8 __ __ INX
208e : 8e ff bb STX $bbff ; (i + 0)
;  23, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
2091 : 6d cf 20 ADC $20cf ; (result + 0)
2094 : 8d cf 20 STA $20cf ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
2097 : e0 03 __ CPX #$03
2099 : 90 ee __ BCC $2089 ; (main.l12 + 0)
.l5:
;  24, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
209b : ad cf 20 LDA $20cf ; (result + 0)
209e : c9 02 __ CMP #$02
;  25, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20a0 : ad cf 20 LDA $20cf ; (result + 0)
;  24, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20a3 : 90 08 __ BCC $20ad ; (main.s7 + 0)
.s6:
;  25, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20a5 : e9 02 __ SBC #$02
20a7 : 8d cf 20 STA $20cf ; (result + 0)
20aa : 4c 9b 20 JMP $209b ; (main.l5 + 0)
.s7:
;  26, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20ad : c9 01 __ CMP #$01
20af : d0 04 __ BNE $20b5 ; (main.s10 + 0)
.s8:
;  27, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20b1 : a9 03 __ LDA #$03
20b3 : d0 02 __ BNE $20b7 ; (main.s11 + 0)
.s10:
;  29, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20b5 : a9 05 __ LDA #$05
.s11:
20b7 : 20 c0 20 JSR $20c0 ; (classify.s4 + 0)
20ba : 8d cf 20 STA $20cf ; (result + 0)
.l9:
20bd : 4c bd 20 JMP $20bd ; (main.l9 + 0)
--------------------------------------------------------------------
classify: ; classify(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20c0 : c9 03 __ CMP #$03
20c2 : b0 03 __ BCS $20c7 ; (classify.s6 + 0)
.s5:
;   9, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20c4 : a9 01 __ LDA #$01
.s3:
20c6 : 60 __ __ RTS
.s6:
;  10, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20c7 : d0 03 __ BNE $20cc ; (classify.s8 + 0)
.s7:
;  12, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20c9 : a9 02 __ LDA #$02
20cb : 60 __ __ RTS
.s8:
;  16, "/private/tmp/rd-source-debug/oscar64/atari/branches-loops/main.c"
20cc : a9 03 __ LDA #$03
20ce : 60 __ __ RTS
--------------------------------------------------------------------
result:
20cf : __ __ __ BSS	1
