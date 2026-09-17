#include "CDebuggerApiC64U.h"
#include "CDebugInterfaceC64U.h"
#include "CDataAdapter.h"

using namespace nlohmann;

CDebuggerApiC64U::CDebuggerApiC64U(CDebugInterfaceC64U *debugInterface)
: CDebuggerApi(debugInterface)
, debugInterfaceC64U(debugInterface)
{
}

void CDebuggerApiC64U::DetachEverything()
{
	LOGError("CDebuggerApiC64U::DetachEverything: not supported by C64U backend");
}

bool CDebuggerApiC64U::MakeJmp(int addr)
{
	LOGError("CDebuggerApiC64U::MakeJmp: not supported by C64U backend");
	return false;
}

json CDebuggerApiC64U::GetCpuStatusJson()
{
	json status;
	status["platform"] = "c64u";
	status["connected"] = debugInterfaceC64U->isRunning;

	if (debugInterfaceC64U->isRunning)
	{
		u16 pc; u8 a, x, y, p, sp;
		debugInterfaceC64U->GetCpuRegs(&pc, &a, &x, &y, &p, &sp);
		status["PC"] = pc;
		status["A"]  = a;
		status["X"]  = x;
		status["Y"]  = y;
		status["SP"] = sp;
		status["P"]  = p;
	}
	else
	{
		status["error"] = "C64U not connected";
	}

	return status;
}
