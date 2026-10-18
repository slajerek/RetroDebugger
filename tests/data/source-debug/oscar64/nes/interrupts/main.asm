; Compiled with 1.32.273
--------------------------------------------------------------------
ticks:
0200 : __ __ __ BSS	1
--------------------------------------------------------------------
interruptWork:
0201 : __ __ __ BSS	2
--------------------------------------------------------------------
result:
0203 : __ __ __ BSS	1
--------------------------------------------------------------------
main: ; main()->i16
;  20, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
.s4:
;  21, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:8000 : a9 18 __ LDA #$18
00:8002 : 8d 01 02 STA $0201 ; (interruptWork + 0)
00:8005 : a9 80 __ LDA #$80
00:8007 : 8d 02 02 STA $0202 ; (interruptWork + 1)
;  22, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:800a : a9 00 __ LDA #$00
00:800c : 8d 03 02 STA $0203 ; (result + 0)
;  23, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:800f : 20 1c 80 JSR $801c ; (work.s4 + 0)
;  24, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:8012 : 20 1c 80 JSR $801c ; (work.s4 + 0)
.l5:
00:8015 : 4c 15 80 JMP $8015 ; (main.l5 + 0)
--------------------------------------------------------------------
tick: ; tick()->void
;   8, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
.s4:
;   9, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:8018 : ee 00 02 INC $0200 ; (ticks + 0)
.s3:
;  10, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:801b : 60 __ __ RTS
--------------------------------------------------------------------
work: ; work()->void
;  15, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
.s4:
;  16, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:801c : ee 03 02 INC $0203 ; (result + 0)
;  17, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:801f : ee 03 02 INC $0203 ; (result + 0)
00:8022 : ee 03 02 INC $0203 ; (result + 0)
.s3:
;  18, "/tmp/rd-source-debug/oscar64/nes/interrupts/main.c"
00:8025 : 60 __ __ RTS
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
00:ff9b : c0 04 __ CPY #$04
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
