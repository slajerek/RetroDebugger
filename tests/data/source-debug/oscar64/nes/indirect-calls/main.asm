; Compiled with 1.32.273
--------------------------------------------------------------------
result:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  19, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
.s1:
00:8000 : 38 __ __ SEC
00:8001 : a5 2f __ LDA SP + 0 
00:8003 : e9 03 __ SBC #$03
00:8005 : 85 2f __ STA SP + 0 
00:8007 : b0 02 __ BCS $800b ; (main.s4 + 0)
00:8009 : c6 30 __ DEC SP + 1 
.s4:
;  21, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:800b : a9 01 __ LDA #$01
00:800d : 8d 00 02 STA $0200 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:8010 : a9 00 __ LDA #$00
00:8012 : 8d ff 07 STA $07ff ; (i + 0)
.l5:
;  23, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:8015 : ad 00 02 LDA $0200 ; (result + 0)
00:8018 : a0 02 __ LDY #$02
00:801a : 91 2f __ STA (SP + 0),y 
00:801c : ad ff 07 LDA $07ff ; (i + 0)
00:801f : 0a __ __ ASL
00:8020 : aa __ __ TAX
00:8021 : bd 50 80 LDA $8050,x ; (operations[0] + 0)
00:8024 : 85 27 __ STA ACCU + 0 
00:8026 : bd 51 80 LDA $8051,x ; (operations[0] + 1)
00:8029 : 85 28 __ STA ACCU + 1 
00:802b : 20 4d 80 JSR $804d ; (bcexec + 0)
00:802e : a5 27 __ LDA ACCU + 0 
00:8030 : 8d 00 02 STA $0200 ; (result + 0)
;  22, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:8033 : ee ff 07 INC $07ff ; (i + 0)
00:8036 : ad ff 07 LDA $07ff ; (i + 0)
00:8039 : c9 02 __ CMP #$02
00:803b : 90 d8 __ BCC $8015 ; (main.l5 + 0)
.l6:
00:803d : b0 fe __ BCS $803d ; (main.l6 + 0)
--------------------------------------------------------------------
twice: ; twice(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:803f : a0 02 __ LDY #$02
00:8041 : b1 2f __ LDA (SP + 0),y 
00:8043 : 0a __ __ ASL
.s3:
00:8044 : 60 __ __ RTS
--------------------------------------------------------------------
plusOne: ; plusOne(u8)->u8
;  11, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
.s4:
;  12, "/private/tmp/rd-source-debug/oscar64/nes/indirect-calls/main.c"
00:8045 : a0 02 __ LDY #$02
00:8047 : b1 2f __ LDA (SP + 0),y 
00:8049 : 18 __ __ CLC
00:804a : 69 01 __ ADC #$01
.s3:
00:804c : 60 __ __ RTS
--------------------------------------------------------------------
bcexec: ; bcexec
00:804d : 6c 27 00 JMP ($0027)
--------------------------------------------------------------------
operations:
00:8050 : __ __ __ BYT 3f 80 45 80                                     : ?.E.
--------------------------------------------------------------------
startup: ; startup
00:ff80 : 78 __ __ SEI
00:ff81 : d8 __ __ CLD
00:ff82 : a2 02 __ LDX #$02
00:ff84 : a0 00 __ LDY #$00
00:ff86 : a9 00 __ LDA #$00
00:ff88 : 85 25 __ STA IP + 0 
00:ff8a : 86 26 __ STX IP + 1 
00:ff8c : e0 02 __ CPX #$02
00:ff8e : f0 0b __ BEQ $ff9b ; (startup + 27)
00:ff90 : 91 25 __ STA (IP + 0),y 
00:ff92 : c8 __ __ INY
00:ff93 : d0 fb __ BNE $ff90 ; (startup + 16)
00:ff95 : e8 __ __ INX
00:ff96 : d0 f2 __ BNE $ff8a ; (startup + 10)
00:ff98 : 91 25 __ STA (IP + 0),y 
00:ff9a : c8 __ __ INY
00:ff9b : c0 01 __ CPY #$01
00:ff9d : d0 f9 __ BNE $ff98 ; (startup + 24)
00:ff9f : a9 00 __ LDA #$00
00:ffa1 : a2 80 __ LDX #$80
00:ffa3 : d0 03 __ BNE $ffa8 ; (startup + 40)
00:ffa5 : 95 00 __ STA $00,x 
00:ffa7 : e8 __ __ INX
00:ffa8 : e0 80 __ CPX #$80
00:ffaa : d0 f9 __ BNE $ffa5 ; (startup + 37)
00:ffac : a9 fd __ LDA #$fd
00:ffae : 85 2f __ STA SP + 0 
00:ffb0 : a9 07 __ LDA #$07
00:ffb2 : 85 30 __ STA SP + 1 
00:ffb4 : 20 00 80 JSR $8000 ; (main.s1 + 0)
00:ffb7 : 4c b7 ff JMP $ffb7 ; (startup + 55)
00:ffba : 60 __ __ RTS
