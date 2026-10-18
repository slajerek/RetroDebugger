; Compiled with 1.32.273
--------------------------------------------------------------------
result:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  19, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
.s4:
;  21, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8000 : a9 00 __ LDA #$00
00:8002 : 8d 00 02 STA $0200 ; (result + 0)
;  22, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8005 : 8d ff 07 STA $07ff ; (i + 0)
;  23, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8008 : 18 __ __ CLC
.l12:
00:8009 : ad ff 07 LDA $07ff ; (i + 0)
00:800c : aa __ __ TAX
;  22, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:800d : e8 __ __ INX
00:800e : 8e ff 07 STX $07ff ; (i + 0)
;  23, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8011 : 6d 00 02 ADC $0200 ; (result + 0)
00:8014 : 8d 00 02 STA $0200 ; (result + 0)
;  22, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8017 : e0 03 __ CPX #$03
00:8019 : 90 ee __ BCC $8009 ; (main.l12 + 0)
.l5:
;  24, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:801b : ad 00 02 LDA $0200 ; (result + 0)
00:801e : c9 02 __ CMP #$02
;  25, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8020 : ad 00 02 LDA $0200 ; (result + 0)
;  24, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8023 : 90 08 __ BCC $802d ; (main.s6 + 0)
.s10:
;  25, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8025 : e9 02 __ SBC #$02
00:8027 : 8d 00 02 STA $0200 ; (result + 0)
00:802a : 4c 1b 80 JMP $801b ; (main.l5 + 0)
.s6:
;  26, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:802d : c9 01 __ CMP #$01
00:802f : d0 04 __ BNE $8035 ; (main.s7 + 0)
.s9:
;  27, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8031 : a9 03 __ LDA #$03
00:8033 : d0 02 __ BNE $8037 ; (main.s11 + 0)
.s7:
;  29, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8035 : a9 05 __ LDA #$05
.s11:
00:8037 : 20 40 80 JSR $8040 ; (classify.s4 + 0)
00:803a : 8d 00 02 STA $0200 ; (result + 0)
.l8:
00:803d : 4c 3d 80 JMP $803d ; (main.l8 + 0)
--------------------------------------------------------------------
classify: ; classify(u8)->u8
;   7, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
.s4:
;   8, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8040 : c9 03 __ CMP #$03
00:8042 : b0 03 __ BCS $8047 ; (classify.s5 + 0)
.s8:
;   9, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8044 : a9 01 __ LDA #$01
.s3:
;  16, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8046 : 60 __ __ RTS
.s5:
;  10, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8047 : d0 03 __ BNE $804c ; (classify.s6 + 0)
.s7:
;  12, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:8049 : a9 02 __ LDA #$02
00:804b : 60 __ __ RTS
.s6:
;  16, "/tmp/rd-source-debug/oscar64/nes/branches-loops/main.c"
00:804c : a9 03 __ LDA #$03
00:804e : 60 __ __ RTS
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
00:ffb4 : 20 00 80 JSR $8000 ; (main.s4 + 0)
00:ffb7 : 4c b7 ff JMP $ffb7 ; (startup + 55)
00:ffba : 60 __ __ RTS
