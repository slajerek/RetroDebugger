#include "CTestIde64UsbListener.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"   // CViewC64.h only forward-declares it
#include "C64SettingsStorage.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"
#include "CSlrString.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

#if defined(RUN_COMMODORE64)
extern "C" {
#include "vice.h"
#include "resources.h"
#include "vicesocket.h"
}
#endif

// The one test that proves the feature the user actually asked for.
//
// ViceNetworkSocket only checks generic sockets and Ide64Settings only checks
// resources; neither would have caught the reported bug, because the IDE64 USB
// server opens nothing unless an IDE64 cartridge is attached (ide64.c:
// set_usbserver() is guarded on ide64_rom_list_item).
//
// ide64_bin_attach() validates only the file SIZE (64K/128K/512K) and then
// loads the bytes, so a generated 64KiB dummy attaches fine -- no copyrighted
// IDEDOS image needed.
#define TEST_ADDRESS "ip4://127.0.0.1:64249"
#define DUMMY_ROM_PATH "/tmp/c64d-test-ide64-dummy.bin"

void CTestIde64UsbListener::Run(ITestCallback *cb)
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

	// preserve what we change
	int oldUsbServer = 0;
	const char *oldAddrRaw = NULL;
	resources_get_int("IDE64USBServer", &oldUsbServer);
	resources_get_string("IDE64USBServerAddress", &oldAddrRaw);
	std::string oldAddr = (oldAddrRaw != NULL) ? oldAddrRaw : "";

	bool oldCliOverrideUsbServer = c64SettingsIDE64CliOverrideUsbServer;
	bool oldCliOverrideUsbAddress = c64SettingsIDE64CliOverrideUsbAddress;
	c64SettingsIDE64CliOverrideUsbServer = false;
	c64SettingsIDE64CliOverrideUsbAddress = false;

	// 1. write a 64KiB dummy ROM -- content is irrelevant, only the size is checked
	FILE *fp = fopen(DUMMY_ROM_PATH, "wb");
	step++;
	if (fp == NULL)
	{
		StepCompleted(step, false, "cannot create the dummy IDE64 ROM in /tmp");
		TestCompleted(false, "dummy ROM could not be written");
		return;
	}
	unsigned char chunk[1024];
	memset(chunk, 0, sizeof(chunk));
	for (int i = 0; i < 64; i++)
	{
		fwrite(chunk, 1, sizeof(chunk), fp);
	}
	fclose(fp);
	StepCompleted(step, true, "64KiB dummy IDE64 ROM written to " DUMMY_ROM_PATH);

	// 2. configure and attach through the production path
	viewC64->debugInterfaceC64->SetIde64UsbServerAddress(TEST_ADDRESS);
	viewC64->debugInterfaceC64->SetIde64UsbServerEnabled(true);

	CSlrString *romPath = new CSlrString(DUMMY_ROM_PATH);
	viewC64->debugInterfaceC64->AttachIde64Cartridge(romPath);
	delete romPath;

	// 3. the listener must now accept a connection. Poll: the attach runs
	//    through the emulation thread in some paths, and usbserver_activate()
	//    happens when the cart registers.
	vice_network_socket_address_t *addr = vice_network_address_generate(TEST_ADDRESS, 0);
	vice_network_socket_t *client = NULL;
	for (int i = 0; i < 300 && client == NULL; i++)
	{
		client = vice_network_client(addr);
		if (client == NULL)
			SYS_Sleep(10);
	}

	step++;
	bool listening = (client != NULL);
	StepCompleted(step, listening, listening
				  ? "IDE64 USB server accepted a connection on " TEST_ADDRESS
				  : "no listener on " TEST_ADDRESS " after attaching the cart and enabling the server");

	// 4. clean up. This test changes global emulator state, so the restore has
	//    to be thorough -- attaching an IDE64 cart queues a POWER_CYCLE reset
	//    (ide64.c, set_version) that would otherwise fire inside a LATER test.
	//    An incomplete cleanup here broke DetachCartridgePaused and
	//    SnapshotBoundary while this test was being written.
	if (client != NULL)
	{
		vice_network_socket_close(client);
	}
	vice_network_address_close(addr);

	viewC64->debugInterfaceC64->SetIde64UsbServerEnabled(false);
	viewC64->debugInterfaceC64->DetachIde64Cartridge();

	// let the queued reset actually happen and settle before the next test
	viewC64->debugInterfaceC64->ResetHard();
	SYS_Sleep(1500);

	viewC64->debugInterfaceC64->SetIde64UsbServerAddress(oldAddr.c_str());
	if (oldUsbServer != 0)
	{
		viewC64->debugInterfaceC64->SetIde64UsbServerEnabled(true);
	}

	c64SettingsIDE64CliOverrideUsbServer = oldCliOverrideUsbServer;
	c64SettingsIDE64CliOverrideUsbAddress = oldCliOverrideUsbAddress;

	unlink(DUMMY_ROM_PATH);

	TestCompleted(listening, listening
				  ? "IDE64 USB server (pc-link) opens a listener"
				  : "IDE64 USB server did not open a listener");
#endif
}

void CTestIde64UsbListener::Cancel()
{
	isRunning = false;
}
