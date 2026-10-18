; Compiled with 1.32.273
--------------------------------------------------------------------
result:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  13, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
.s1:
00:8000 : 38 __ __ SEC
00:8001 : a5 2f __ LDA SP + 0 
00:8003 : e9 03 __ SBC #$03
00:8005 : 85 2f __ STA SP + 0 
00:8007 : b0 02 __ BCS $800b ; (main.s4 + 0)
00:8009 : c6 30 __ DEC SP + 1 
.s4:
;  14, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
00:800b : a9 03 __ LDA #$03
00:800d : a0 02 __ LDY #$02
00:800f : 91 2f __ STA (SP + 0),y 
00:8011 : 20 1c 80 JSR $801c ; (sum.s1 + 0)
00:8014 : a5 27 __ LDA ACCU + 0 
00:8016 : 8d 00 02 STA $0200 ; (result + 0)
.l5:
00:8019 : 4c 19 80 JMP $8019 ; (main.l5 + 0)
--------------------------------------------------------------------
sum: ; sum(u8)->u8
;   7, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
.s1:
00:801c : 38 __ __ SEC
00:801d : a5 2f __ LDA SP + 0 
00:801f : e9 05 __ SBC #$05
00:8021 : 85 2f __ STA SP + 0 
00:8023 : b0 02 __ BCS $8027 ; (sum.s1 + 11)
00:8025 : c6 30 __ DEC SP + 1 
00:8027 : a0 03 __ LDY #$03
00:8029 : a5 53 __ LDA T1 + 0 
00:802b : 91 2f __ STA (SP + 0),y 
.s4:
;   8, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
00:802d : a0 07 __ LDY #$07
00:802f : b1 2f __ LDA (SP + 0),y 
00:8031 : f0 11 __ BEQ $8044 ; (sum.s6 + 0)
.s5:
00:8033 : 85 53 __ STA T1 + 0 
;  10, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
00:8035 : 38 __ __ SEC
00:8036 : e9 01 __ SBC #$01
00:8038 : a0 02 __ LDY #$02
00:803a : 91 2f __ STA (SP + 0),y 
00:803c : 20 1c 80 JSR $801c ; (sum.s1 + 0)
00:803f : 18 __ __ CLC
00:8040 : a5 27 __ LDA ACCU + 0 
00:8042 : 65 53 __ ADC T1 + 0 
.s6:
;   9, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
00:8044 : 85 27 __ STA ACCU + 0 
.s3:
;  10, "/private/tmp/rd-source-debug/oscar64/nes/recursion/main.c"
00:8046 : a0 03 __ LDY #$03
00:8048 : b1 2f __ LDA (SP + 0),y 
00:804a : 85 53 __ STA T1 + 0 
00:804c : 18 __ __ CLC
00:804d : a5 2f __ LDA SP + 0 
00:804f : 69 05 __ ADC #$05
00:8051 : 85 2f __ STA SP + 0 
00:8053 : 90 02 __ BCC $8057 ; (sum.s3 + 17)
00:8055 : e6 30 __ INC SP + 1 
00:8057 : 60 __ __ RTS
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
00:ffb4 : 20 00 80 JSR $8000 ; (main.s1 + 0)
00:ffb7 : 4c b7 ff JMP $ffb7 ; (startup + 55)
00:ffba : 60 __ __ RTS
