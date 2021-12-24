#include "C64DisplayListCodeGenerator.h"
#include "CDebugInterfaceC64.h"
#include "CViewVicEditor.h"
#include "CViewDisassembly.h"
#include "CViewC64.h"

// TODO: move as plugin: c64 display generator. THIS IS START OF POC, not used here

#define B(b) dataAdapter->AdapterWriteByte(addr++, (b))
#define A(b) strcpy(buf, (b)); addr += viewDisassembly->Assemble(addr, (buf), false);

void C64GenerateDisplayListCode(CViewVicEditor *vicEditor)
{
	LOGD("C64GenerateDisplayListCode");

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
}
