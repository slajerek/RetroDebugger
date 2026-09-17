#include "CTestIde64Settings.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"   // CViewC64.h only forward-declares it
#include "C64SettingsStorage.h"
#include "SYS_Main.h"
#include "CSlrString.h"
#include <cstring>
#include <string>

#if defined(RUN_COMMODORE64)
extern "C" {
#include "vice.h"
#include "resources.h"
}
#endif

// Proves the whole settings chain reaches VICE:
//   C64DebuggerSetSetting -> handler -> CDebugInterfaceVice -> VICE resource
// This is what the Settings/C64/IDE64 menu items do, minus ImGui.

void CTestIde64Settings::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;

#if !defined(RUN_COMMODORE64)
	TestCompleted(true, "C64 emulator disabled, skipping");
	return;
#else
	if (viewC64->debugInterfaceC64 == NULL)
	{
		TestCompleted(true, "C64 debug interface not available, skipping");
		return;
	}

	int step = 0;

	// preserve everything this test writes
	int oldVersion = 0;
	int oldUsbServer = 0;
	const char *oldAddrRaw = NULL;
	resources_get_int("IDE64Version", &oldVersion);
	resources_get_int("IDE64USBServer", &oldUsbServer);
	resources_get_string("IDE64USBServerAddress", &oldAddrRaw);
	std::string oldAddr = (oldAddrRaw != NULL) ? oldAddrRaw : "";

	bool oldCliOverrideUsbServer = c64SettingsIDE64CliOverrideUsbServer;
	bool oldCliOverrideUsbAddress = c64SettingsIDE64CliOverrideUsbAddress;
	bool oldCliOverrideVersion = c64SettingsIDE64CliOverrideVersion;

	// the command-line override flags must be off or the handlers will
	// deliberately skip applying -- that behaviour gets its own step below
	c64SettingsIDE64CliOverrideUsbServer = false;
	c64SettingsIDE64CliOverrideUsbAddress = false;
	c64SettingsIDE64CliOverrideVersion = false;

	int version = 2;					// V4.2
	C64DebuggerSetSetting("IDE64Version", &version);
	int viceVersion = -1;
	resources_get_int("IDE64Version", &viceVersion);
	step++;
	StepCompleted(step, viceVersion == 2, viceVersion == 2
				  ? "IDE64Version reached the VICE resource"
				  : "IDE64Version did not reach VICE");

	bool usbServer = true;
	C64DebuggerSetSetting("IDE64USBServer", &usbServer);
	int viceUsbServer = -1;
	resources_get_int("IDE64USBServer", &viceUsbServer);
	step++;
	StepCompleted(step, viceUsbServer == 1, viceUsbServer == 1
				  ? "IDE64USBServer reached the VICE resource"
				  : "IDE64USBServer did not reach VICE");

	// string settings take a CSlrString*, which is what the restore path
	// passes -- never a raw char*
	CSlrString *newAddr = new CSlrString("ip4://127.0.0.1:64248");
	C64DebuggerSetSetting("IDE64USBServerAddress", newAddr);
	delete newAddr;

	const char *viceAddr = NULL;
	resources_get_string("IDE64USBServerAddress", &viceAddr);
	bool addrOk = (viceAddr != NULL) && !strcmp(viceAddr, "ip4://127.0.0.1:64248");
	step++;
	StepCompleted(step, addrOk, addrOk
				  ? "IDE64USBServerAddress reached the VICE resource"
				  : "IDE64USBServerAddress did not reach VICE");

	// with the CLI override set, the stored value must NOT be applied
	c64SettingsIDE64CliOverrideVersion = true;
	int overriddenVersion = 0;			// V3
	C64DebuggerSetSetting("IDE64Version", &overriddenVersion);
	resources_get_int("IDE64Version", &viceVersion);
	bool overrideOk = (viceVersion == 2) && (c64SettingsIDE64Version == 0);
	step++;
	StepCompleted(step, overrideOk, overrideOk
				  ? "command line takes precedence: VICE resource kept, global updated"
				  : "CLI override did not protect the VICE resource");

	// restore
	c64SettingsIDE64CliOverrideUsbServer = oldCliOverrideUsbServer;
	c64SettingsIDE64CliOverrideUsbAddress = oldCliOverrideUsbAddress;
	c64SettingsIDE64CliOverrideVersion = oldCliOverrideVersion;

	viewC64->debugInterfaceC64->SetIde64Version(oldVersion);
	viewC64->debugInterfaceC64->SetIde64UsbServerEnabled(oldUsbServer != 0);
	viewC64->debugInterfaceC64->SetIde64UsbServerAddress(oldAddr.c_str());
	c64SettingsIDE64Version = oldVersion;
	c64SettingsIDE64UsbServerEnabled = (oldUsbServer != 0);

	bool allOk = (viceVersion == 2) && (viceUsbServer == 1) && addrOk && overrideOk;
	TestCompleted(allOk, allOk
				  ? "IDE64 settings reach VICE, and the command line keeps precedence"
				  : "IDE64 settings chain is broken");
#endif
}

void CTestIde64Settings::Cancel()
{
	isRunning = false;
}
