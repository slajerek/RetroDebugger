; Startup for the XEGS banked fixture: the .car header (type 12, XEGS 32 KB;
; car_checksum.py writes its checksum after the link) is a linker segment,
; so the debug info's output offsets are offsets in the .car file. The
; cartridge vectors at the end of the fixed bank make the OS run this
; startup after its own initialization. The C stack pointer, zerobss and
; copydata come from atari.lib.

        .export         __STARTUP__ : absolute = 1
        .import         _main, copydata, zerobss
        .import         __STACK_START__, __STACK_SIZE__
        .importzp       sp

.segment "CARHEADER"
        .byte   $43, $41, $52, $54      ; "CART" in ASCII (strings would be ATASCII)
        .byte   $00, $00, $00, $0C      ; type 12: XEGS 32 KB
        .byte   $00, $00, $00, $00      ; checksum, written by car_checksum.py
        .byte   $00, $00, $00, $00

.segment "STARTUP"
start:  cld
        lda     #<(__STACK_START__ + __STACK_SIZE__)
        sta     sp
        lda     #>(__STACK_START__ + __STACK_SIZE__)
        sta     sp+1
        jsr     zerobss
        jsr     copydata
        jsr     _main
forever:
        jmp     forever

init:   rts

.segment "CARTVECTORS"
        .word   start                   ; $BFFA: run address
        .byte   $00                     ; $BFFC: a cartridge is present
        .byte   $04                     ; $BFFD: run it, no disk boot
        .word   init                    ; $BFFE: init address
