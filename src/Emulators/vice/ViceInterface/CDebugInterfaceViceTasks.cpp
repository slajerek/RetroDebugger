#include "CDebugInterfaceViceTasks.h"
#include "CDebugInterfaceVice.h"
#include "CSnapshotsManager.h"
#include "CByteBuffer.h"
#include "DebuggerDefs.h"
#include "SYS_Main.h"
#include <cstring>
#include <cstdlib>

extern "C" {
	void c64d_joystick_key_down(int key, unsigned int joyport);
	void c64d_joystick_key_up(int key, unsigned int joyport);
	int keyboard_key_pressed(signed long key);
	int keyboard_key_released(signed long key);
	int cartridge_attach_image(int type, const char *filename);
	void cartridge_detach_image(int type);
#include "resources.h"
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
	CDebugInterfaceVice *debugInterface, u8 buttonState, u32 mtKeyCode)
{
	this->debugInterface = debugInterface;
	this->buttonState = buttonState;
	this->mtKeyCode = mtKeyCode;
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

	if (buttonState == DEBUGGER_EVENT_BUTTON_DOWN)
	{
		keyboard_key_pressed((signed long)mtKeyCode);
	}
	else
	{
		keyboard_key_released((signed long)mtKeyCode);
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

CDebugInterfaceViceTaskAttachCartridge::CDebugInterfaceViceTaskAttachCartridge(CDebugInterfaceVice *debugInterface, int cartridgeType, char *absolutePath)
{
	this->debugInterface = debugInterface;
	this->cartridgeType = cartridgeType;
	this->absolutePath = absolutePath;
}

void CDebugInterfaceViceTaskAttachCartridge::ExecuteTask()
{
	// cartridge_attach_image() runs cart_power_off()'s power-cycle reset
	// from this (CPU) thread and the machine_reset() triggered behind it
	// applies on the next interrupt check of the very same execution loop,
	// so the freshly zeroed RAM from mem_powerup() is never observed by an
	// executing CPU.
	cartridge_attach_image(this->cartridgeType, this->absolutePath);
	this->debugInterface->ResetEmulationFrameCounter();
	delete[] this->absolutePath;
}

CDebugInterfaceViceTaskCartridgeDetach::CDebugInterfaceViceTaskCartridgeDetach(CDebugInterfaceVice *debugInterface, int cartridgeType)
{
	this->debugInterface = debugInterface;
	this->cartridgeType = cartridgeType;
}

void CDebugInterfaceViceTaskCartridgeDetach::ExecuteTask()
{
	// Same reasoning as the attach task: cartridge_detach_image() tears down
	// the cart's io-device registrations (and, through the clean-up hooks,
	// anything else the executing CPU may still be dereferencing) and so has
	// to happen while no other thread is inside the execution loop.
	cartridge_detach_image(this->cartridgeType);
}

CDebugInterfaceViceTaskResourceSetInt::CDebugInterfaceViceTaskResourceSetInt(CDebugInterfaceVice *debugInterface, const char *resourceName, int value)
{
	this->debugInterface = debugInterface;
	size_t nameLen = strlen(resourceName) + 1;
	this->resourceName = new char[nameLen];
	strcpy(this->resourceName, resourceName);
	this->value = value;
}

void CDebugInterfaceViceTaskResourceSetInt::ExecuteTask()
{
	// The resource setter runs on this thread, so anything its setter hooks do
	// (usbserver_activate()'s alarm create/destroy and socket close are the
	// known ones) cannot race the execution loop's own alarm-queue walk.
	resources_set_int(this->resourceName, this->value);
	delete[] this->resourceName;
}

CDebugInterfaceViceTaskResourceSetString::CDebugInterfaceViceTaskResourceSetString(CDebugInterfaceVice *debugInterface, const char *resourceName, const char *value)
{
	this->debugInterface = debugInterface;
	size_t nameLen = strlen(resourceName) + 1;
	this->resourceName = new char[nameLen];
	strcpy(this->resourceName, resourceName);
	size_t valueLen = strlen(value) + 1;
	this->value = new char[valueLen];
	strcpy(this->value, value);
}

void CDebugInterfaceViceTaskResourceSetString::ExecuteTask()
{
	resources_set_string(this->resourceName, this->value);
	delete[] this->resourceName;
	delete[] this->value;
}
