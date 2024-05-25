; get_crunched_byte routine for RetroDebugger's CrtMaker plugin

; -------------------------------------------------------------------
; this file is intended to be assembled and linked with the cc65 toolchain.
; It has not been tested with any other assemblers or linkers.
; -------------------------------------------------------------------
.import decrunch
.export get_crunched_byte
.export decrunch_table
.import end_of_data

; -------------------------------------------------------------------
; we begin here
; -------------------------------------------------------------------
	stx data_addr+1
	sty data_addr+2
	
	sta $DE00
	sta bank+1
	
	jmp decrunch


get_crunched_byte:
		lda #$37	; data from CART
		sta $01 
data_addr:	lda $1000
		sta $02		; getByteData+1
	        inc data_addr+1 
		bne ok 
		
		php
		inc data_addr+2		 
		lda data_addr+2
		cmp #$A0
		beq nextBank
		
getByte:	plp
ok: 		lda #$34
		sta $01
getByteData:	lda $02		; #$00

		rts 
nextBank:	
		lda #$00
		sta data_addr+1
		lda #$80
		sta data_addr+2
		inc bank+1
bank:		lda #$00
		sta $DE00
		jmp getByte


		
decrunch_table: 
;.byte 0

;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;.IF EXTRA_TABLE_ENTRY_FOR_LENGTH_THREE <> 0
;;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;;.ENDIF
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;        .byte 0,0,0,0,0,0,0,0,0,0,0,0

;	.byte $aa		