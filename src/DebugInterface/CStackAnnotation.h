#ifndef _CSTACKANNOTATION_H_
#define _CSTACKANNOTATION_H_

#include "SYS_Defs.h"

// What caused a byte to be pushed onto the 6502 stack
enum CStackEntryType : u8
{
	STACK_ENTRY_UNKNOWN = 0,
	STACK_ENTRY_VALUE,         // PHA — pushed accumulator value
	STACK_ENTRY_STATUS,        // PHP — pushed processor status
	STACK_ENTRY_JSR_PCH,       // JSR — high byte of return address (PC-1)
	STACK_ENTRY_JSR_PCL,       // JSR — low byte of return address
	STACK_ENTRY_BRK_PCH,       // BRK — high byte of PC+2
	STACK_ENTRY_BRK_PCL,       // BRK — low byte
	STACK_ENTRY_BRK_STATUS,    // BRK — status with B=1
	STACK_ENTRY_IRQ_PCH,       // IRQ — high byte of interrupted PC
	STACK_ENTRY_IRQ_PCL,       // IRQ — low byte
	STACK_ENTRY_IRQ_STATUS,    // IRQ — status with B=0
	STACK_ENTRY_NMI_PCH,       // NMI — high byte of interrupted PC
	STACK_ENTRY_NMI_PCL,       // NMI — low byte
	STACK_ENTRY_NMI_STATUS,    // NMI — status with B=0
};

// Which hardware device caused the interrupt
enum CIrqSource : u8
{
	IRQ_SOURCE_UNKNOWN = 0,

	// C64 main CPU
	IRQ_SOURCE_VIC,            // VIC-II (raster, sprite collision, lightpen)
	IRQ_SOURCE_CIA1,           // CIA1 timer
	IRQ_SOURCE_CIA2_NMI,       // CIA2 NMI
	IRQ_SOURCE_RESTORE_NMI,    // RESTORE key NMI
	IRQ_SOURCE_CARTRIDGE,      // Cartridge IRQ/NMI

	// C64 1541 drive CPU
	IRQ_SOURCE_VIA1,           // VIA1 (6522) — drive communication
	IRQ_SOURCE_VIA2,           // VIA2 (6522) — drive mechanics/head
	IRQ_SOURCE_IEC,            // IEC bus

	// Atari
	IRQ_SOURCE_POKEY,          // POKEY timer/serial/keyboard IRQ
	IRQ_SOURCE_ANTIC_DLI,      // ANTIC Display List Interrupt
	IRQ_SOURCE_ANTIC_VBI,      // ANTIC Vertical Blank Interrupt

	// NES (future)
	IRQ_SOURCE_PPU_NMI,        // PPU vblank NMI
	IRQ_SOURCE_APU,            // APU frame counter IRQ
	IRQ_SOURCE_MAPPER,         // Mapper-specific IRQ
};

// Per-CPU stack annotation data (256 entries, one per $0100-$01FF)
struct CStackAnnotationData
{
	u8  entryTypes[0x100];     // CStackEntryType
	u8  irqSources[0x100];    // CIrqSource (meaningful for IRQ/NMI/BRK entries)
	u16 originPC[0x100];      // PC of instruction that caused the push

	void Clear()
	{
		memset(entryTypes, STACK_ENTRY_UNKNOWN, sizeof(entryTypes));
		memset(irqSources, IRQ_SOURCE_UNKNOWN, sizeof(irqSources));
		memset(originPC, 0, sizeof(originPC));
	}
};

#endif
