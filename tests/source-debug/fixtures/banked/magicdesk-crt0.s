; Startup for the Magic Desk banked fixture: the CRT file header and one CHIP
; packet header per bank are linker segments, so the debug info's output
; offsets are offsets in the .crt file. Bank 0 holds the cartridge header
; (CBM80, the KERNAL's reset jumps through $8000) and this startup, which
; copies CODE and RODATA to RAM -- no bank is fixed -- and then runs the C
; runtime's zerobss and copydata from there. The C stack pointer, zerobss
; and copydata come from c64.lib.

        .export         __STARTUP__ : absolute = 1
        .import         _main, copydata, zerobss
        .import         __CODE_LOAD__, __CODE_RUN__, __CODE_SIZE__
        .import         __RODATA_LOAD__, __RODATA_RUN__, __RODATA_SIZE__
        .import         __STACK_START__, __STACK_SIZE__
        .importzp       sp, ptr1, ptr2, tmp1, tmp2

.segment "CRTHEADER"
        .byte   $43, $36, $34, $20, $43, $41, $52, $54
        .byte   $52, $49, $44, $47, $45, $20, $20, $20 ; "C64 CARTRIDGE   " in ASCII (strings would be PETSCII)
        .byte   $00, $00, $00, $40      ; header length
        .byte   $01, $00                ; version 1.0
        .byte   $00, $13                ; hardware type 19: Magic Desk
        .byte   $00                     ; EXROM active
        .byte   $01                     ; GAME inactive: 8 KB mode
        .byte   0, 0, 0, 0, 0, 0
        .byte   $52, $44, $20, $42, $41, $4E, $4B, $45, $44 ; "RD BANKED"
        .res    32 - 9, 0

.macro  chip bank
        .byte   $43, $48, $49, $50 ; "CHIP"
        .byte   $00, $00, $20, $10      ; packet length
        .byte   $00, $00                ; ROM
        .byte   $00, bank
        .byte   $80, $00                ; load address
        .byte   $20, $00                ; size
.endmacro

.segment "CHIP0"
        chip    0
.segment "CHIP1"
        chip    1

.segment "CARTHDR"
        .word   reset, reset
        .byte   $C3, $C2, $CD, $38, $30 ; CBM80

.segment "STARTUP"
reset:  sei
        cld
        ldx     #$FF
        txs
        lda     #$37                    ; the port's latch first, as the KERNAL does:
        sta     $01                     ; after reset it is 0, and with the direction
        lda     #$2F                    ; set first the RAM-only configuration would
        sta     $00                     ; unmap the cartridge under this code
        lda     #$7F
        sta     $DC0D                   ; CIA interrupts off
        sta     $DD0D
        lda     #<__CODE_LOAD__
        ldx     #>__CODE_LOAD__
        ldy     #<__CODE_RUN__
        sty     ptr2
        ldy     #>__CODE_RUN__
        sty     ptr2+1
        ldy     #<__CODE_SIZE__
        sty     tmp1
        ldy     #>__CODE_SIZE__
        sty     tmp2
        jsr     copy
        lda     #<__RODATA_LOAD__
        ldx     #>__RODATA_LOAD__
        ldy     #<__RODATA_RUN__
        sty     ptr2
        ldy     #>__RODATA_RUN__
        sty     ptr2+1
        ldy     #<__RODATA_SIZE__
        sty     tmp1
        ldy     #>__RODATA_SIZE__
        sty     tmp2
        jsr     copy
        lda     #<(__STACK_START__ + __STACK_SIZE__)
        sta     sp
        lda     #>(__STACK_START__ + __STACK_SIZE__)
        sta     sp+1
        jsr     zerobss
        jsr     copydata
        jsr     _main
forever:
        jmp     forever

; copy tmp1/tmp2 bytes from A/X to ptr2
copy:   sta     ptr1
        stx     ptr1+1
        ldy     #0
@loop:  lda     tmp1
        ora     tmp2
        beq     @done
        lda     (ptr1),y
        sta     (ptr2),y
        inc     ptr1
        bne     @src
        inc     ptr1+1
@src:   inc     ptr2
        bne     @dst
        inc     ptr2+1
@dst:   lda     tmp1
        bne     @low
        dec     tmp2
@low:   dec     tmp1
        jmp     @loop
@done:  rts
