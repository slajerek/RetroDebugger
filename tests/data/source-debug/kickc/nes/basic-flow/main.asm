/* basic-flow: the shared minimal source of spec #9.2. Step Into enters add()
   and doSomething(), Step Over runs them, Step Out returns to the caller, and
   the final loop is a stable stop point. Portable across Oscar64, cc65, KickC. */
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
  .label result = 2
.segment Code
__start: {
    lda #0
    sta.z result
    jsr main
    rts
}
main: {
    lda #1
    sta.z result
    jsr doSomething
    ldx.z result
    lda #2
    jsr add
    sta.z result
  __b1:
    jmp __b1
}
doSomething: {
    ldx.z result
    lda #3
    jsr add
    sta.z result
    rts
}
// __register(A) char add(__register(X) char left, __register(A) char right)
add: {
    stx.z $ff
    clc
    adc.z $ff
    rts
}
