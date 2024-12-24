#ifndef _CDebugInterfaceNesTasks_H_
#define _CDebugInterfaceNesTasks_H_

#include "CDebugInterfaceTask.h"

class CSlrString;
class CDebugInterfaceNes;

class CDebugInterfaceNesTaskJoystickEvent : public CDebugInterfaceTask
{
public:
	CDebugInterfaceNesTaskJoystickEvent(CDebugInterfaceNes *debugInterface, u8 buttonState, int port, u32 axis);
	virtual void ExecuteTask();
	
	CDebugInterfaceNes *debugInterface;

	u8 buttonState;	// DEBUGGER_EVENT_BUTTON
	int port;
	u32 axis;
};

class CDebugInterfaceNesTaskInsertCartridge : public CDebugInterfaceTask
{
public:
	CDebugInterfaceNesTaskInsertCartridge(CDebugInterfaceNes *debugInterface, CSlrString *pathToCart);
	virtual ~CDebugInterfaceNesTaskInsertCartridge();
	
	CDebugInterfaceNes *debugInterface;
	CSlrString *pathToCart;

	virtual void ExecuteTask();
};

class CDebugInterfaceNesTaskReset : public CDebugInterfaceTask
{
public:
	CDebugInterfaceNesTaskReset(CDebugInterfaceNes *debugInterface);
	CDebugInterfaceNes *debugInterface;
	virtual void ExecuteTask();
};

class CDebugInterfaceNesTaskResetHard : public CDebugInterfaceTask
{
public:
	CDebugInterfaceNesTaskResetHard(CDebugInterfaceNes *debugInterface);
	CDebugInterfaceNes *debugInterface;
	virtual void ExecuteTask();
};


#endif
