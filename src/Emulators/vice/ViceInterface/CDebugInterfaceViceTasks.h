#ifndef _CDebugInterfaceViceTasks_H_
#define _CDebugInterfaceViceTasks_H_

#include "CDebugInterfaceTask.h"
#include "SYS_Defs.h"

class CDebugInterfaceVice;

class CDebugInterfaceViceTaskJoystickEvent : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskJoystickEvent(CDebugInterfaceVice *debugInterface, u8 buttonState, int port, u32 axis);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	u8 buttonState;
	int port;
	u32 axis;
};

class CDebugInterfaceViceTaskKeyboardEvent : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskKeyboardEvent(CDebugInterfaceVice *debugInterface, u8 buttonState, u32 mtKeyCode,
										 int matrixRow, int matrixCol, int shift);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	u8 buttonState;
	u32 mtKeyCode;
	// C64 keyboard matrix position resolved from C64KeyMap (matrixRow < 0 = special key, e.g. RESTORE)
	int matrixRow;
	int matrixCol;
	int shift;
};

class CDebugInterfaceViceTaskReset : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskReset(CDebugInterfaceVice *debugInterface, bool isHardReset);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	bool isHardReset;
};

#endif
