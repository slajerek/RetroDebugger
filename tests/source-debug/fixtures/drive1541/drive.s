; drive1541: code for the 1541 drive CPU, in drive RAM at $0500 (buffer 2),
; for source steps on the drive while the C64 runs (RD-044). The test pokes
; it into drive RAM and sends the drive's PC to start. The drive ROM's own
; interrupt handler keeps running between any two instructions. delay is
; longer than VICE's drive catch-up slices, so the C64 clock moves over it.

        .export         start

        .segment        "CODE"
start:  ldx     #0
loop:   jsr     bump
        inx
        jsr     delay
        jmp     loop

bump:   lda     count
        clc
        adc     #1
        sta     count
        rts

delay:  lda     #64                     ; 64 x 256 x 5 cycles, about four frames
outer:  ldy     #0
wait:   dey
        bne     wait
        sec
        sbc     #1
        bne     outer
        rts

        .segment        "BSS"
count:  .res    1
