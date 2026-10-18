/* same-line: several statements and calls on one source line. A source step
   must stop at each statement in column order, and Step Over on "a(); b();"
   must not stop inside a() or b(). */
.cpu _6502
  // Nintendo Entertainment System (NES) ROM (Mapper 0 NROM, Vertical Mirroring)
// https://sadistech.com/nesromtool/romdoc.html
// https://forums.nesdev.com/viewtopic.php?f=2&t=9896
// https://github.com/gregkrsak/first_nes
.file [name="main.nes", type="bin", segments="NesRom"]
.file [name="main.nes_hdr", type="bin", segments="Header"]
.file [name="main.nes_prg", type="bin", segments="ProgramRom"]
.file [name="main.nes_chr", type="bin", segments="CharacterRom"]
.segmentdef Header  [ start=$0000,       min=$0000, max=$000f, fill ]
.segmentdef Tiles   [ start=$0000,       min=$0000, max=$1fff, fill ]
.segmentdef Code    [ start=$c000,       min=$c000, max=$fff9 ]
.segmentdef Data    [ startAfter="Code", min=$c000, max=$fff9 ]
.segmentdef Vectors [ start=$fffa,       min=$fffa, max=$ffff ]
.segmentdef GameRam [start=$200,max=$7ff, virtual]
.segmentdef ProgramRom [ segments="Code, Data, Vectors" ]
.segmentdef CharacterRom [ segments="Tiles" ]
.segmentdef NesRom
.segment NesRom
.segmentout [ segments="Header" ]
.segmentout [ segments="ProgramRom" ]
.segmentout [ segments="CharacterRom" ]
.segment Header
.text   @"NES\$1a"
.byte   $01           // 1x 16KB ROM (PRG)
.byte   $01           // 1x 8KB VROM (CHR)
.byte   %00000001     // Mapper nibble 0000 == No mapping (a simple 16KB PRG + 8KB CHR game)
                      // Mirroring nibble 0001 == Vertical mirroring only
  .const STACK_BASE = $103
  .label result = 2
.segment Code
__start: {
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:5
    lda #0
    sta.z result
    pha
    pha
    jsr main
    pla
    pla
    rts
}
main: {
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:11
    lda #0
    sta.z result
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:12
    jsr a
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:12
    jsr b
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:13
    lda.z result
    asl
    sta.z result
    jsr a
  __b1:
    jmp __b1
}
a: {
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:7
    ldx.z result
    inx
    stx.z result
    rts
}
b: {
    // /private/tmp/rd-source-debug/kickc/nes/same-line/main.c:8
    lda.z result
    clc
    adc #2
    sta.z result
    rts
}
.segment Vectors
  VECTORS: .word 0, main, 0
