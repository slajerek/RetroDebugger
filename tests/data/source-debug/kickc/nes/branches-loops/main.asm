/* branches-loops: if/else, for, while, switch and an early return. Step Over
   follows the taken path line by line and comes back to the loop head on
   every iteration. */
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
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:5
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
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:21
    lda #0
    sta.z result
    tax
  __b2:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:22
    cpx #3
    bcc __b3
  __b4:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:24
    lda.z result
    cmp #1+1
    bcs __b5
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:26
    lda #1
    cmp.z result
    beq __b1
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:29
    lda #5
    jsr classify
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:29
    sta.z result
  __b8:
    jmp __b8
  __b1:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:27
    lda #3
    jsr classify
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:27
    sta.z result
    jmp __b8
  __b5:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:25
    lda.z result
    sec
    sbc #2
    sta.z result
    jmp __b4
  __b3:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:23
    txa
    clc
    adc.z result
    sta.z result
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:22
    inx
    jmp __b2
}
// __register(A) char classify(__register(A) char v)
classify: {
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:8
    cmp #3
    bcs __b1
    lda #1
    rts
  __b1:
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:11
    cmp #3
    beq __b2
    lda #3
    rts
  __b2:
    lda #2
    // /private/tmp/rd-source-debug/kickc/nes/branches-loops/main.c:17
    rts
}
.segment Vectors
  VECTORS: .word 0, main, 0
