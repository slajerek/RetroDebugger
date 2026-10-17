; Compiled with 1.32.273
--------------------------------------------------------------------
result:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  10, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
.s4:
;  11, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
00:8000 : a9 00 __ LDA #$00
00:8002 : 8d 00 02 STA $0200 ; (result + 0)
;  12, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
00:8005 : 20 14 80 JSR $8014 ; (a.s4 + 0)
00:8008 : 20 18 80 JSR $8018 ; (b.s4 + 0)
;  13, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
00:800b : 0e 00 02 ASL $0200 ; (result + 0)
00:800e : 20 14 80 JSR $8014 ; (a.s4 + 0)
.l5:
00:8011 : 4c 11 80 JMP $8011 ; (main.l5 + 0)
--------------------------------------------------------------------
a: ; a()->void
;   7, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
.s4:
;   7, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
00:8014 : ee 00 02 INC $0200 ; (result + 0)
.s3:
00:8017 : 60 __ __ RTS
--------------------------------------------------------------------
b: ; b()->void
;   8, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
.s4:
;   8, "/tmp/rd-source-debug/oscar64/nes/same-line/main.c"
00:8018 : ee 00 02 INC $0200 ; (result + 0)
00:801b : ee 00 02 INC $0200 ; (result + 0)
.s3:
00:801e : 60 __ __ RTS
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
00:ffac : a9 fe __ LDA #$fe
00:ffae : 85 2f __ STA SP + 0 
00:ffb0 : a9 07 __ LDA #$07
00:ffb2 : 85 30 __ STA SP + 1 
00:ffb4 : 20 00 80 JSR $8000 ; (main.s4 + 0)
00:ffb7 : 4c b7 ff JMP $ffb7 ; (startup + 55)
00:ffba : 60 __ __ RTS
