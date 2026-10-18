/* nested-calls: three levels of calls, a call used as another call's
   argument, and the same function called twice from one line. Step Into
   walks down one level at a time, Step Out climbs one level at a time, and
   Step Over on a line runs the whole chain below it. */
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
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:6
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
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:22
    jsr top
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:22
    sta.z result
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:23
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:23
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:23
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:23
    sta.z result
  __b1:
    jmp __b1
}
// __register(A) char top(char v)
top: {
    .label v = 1
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:18
    jsr middle
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:18
    clc
    adc #1
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:19
    rts
}
// __register(A) char leaf(__register(A) char v)
leaf: {
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:9
    clc
    adc #1
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:10
    rts
}
// __register(A) char middle(char v)
middle: {
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:13
    lda #top.v
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:13
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:14
    jsr leaf
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:14
    // /private/tmp/rd-source-debug/kickc/nes/nested-calls/main.c:15
    rts
}
.segment Vectors
  VECTORS: .word 0, main, 0
