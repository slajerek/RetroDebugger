// banked, C64 Magic Desk cartridge (CRT type 19) in KickAssembler (RD-007):
// bankA and bankB assembled at the same CPU address, $9000, in the segments
// Bank0 and Bank1, 8 KB ROML banks 0 and 1. KickAssembler's .dbg names the
// segments but records no file offset, so which bytes of main.crt each
// segment is comes from the project's segment table, main.segments.json
// next to main.dbg (the debugger reads it with the .dbg). Bank 0 holds the cartridge
// header and the startup, which copies main to RAM: main selects a bank
// before each call. The same checks as magicdesk.c for cc65.

.const BANK_REG = $de00			// Magic Desk: the bank number (bit 7 switches the cartridge off)
.const result = $02

.segmentdef Bank0 [start=$8000, min=$8000, max=$9fff, fill]
.segmentdef Bank1 [start=$8000, min=$8000, max=$9fff, fill]

// the .crt file: its header, then per bank a CHIP packet header and the bank
.file [name="main.crt", type="bin", segments="Crt"]
.segmentdef Crt [start=$0000]
.segment Crt
	.text "C64 CARTRIDGE   "
	.byte $00, $00, $00, $40		// header length
	.byte $01, $00				// version 1.0
	.byte $00, $13				// hardware type 19: Magic Desk
	.byte $00				// EXROM active
	.byte $01				// GAME inactive: 8 KB mode
	.fill 6, 0
	.text "RD BANKED KICKASS"
	.fill 32 - 17, 0
	.text "CHIP"
	.byte $00, $00, $20, $10, $00, $00, $00, $00, $80, $00, $20, $00
	.segmentout [segments="Bank0"]
	.text "CHIP"
	.byte $00, $00, $20, $10, $00, $00, $00, $01, $80, $00, $20, $00
	.segmentout [segments="Bank1"]

.segment Bank0
	.word reset, reset
	.byte $c3, $c2, $cd, $38, $30		// CBM80
reset:
	sei
	cld
	ldx #$ff
	txs
	lda #$37				// the port's latch first, then its direction
	sta $01
	lda #$2f
	sta $00
	lda #$7f
	sta $dc0d				// CIA interrupts off
	sta $dd0d
	ldx #mainEnd - mainLoad
copy:
	lda mainLoad - 1,x
	sta main - 1,x
	dex
	bne copy
	jmp main

mainLoad:
.pseudopc $0900 {
main:
	lda #0
	sta result
	lda #0
	sta BANK_REG
	jsr bankA
	lda #1
	sta BANK_REG
	jsr bankB
	lda #0
	sta BANK_REG
	jsr bankA
forever:
	jmp forever				// stable debugger stop point
}
mainEnd:

	* = $9000 "bank 0 code"
bankA:
	inc result
	rts

.segment Bank1
	* = $9000 "bank 1 code"
bankB:
	lda result
	clc
	adc #10
	sta result
	rts
