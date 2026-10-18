; Startup for the banked fixture: an iNES header for UxROM (mapper 2, four
; 16 KB PRG banks, 8 KB CHR RAM) and a reset handler in the fixed bank. It
; replaces nes.lib's crt0, whose header is NROM; the C runtime pieces (the C
; stack pointer, copydata, zerobss) still come from nes.lib.

        .export         __STARTUP__ : absolute = 1
        .import         _main, copydata, zerobss
        .import         __SRAM_START__, __SRAM_SIZE__
        .importzp       sp

.segment "HEADER"
        .byte   $4E, $45, $53, $1A      ; "NES" EOF
        .byte   4                       ; PRG-ROM: 4 x 16 KB
        .byte   0                       ; CHR: RAM
        .byte   $21                     ; mapper 2 (low nibble), vertical mirroring
        .byte   $00                     ; mapper high nibble
        .byte   0, 0, 0, 0, 0, 0, 0, 0

.segment "STARTUP"
reset:  sei
        cld
        ldx     #$FF
        txs
        lda     #$00
        sta     $2000                   ; no NMI
        sta     $2001                   ; rendering off
        lda     #<(__SRAM_START__ + __SRAM_SIZE__)
        sta     sp
        lda     #>(__SRAM_START__ + __SRAM_SIZE__)
        sta     sp+1
        jsr     zerobss
        jsr     copydata
        jsr     _main
forever:
        jmp     forever

nmi:
irq:    rti

.segment "VECTORS"
        .word   nmi, reset, irq
