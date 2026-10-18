; Startup for the MMC3 banked fixture: an iNES header for MMC3 (mapper 4,
; 32 KB PRG in four 8 KB banks, 8 KB CHR RAM) and a reset handler in the
; fixed last bank. Banks 2 and 3 are fixed at $C000 and $E000 in PRG mode 0
; ($8000 bank select bit 6 clear). The C runtime pieces (the C stack
; pointer, copydata, zerobss) come from nes.lib.

        .export         __STARTUP__ : absolute = 1
        .import         _main, copydata, zerobss
        .import         __SRAM_START__, __SRAM_SIZE__
        .importzp       sp

.segment "HEADER"
        .byte   $4E, $45, $53, $1A      ; "NES" EOF
        .byte   2                       ; PRG-ROM: 2 x 16 KB (four 8 KB banks)
        .byte   0                       ; CHR: RAM
        .byte   $41                     ; mapper 4 (low nibble), vertical mirroring
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
        sta     $E000                   ; MMC3 IRQ off
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
