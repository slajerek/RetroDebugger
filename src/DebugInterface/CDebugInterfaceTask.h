#ifndef _CDEBUGINTERFACETASK_H_
#define _CDEBUGINTERFACETASK_H_

#include "SYS_Defs.h"

class CDebugInterface;

// these tasks are synced and executed in various emulation moments
class CDebugInterfaceTask
{
public:
	virtual void ExecuteTask();
};

#endif

