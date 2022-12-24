#include "C64DisplayListCodeGenerator.h"
#include "CDebugInterfaceC64.h"
#include "CViewDisassembly.h"
#include "CViewC64.h"

// TODO: move as plugin: c64 display generator. THIS IS START OF POC, not used here

// Display list generator is PRG asm code generator based on list of display commands, such as:
// change border colors, open right border, open bottom border, change sprite pos, change spirte flags, etc.
// UI should have a editor that can allow scroll the raster cursor and select a code to put synced to this cycle from a dropdown menu
// of course not all combinations and cycles are doable thus mark cycles that can't be assigned in grayed color. 
// This is to be used with VIC Editor, color bars, effects setup etc.

#define B(b) dataAdapter->AdapterWriteByte(addr++, (b))
#define A(b) strcpy(buf, (b)); addr += viewDisassembly->Assemble(addr, (buf), false);

void C64GenerateDisplayListCode(CViewC64VicEditor *vicEditor)
{
	LOGTODO("C64GenerateDisplayListCode");

	/*
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	CDataAdapter *dataAdapter = debugInterface->dataAdapterC64;

	// TODO: move Assemble from view to specific assemble class in CDebugInterfaceC64 / MemoryAdapter
	CViewDisassembly *viewDisassembly = viewC64->viewC64Disassembly;

	char buf[128];
	
	u16 addr = 0x0801;

	// BASICe
	B(	0x00	);
	B(	0x00	);
	
	// Init CODE
	A(	"SEI"	);
	A(	"NOP"	);
	A(	"NOP"	);
	A(	"NOP"	);

	
	LOGD("C64GenerateDisplayListCode done");
	*/
}
