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
	CDebugInterfaceViceTaskAttachCartridge(CDebugInterfaceVice *debugInterface, int cartridgeType, char *absolutePath);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	int cartridgeType;
	char *absolutePath;
};

class CDebugInterfaceViceTaskCartridgeDetach : public CDebugInterfaceTask
{
public:
	CDebugInterfaceViceTaskCartridgeDetach(CDebugInterfaceVice *debugInterface, int cartridgeType);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	int cartridgeType;
};

class CDebugInterfaceViceTaskResourceSetInt : public CDebugInterfaceTask
{
public:
	// resourceName/value are copied and owned by the task; ExecuteTask frees them
	CDebugInterfaceViceTaskResourceSetInt(CDebugInterfaceVice *debugInterface, const char *resourceName, int value);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	char *resourceName;
	int value;
};

class CDebugInterfaceViceTaskResourceSetString : public CDebugInterfaceTask
{
public:
	// resourceName/value are copied and owned by the task; ExecuteTask frees them
	CDebugInterfaceViceTaskResourceSetString(CDebugInterfaceVice *debugInterface, const char *resourceName, const char *value);
	virtual void ExecuteTask();

	CDebugInterfaceVice *debugInterface;
	char *resourceName;
	char *value;
};

#endif
