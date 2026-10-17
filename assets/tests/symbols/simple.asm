; RD-038 fixture: source text for the KickAss .dbg loader test.
		* = $1000
start:	lda #$01
	sta $d020
loop:	jmp loop
