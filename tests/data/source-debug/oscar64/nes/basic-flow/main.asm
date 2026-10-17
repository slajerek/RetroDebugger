; Compiled with 1.32.273
--------------------------------------------------------------------
result:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  15, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
.s4:
;  16, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:8000 : a9 01 __ LDA #$01
00:8002 : 8d 00 02 STA $0200 ; (result + 0)
;  17, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:8005 : 20 1a 80 JSR $801a ; (doSomething.s4 + 0)
;  18, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:8008 : ad 00 02 LDA $0200 ; (result + 0)
00:800b : 85 0d __ STA P0 
00:800d : a9 02 __ LDA #$02
00:800f : 85 0e __ STA P1 
00:8011 : 20 2a 80 JSR $802a ; (add.s4 + 0)
00:8014 : 8d 00 02 STA $0200 ; (result + 0)
.l5:
00:8017 : 4c 17 80 JMP $8017 ; (main.l5 + 0)
--------------------------------------------------------------------
doSomething: ; doSomething()->void
;  11, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
.s4:
;  12, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:801a : ad 00 02 LDA $0200 ; (result + 0)
00:801d : 85 0d __ STA P0 
00:801f : a9 03 __ LDA #$03
00:8021 : 85 0e __ STA P1 
00:8023 : 20 2a 80 JSR $802a ; (add.s4 + 0)
00:8026 : 8d 00 02 STA $0200 ; (result + 0)
.s3:
;  13, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:8029 : 60 __ __ RTS
--------------------------------------------------------------------
add: ; add(u8,u8)->u8
;   7, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
.s4:
;   8, "/tmp/rd-source-debug/oscar64/nes/basic-flow/main.c"
00:802a : 18 __ __ CLC
00:802b : a5 0e __ LDA P1 ; (right + 0)
00:802d : 65 0d __ ADC P0 ; (left + 0)
.s3:
00:802f : 60 __ __ RTS
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
