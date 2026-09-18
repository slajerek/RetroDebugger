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
	CDebugInterfaceViceTaskKeyboardEvent(CDebugInterfaceVice *debugInterface, u8 buttonState, u32 mtKeyCode);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	u8 buttonState;
	u32 mtKeyCode;
};

class CDebugInterfaceViceTaskReset : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskReset(CDebugInterfaceVice *debugInterface, bool isHardReset);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	bool isHardReset;
};

class CDebugInterfaceViceTaskAttachCartridge : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskAttachCartridge(CDebugInterfaceVice *debugInterface, char *absolutePath);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	char *absolutePath;
};

#endif
