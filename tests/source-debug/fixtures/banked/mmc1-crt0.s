; Startup for the MMC1 banked fixture: an iNES header for MMC1 (mapper 1,
; four 16 KB PRG banks, 8 KB CHR RAM) and a reset handler in the fixed last
; bank. The reset writes $80 to the mapper, which clears its shift register
; and selects PRG mode 3 (switchable $8000, last bank fixed at $C000). The C
; runtime pieces (the C stack pointer, copydata, zerobss) come from nes.lib.

        .export         __STARTUP__ : absolute = 1
        .import         _main, copydata, zerobss
        .import         __SRAM_START__, __SRAM_SIZE__
        .importzp       sp

.segment "HEADER"
        .byte   $4E, $45, $53, $1A      ; "NES" EOF
        .byte   4                       ; PRG-ROM: 4 x 16 KB
        .byte   0                       ; CHR: RAM
        .byte   $11                     ; mapper 1 (low nibble), vertical mirroring
        .byte   $00                     ; mapper high nibble
        .byte   0, 0, 0, 0, 0, 0, 0, 0

.segment "STARTUP"
reset:  sei
        cld
        ldx     #$FF
        txs
        lda     #$80
        sta     $8000                   ; MMC1 reset: PRG mode 3
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
