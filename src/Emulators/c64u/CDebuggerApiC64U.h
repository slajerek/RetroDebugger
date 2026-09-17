#ifndef _CDebuggerApiC64U_H_
#define _CDebuggerApiC64U_H_

#include "CDebuggerApi.h"

class CDebugInterfaceC64U;

// Stub debugger API for the C64 Ultimate backend.
// Most methods delegate to the base CDebuggerApi which calls through
// debugInterface.  Only methods that would SYS_FatalExit in the base
// class are overridden here to be safe no-ops.
class CDebuggerApiC64U : public CDebuggerApi
{
public:
	CDebuggerApiC64U(CDebugInterfaceC64U *debugInterface);

	CDebugInterfaceC64U *debugInterfaceC64U;

	// Base class calls SYS_FatalExit — override as no-op
	virtual void DetachEverything() override;
	virtual bool MakeJmp(int addr) override;

	// Return meaningful status even when C64U is not connected
	virtual nlohmann::json GetCpuStatusJson() override;
};

#endif
