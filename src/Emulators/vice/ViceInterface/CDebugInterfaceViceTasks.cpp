#include "CDebugInterfaceViceTasks.h"
#include "CDebugInterfaceVice.h"
#include "CSnapshotsManager.h"
#include "CByteBuffer.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"

extern "C" {
	void c64d_joystick_key_down(int key, unsigned int joyport);
	void c64d_joystick_key_up(int key, unsigned int joyport);
	int keyboard_key_pressed(signed long key);
	int keyboard_key_released(signed long key);
	int keyboard_key_pressed_matrix(int row, int column, int shift);
	int keyboard_key_released_matrix(int row, int column, int shift);
	int keyboard_set_latch_keyarr(int row, int col, int value);
	void keyboard_set_keyarr_any(int row, int col, int value);
	void c64d_keyboard_key_down_latch();
	void c64d_keyboard_key_up_latch();
}

CDebugInterfaceViceTaskJoystickEvent::CDebugInterfaceViceTaskJoystickEvent(
	CDebugInterfaceVice *debugInterface, u8 buttonState, int port, u32 axis)
{
	this->debugInterface = debugInterface;
	this->buttonState = buttonState;
	this->port = port;
	this->axis = axis;
}

void CDebugInterfaceViceTaskJoystickEvent::ExecuteTask()
{
	if (debugInterface->snapshotsManager->isStoreInputEventsEnabled)
	{
		CByteBuffer *inputEventsBuffer = debugInterface->GetInputEventsBufferForCurrentCycle();
		inputEventsBuffer->Rewind();

		bool found = false;
		while (inputEventsBuffer->IsEof() == false)
		{
			u8 eventType = inputEventsBuffer->GetU8();
			if (eventType == DEBUGGER_EVENT_TYPE_JOYSTICK)
			{
				int bufferEventPort = inputEventsBuffer->GetI32();
				if (bufferEventPort == port)
				{
					// replace existing event for this port
					int len = inputEventsBuffer->length;
					inputEventsBuffer->PutU32(axis);
					inputEventsBuffer->PutU8(buttonState);
					inputEventsBuffer->length = len;
					found = true;
				}
				else
				{
					inputEventsBuffer->GetU32(); // skip axis
					inputEventsBuffer->GetU8();  // skip buttonState
				}
			}
			else if (eventType == DEBUGGER_EVENT_TYPE_KEYBOARD)
			{
				inputEventsBuffer->GetU32(); // skip keyCode
				inputEventsBuffer->GetU8();  // skip buttonState
			}
			else
			{
				LOGError("CDebugInterfaceViceTaskJoystickEvent::ExecuteTask: unknown event type: %d", eventType);
				break;
			}
		}

		if (!found)
		{
			inputEventsBuffer->PutU8(DEBUGGER_EVENT_TYPE_JOYSTICK);
			inputEventsBuffer->PutI32(port);
			inputEventsBuffer->PutU32(axis);
			inputEventsBuffer->PutU8(buttonState);
		}
	}

	// Apply the input to the emulator
	if (buttonState == DEBUGGER_EVENT_BUTTON_DOWN)
	{
		c64d_joystick_key_down(axis, port);
	}
	else
	{
		c64d_joystick_key_up(axis, port);
	}
}

CDebugInterfaceViceTaskKeyboardEvent::CDebugInterfaceViceTaskKeyboardEvent(
	CDebugInterfaceVice *debugInterface, u8 buttonState, u32 mtKeyCode,
	int matrixRow, int matrixCol, int shift)
{
	this->debugInterface = debugInterface;
	this->buttonState = buttonState;
	this->mtKeyCode = mtKeyCode;
	this->matrixRow = matrixRow;
	this->matrixCol = matrixCol;
	this->shift = shift;
}

void CDebugInterfaceViceTaskKeyboardEvent::ExecuteTask()
{
	if (debugInterface->snapshotsManager->isStoreInputEventsEnabled)
	{
		CByteBuffer *inputEventsBuffer = debugInterface->GetInputEventsBufferForCurrentCycle();
		inputEventsBuffer->PutU8(DEBUGGER_EVENT_TYPE_KEYBOARD);
		inputEventsBuffer->PutU32(mtKeyCode);
		inputEventsBuffer->PutU8(buttonState);
	}

	// press/release via the resolved matrix position — same sequence the virtual keyboard
	// uses (CViewC64KeyMap::SelectKey); the c64d_*_latch calls latch immediately, without
	// the randomized KEYBOARD_RAND alarm, so measurements stay cycle-deterministic
	if (buttonState == DEBUGGER_EVENT_BUTTON_DOWN)
	{
		if (matrixRow < 0)
		{
			keyboard_set_keyarr_any(matrixRow, matrixCol, 1);
		}
		else
		{
			keyboard_key_pressed_matrix(matrixRow, matrixCol, shift);
			keyboard_set_latch_keyarr(matrixRow, matrixCol, 1);
			c64d_keyboard_key_down_latch();
		}
	}
	else
	{
		if (matrixRow < 0)
		{
			keyboard_set_keyarr_any(matrixRow, matrixCol, 0);
		}
		else
		{
			keyboard_key_released_matrix(matrixRow, matrixCol, shift);
			keyboard_set_latch_keyarr(matrixRow, matrixCol, 0);
			c64d_keyboard_key_up_latch();
		}
	}
}

CDebugInterfaceViceTaskReset::CDebugInterfaceViceTaskReset(CDebugInterfaceVice *debugInterface, bool isHardReset)
{
	this->debugInterface = debugInterface;
	this->isHardReset = isHardReset;
}

void CDebugInterfaceViceTaskReset::ExecuteTask()
{
	if (isHardReset)
	{
		debugInterface->ResetHardSynced();
	}
	else
	{
		debugInterface->ResetSoftSynced();
	}
}
