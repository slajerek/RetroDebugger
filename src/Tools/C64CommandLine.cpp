#include "C64D_Version.h"
#include "EmulatorsConfig.h"
#include "C64CommandLine.h"
#include "SYS_FileSystem.h"
#include "CViewC64.h"
#include "SYS_CommandLine.h"
#include "CDebugSymbols.h"
#include "CMainMenuHelper.h"
#include "CViewSnapshots.h"
#include "CSlrString.h"
#include "RES_ResourceManager.h"
#include "C64SettingsStorage.h"
#include "C64SharedMemory.h"
#include "CGuiMain.h"
#include "SND_SoundEngine.h"
#include "CViewJukeboxPlaylist.h"
#include "SYS_Platform.h"

#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceNes.h"

// TODO: fixme, plugins should also parse command line options
extern char *crtMakerConfigFilePath;

// workaround for Windows to not show console when user clicked file in OS and started debugger, then with added --pass
bool c64CommandLineSkipConsoleAttach = false;

#define C64D_PASS_CONFIG_DATA_MARKER	0x029A
#define C64D_PASS_CONFIG_DATA_VERSION	0x0003

CSlrMutex *c64DebuggerStartupTasksCallbacksMutex = NULL;
std::list<C64DebuggerStartupTaskCallback *> c64DebuggerStartupTasksCallbacks;

bool isPRGInCommandLine = false;
bool isD64InCommandLine = false;
bool isSNAPInCommandLine = false;
bool isTAPInCommandLine = false;
bool isCRTInCommandLine = false;
bool isREUInCommandLine = false;

bool isXEXInCommandLine = false;
bool isATRInCommandLine = false;

bool cmdLineOptionDoAutoJmp = false;

void C64DebuggerPassConfigToRunningInstance();

#if !defined(WIN32)

#define printLine printf
#define printInfo printf
#define printHelp printf

#else

// Warning: on Win32 API apps do not have a console, so this will not be printed to console but log instead:
#define printLine LOGM
#define printInfo(...)	{	MessageBox(NULL, __VA_ARGS__, "Retro Debugger", MB_ICONWARNING | MB_OK);	}
#define printHelp printf

#include "SYS_Startup.h"

#endif

void C64DebuggerInitStartupTasks()
{
	LOGD("C64DebuggerInitStartupTasks");
	c64DebuggerStartupTasksCallbacksMutex = new CSlrMutex("c64DebuggerStartupTasksCallbacksMutex");
}

void c64PrintC64DebuggerVersion()
{
	SYS_AttachConsole();

	printHelp("Retro Debugger v%s by Slajerek/Samar\n", RETRODEBUGGER_VERSION_STRING);

#if defined(RUN_COMMODORE64)
	printHelp("VICE %s by The VICE Team\n", C64DEBUGGER_VICE_VERSION_STRING);
#endif
	
#if defined(RUN_ATARI)
	printHelp("Atari 800 Emulator, Version %s\n", C64DEBUGGER_ATARI800_VERSION_STRING);
#endif
	
#if defined(RUN_NES)
	printHelp("NestopiaUE, Version %s\n", C64DEBUGGER_NES_VERSION_STRING);
#endif
}

void c64PrintCommandLineHelp()
{
	SYS_AttachConsole();
	
	c64PrintC64DebuggerVersion();
	
	printHelp("\n");
	printHelp("-help\n");
	printHelp("     show this help\n");
	printHelp("-version\n");
	printHelp("     display version string\n");
	printHelp("\n");
//	printHelp("-layout <id>\n");
//	printHelp("     start with layout id <1-%d>\n", SCREEN_LAYOUT_MAX);
	printHelp("-breakpoints <file>\n");
	printHelp("     load breakpoints\n");
	printHelp("-symbols <file>\n");
	printHelp("     load symbols (code labels)\n");
	printHelp("-watch <file>\n");
	printHelp("     load watches\n");
	printHelp("-debuginfo <file>\n");
	printHelp("     load debug symbols (*.dbg)\n");
//	printHelp("\n");
	printHelp("-wait <ms>\n");
	printHelp("     wait before performing tasks\n");
	
#if defined(RUN_COMMODORE64)
	printHelp("-c64 select emulator: C64 Vice\n");
	printHelp("-prg <file>\n");
	printHelp("     load PRG file into memory\n");
	printHelp("-d64 <file>\n");
	printHelp("     insert D64 disk\n");
	printHelp("-tap <file>\n");
	printHelp("     attach TAP file\n");
	printHelp("-crt <file>\n");
	printHelp("     attach cartridge\n");
	printHelp("-reu <file>\n");
	printHelp("     attach REU\n");
	printHelp("-snapshot <file>\n");
	printHelp("     load snapshot from file\n");
	printHelp("-jmp <addr>\n");
	printHelp("     jmp to address, for example jmp x1000, jmp $1000 or jmp 4096\n");
	printHelp("-autojmp\n");
	printHelp("     automatically jmp to address if basic SYS is detected\n");
	printHelp("-alwaysjmp\n");
	printHelp("     always jmp to load address of PRG\n");
	printHelp("-autorundisk\n");
	printHelp("     automatically load first PRG from inserted disk\n");
#endif
	
	printHelp("-unpause\n");
	printHelp("     force code running\n");
	printHelp("-reset\n");
	printHelp("     hard reset machine\n");
	
#if defined(RUN_ATARI)
	printHelp("-atari select emulator: Atari800\n");
	printHelp("-xex <file>\n");
	printHelp("     load XEX file into memory\n");
	printHelp("-atr <file>\n");
	printHelp("     insert ATR disk\n");
#endif

#if defined(RUN_NES)
	printHelp("-nes select emulator: NestopiaUE\n");
	printHelp("-ines <file>\n");
	printHelp("     insert iNES cartridge\n");
#endif

	printHelp("-soundout <\"device name\" | device number>\n");
	printHelp("     set sound out device by name or number\n");
	printHelp("-fullscreen\n");
	printHelp("     start in full screen mode\n");
	printHelp("-playlist <file>\n");
	printHelp("     load and start jukebox playlist from json file\n");
	printHelp("\n");
	printHelp("-clearsettings\n");
	printHelp("     clear all config settings\n");
	printHelp("-pass\n");
	printHelp("     pass parameters to already running instance\n");
	printHelp("\n");
}

std::vector<const char *>::iterator c64cmdIt;

char *c64ParseCommandLineGetArgument()
{
	if (c64cmdIt == sysCommandLineArguments.end())
	{
		c64PrintCommandLineHelp();
		SYS_CleanExit();
	}
	
	const char *arg = *c64cmdIt;
	c64cmdIt++;
	
	LOGD("c64ParseCommandLineGetArgument: arg='%s'", arg);
	
	return (char*)arg;
}

void C64DebuggerParseCommandLine0()
{
	LOGD("C64DebuggerParseCommandLine0");

	if (sysCommandLineArguments.empty())
		return;

	// check if it's just a single argument with file path (drop file on exe in Win32)
	if (sysCommandLineArguments.size() == 1)	// 1   , 3 for dev xcode
	{
		const char *arg = sysCommandLineArguments[0];

		LOGD("arg=%s", arg);

		if (SYS_FileExists(arg))
		{
			// workaround for Windows, so we will not show console when user started by clicking on prg/d64/... file
			c64CommandLineSkipConsoleAttach = true;

			CSlrString *filePath = new CSlrString(arg);
			filePath->DebugPrint("filePath=");

			CSlrString *ext = filePath->GetFileExtensionComponentFromPath();
			ext->DebugPrint("ext=");

#if defined(RUN_COMMODORE64)
			if (ext->CompareWith("prg") || ext->CompareWith("PRG"))
			{
				isPRGInCommandLine = true;

				const char *path = sysCommandLineArguments[0];
				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-wait");
				sysCommandLineArguments.push_back("700");
				sysCommandLineArguments.push_back("-prg");
				sysCommandLineArguments.push_back(path);
				sysCommandLineArguments.push_back("-autojmp");

				// TODO: this will be overwritten by settings loader
				c64SettingsFastBootKernalPatch = true;
				
				LOGD("delete filePath");
				delete filePath;
				
				//				c64SettingsPathToPRG = filePath;
				c64SettingsWaitOnStartup = 500;
				//				c64SettingsAutoJmp = true;
				LOGD("delete ext");
				delete ext;
				
				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("d64") || ext->CompareWith("D64")
					 || ext->CompareWith("g64") || ext->CompareWith("G64"))
			{
				isD64InCommandLine = true;

				const char *path = sysCommandLineArguments[0];
				
				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-d64");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("tap") || ext->CompareWith("TAP")
					 || ext->CompareWith("t64") || ext->CompareWith("T64"))
			{
				isTAPInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-tap");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("crt") || ext->CompareWith("CRT"))
			{
				isCRTInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-crt");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("reu") || ext->CompareWith("REU"))
			{
				isREUInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-reu");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("snap") || ext->CompareWith("SNAP")
					 || ext->CompareWith("vsf") || ext->CompareWith("VSF"))
			{
				isSNAPInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-c64");
				sysCommandLineArguments.push_back("-snapshot");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
#endif

#if defined(RUN_ATARI)
			if (ext->CompareWith("xex") || ext->CompareWith("XEX"))
			{
				isXEXInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-atari");
				sysCommandLineArguments.push_back("-wait");
				sysCommandLineArguments.push_back("700");
				sysCommandLineArguments.push_back("-xex");
				sysCommandLineArguments.push_back(path);
				sysCommandLineArguments.push_back("-autojmp");

				// TODO: this will be overwritten by settings loader
				c64SettingsFastBootKernalPatch = true;

				c64SettingsWaitOnStartup = 500;
				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
			else if (ext->CompareWith("atr") || ext->CompareWith("ATR"))
			{
				isATRInCommandLine = true;

				const char *path = sysCommandLineArguments[0];

				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-atari");
				sysCommandLineArguments.push_back("-atr");
				sysCommandLineArguments.push_back(path);

				delete ext;
				delete filePath;

				// pass to running instance if exists
				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
#endif

#if defined(RUN_NES)
			if (ext->CompareWith("nes") || ext->CompareWith("NES"))
			{
				const char *path = sysCommandLineArguments[0];
				sysCommandLineArguments.clear();
				sysCommandLineArguments.push_back("-pass");
				sysCommandLineArguments.push_back("-nes");
				sysCommandLineArguments.push_back("-wait");
				sysCommandLineArguments.push_back("100");
				sysCommandLineArguments.push_back("-ines");
				sysCommandLineArguments.push_back(path);

				c64SettingsWaitOnStartup = 50;
				delete ext;
				delete filePath;

				C64DebuggerInitSharedMemory();
				C64DebuggerPassConfigToRunningInstance();
				return;
			}
#endif
			delete ext;
			delete filePath;
		}
	}

	c64cmdIt = sysCommandLineArguments.begin();

	while(c64cmdIt != sysCommandLineArguments.end())
	{
		char *cmd = c64ParseCommandLineGetArgument();

		if (!strcmp(cmd, "help") || !strcmp(cmd, "h")
			|| !strcmp(cmd, "-help") || !strcmp(cmd, "-h")
			|| !strcmp(cmd, "--help") || !strcmp(cmd, "--h"))
		{
			c64PrintCommandLineHelp();
			SYS_CleanExit();
		}

		if (!strcmp(cmd, "version") || !strcmp(cmd, "v")
			|| !strcmp(cmd, "-version") || !strcmp(cmd, "-v")
			|| !strcmp(cmd, "--version") || !strcmp(cmd, "--v"))
		{
			c64PrintC64DebuggerVersion();
			SYS_CleanExit();
		}

		if (!strcmp(cmd, "-pass") || !strcmp(cmd, "pass"))
		{
			C64DebuggerInitSharedMemory();
			C64DebuggerPassConfigToRunningInstance();
		}
	}
}

void C64DebuggerParseCommandLine1()
{
	if (sysCommandLineArguments.empty())
		return;

	c64cmdIt = sysCommandLineArguments.begin();

	while(c64cmdIt != sysCommandLineArguments.end())
	{
		char *cmd = c64ParseCommandLineGetArgument();

		if (!strcmp(cmd, "help") || !strcmp(cmd, "h")
			|| !strcmp(cmd, "-help") || !strcmp(cmd, "-h")
			|| !strcmp(cmd, "--help") || !strcmp(cmd, "--h"))
		{
			c64PrintCommandLineHelp();
			SYS_CleanExit();
		}

		if (!strcmp(cmd, "version") || !strcmp(cmd, "v")
			|| !strcmp(cmd, "-version") || !strcmp(cmd, "-v")
			|| !strcmp(cmd, "--version") || !strcmp(cmd, "--v"))
		{
			c64PrintC64DebuggerVersion();
			SYS_CleanExit();
		}

		if (!strcmp(cmd, "-clearsettings") || !strcmp(cmd, "clearsettings"))
		{
			c64SettingsSkipConfig = true;
			printInfo("Skipping loading config\n");
			LOGD("Skipping auto loading settings config");
		}
	}
}

void C64DebuggerAddStartupTaskCallback(C64DebuggerStartupTaskCallback *callback)
{
	c64DebuggerStartupTasksCallbacksMutex->Lock();
	c64DebuggerStartupTasksCallbacks.push_back(callback);
	c64DebuggerStartupTasksCallbacksMutex->Unlock();
}

CSlrString *c64CommandLineAudioOutDevice = NULL;

bool c64CommandLineHardReset = false;
bool c64CommandLineWindowFullScreen = false;

void c64PreRunStartupCallbacks()
{
	c64DebuggerStartupTasksCallbacksMutex->Lock();
	if (!c64DebuggerStartupTasksCallbacks.empty())
	{
		LOGD("pre-run c64DebuggerStartupTasksCallbacks");
		for (std::list<C64DebuggerStartupTaskCallback *>::iterator it = c64DebuggerStartupTasksCallbacks.begin();
			 it != c64DebuggerStartupTasksCallbacks.end(); ++it)
		{
			C64DebuggerStartupTaskCallback *callback = *it;
			callback->PreRunStartupTaskCallback();
		}
		LOGD("pre-run c64DebuggerStartupTasksCallbacks completed");
	}
	c64DebuggerStartupTasksCallbacksMutex->Unlock();
}

void c64PostRunStartupCallbacks()
{
	c64DebuggerStartupTasksCallbacksMutex->Lock();
	if (!c64DebuggerStartupTasksCallbacks.empty())
	{
		LOGD("post-run c64DebuggerStartupTasksCallbacks");
		while (!c64DebuggerStartupTasksCallbacks.empty())
		{
			C64DebuggerStartupTaskCallback *callback = c64DebuggerStartupTasksCallbacks.front();
			callback->PostRunStartupTaskCallback();

			c64DebuggerStartupTasksCallbacks.pop_front();
			delete callback;
		}
		LOGD("post-run c64DebuggerStartupTasksCallbacks completed, tasks deleted");
	}
	c64DebuggerStartupTasksCallbacksMutex->Unlock();
}

void c64PerformStartupTasksThreaded()
{
	LOGM("START c64PerformStartupTasksThreaded");

	SYS_Sleep(100);
	guiMain->SetApplicationWindowAlwaysOnTop(c64SettingsWindowAlwaysOnTop);

	LOGD("c64CommandLineWindowFullScreen=%s", STRBOOL(c64CommandLineWindowFullScreen));

	if (c64CommandLineWindowFullScreen)
	{
		viewC64->GoFullScreen(SetFullScreenMode::MainWindowEnterFullScreen, NULL);
	}

	c64PreRunStartupCallbacks();

	if (c64CommandLineAudioOutDevice != NULL)
	{
		gSoundEngine->LockMutex("c64PerformStartupTasksThreaded/c64CommandLineAudioOutDevice");
		char *cDeviceName = c64CommandLineAudioOutDevice->GetStdASCII();
		if (!gSoundEngine->SetOutputAudioDevice(cDeviceName))
		{
			printInfo("Selected sound out device not found, falling back to default output.\n");
		}
		delete [] cDeviceName;
		gSoundEngine->UnlockMutex("c64PerformStartupTasksThreaded/c64CommandLineAudioOutDevice");
	}

	// load breakpoints & symbols
	LOGTODO("c64PerformStartupTasksThreaded: SYMBOLS & BREAKPOINTS FOR BOTH ATARI & C64 + NES. GENERALIZE ME");

	CDebugInterface *debugInterface = NULL;
	if (viewC64->debugInterfaceC64)
	{
		debugInterface = viewC64->debugInterfaceC64;
	}
	else if (viewC64->debugInterfaceAtari)
	{
		debugInterface = viewC64->debugInterfaceAtari;
	}

	if (debugInterface)
	{
		if (c64SettingsPathToBreakpoints != NULL)
		{
			debugInterface->symbols->DeleteAllBreakpoints();
			debugInterface->symbols->ParseBreakpoints(c64SettingsPathToBreakpoints);
		}

		if (c64SettingsPathToSymbols != NULL)
		{
			debugInterface->symbols->DeleteAllSymbols();
			debugInterface->symbols->ParseSymbols(c64SettingsPathToSymbols);
		}

		if (c64SettingsPathToWatches != NULL)
		{
			debugInterface->symbols->DeleteAllWatches();
			debugInterface->symbols->ParseWatches(c64SettingsPathToWatches);
		}

		if (c64SettingsPathToDebugInfo != NULL)
		{
			debugInterface->symbols->DeleteAllSymbols();
			debugInterface->symbols->ParseSourceDebugInfo(c64SettingsPathToDebugInfo);
		}
	}

	// skip any automatic loading if jukebox is active
	if (viewC64->viewJukeboxPlaylist != NULL)
	{
		if (c64SettingsSIDEngineModel != 0)
		{
			//viewC64->debugInterfaceC64->LockMutex();
			gSoundEngine->LockMutex("c64PerformStartupTasksThreaded/viewJukeboxPlaylist");
			viewC64->debugInterfaceC64->SetSidType(c64SettingsSIDEngineModel);
			gSoundEngine->UnlockMutex("c64PerformStartupTasksThreaded/viewJukeboxPlaylist");
			//viewC64->debugInterfaceC64->UnlockMutex();
		}

		viewC64->viewJukeboxPlaylist->StartPlaylist();

		c64PostRunStartupCallbacks();

		return;
	}

	if (viewC64->debugInterfaceC64)
	{
		// process, order is important
		// we need to create new strings for path as they will be deleted and updated by loaders
		if (c64SettingsPathToViceSnapshot != NULL)
		{
			viewC64->viewC64Snapshots->LoadSnapshot(c64SettingsPathToViceSnapshot, false, viewC64->debugInterfaceC64);
			SYS_Sleep(150);
		}
		else
		{
			// setup SID
			if (c64SettingsSIDEngineModel != 0)
			{
				gSoundEngine->LockMutex("c64PerformStartupTasksThreaded");
				viewC64->debugInterfaceC64->SetSidType(c64SettingsSIDEngineModel);
				gSoundEngine->UnlockMutex("c64PerformStartupTasksThreaded");
			}

			if (c64SettingsPathToD64 != NULL)
			{
				LOGD("isPRGInCommandLine=%s", STRBOOL(isPRGInCommandLine));
				if (!isPRGInCommandLine && isD64InCommandLine)
				{
					// start disk based on settings
					if (c64SettingsAutoJmpFromInsertedDiskFirstPrg)
					{
						SYS_Sleep(100);
					}
					viewC64->mainMenuHelper->InsertD64(c64SettingsPathToD64, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
				}
				else
				{
					// just load disk, do not start, we will start PRG instead
					viewC64->mainMenuHelper->InsertD64(c64SettingsPathToD64, false, false, 0, false);
				}
			}

			if (c64SettingsPathToTAP != NULL)
			{
				// just load tape, do not start
				viewC64->mainMenuHelper->LoadTape(c64SettingsPathToTAP, false, false, false);
			}

			if (c64SettingsPathToCartridge != NULL)
			{
				viewC64->mainMenuHelper->InsertCartridge(c64SettingsPathToCartridge, false);
				SYS_Sleep(666);
			}

			LOGD("c64PerformStartupTasksThreaded: c64SettingsPathToReu=%p", c64SettingsPathToReu);
			if (c64SettingsPathToReu != NULL)
			{
				c64SettingsPathToReu->DebugPrint("c64SettingsPathToReu=");
				viewC64->mainMenuHelper->AttachReu(c64SettingsPathToReu, false, false);
				C64DebuggerSetSetting("ReuEnabled", &c64SettingsReuEnabled);
				SYS_Sleep(100);
			}
		}

		if (c64SettingsPathToPRG != NULL)
		{
			LOGD("c64PerformStartupTasksThreaded: loading PRG, isPRGInCommandLine=%s isD64InCommandLine=%s", STRBOOL(isPRGInCommandLine), STRBOOL(isD64InCommandLine));
			c64SettingsPathToPRG->DebugPrint("c64SettingsPathToPRG=");

			if ((!isPRGInCommandLine) && (isD64InCommandLine) && c64SettingsAutoJmpFromInsertedDiskFirstPrg)
			{
				// do not load prg when disk inserted from command line and autostart
			}
			else
			{
				viewC64->mainMenuHelper->LoadPRG(c64SettingsPathToPRG, cmdLineOptionDoAutoJmp, false, true, false);
			}
		}

		if (c64CommandLineHardReset)
		{
			viewC64->debugInterfaceC64->ResetHard();
		}

		if (c64SettingsEmulatedMouseC64Enabled)
		{
			viewC64->debugInterfaceC64->EmulatedMouseUpdateSettings();
		}
	}

	// Atari
	if (viewC64->debugInterfaceAtari)
	{
		if (c64SettingsPathToATR != NULL)
		{
			LOGD("isXEXInCommandLine=%s", STRBOOL(isXEXInCommandLine));
			if (!isXEXInCommandLine && isATRInCommandLine)
			{
				if (c64SettingsAutoJmpFromInsertedDiskFirstPrg)
				{
					SYS_Sleep(100);
				}
				viewC64->mainMenuHelper->InsertATR(c64SettingsPathToATR, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
			}
			else
			{
				viewC64->mainMenuHelper->InsertATR(c64SettingsPathToATR, false, false, 0, false);
			}
		}

		// TODO: Atari TAPE files and Cartridge support not implemented yet here

		if (c64CommandLineHardReset)
		{
			viewC64->debugInterfaceAtari->ResetHard();
		}

		if (c64SettingsPathToXEX != NULL)
		{
			LOGD("c64PerformStartupTasksThreaded: loading XEX, isXEXInCommandLine=%s isATRInCommandLine=%s", STRBOOL(isXEXInCommandLine), STRBOOL(isATRInCommandLine));
			c64SettingsPathToXEX->DebugPrint("c64SettingsPathToXEX=");

			if ((!isXEXInCommandLine) && (isATRInCommandLine) && c64SettingsAutoJmpFromInsertedDiskFirstPrg)
			{
				// do not load xex when disk inserted from command line and autostart
			}
			else
			{
				viewC64->mainMenuHelper->LoadXEX(c64SettingsPathToXEX, cmdLineOptionDoAutoJmp, false, true);
			}
		}
	}

	if (viewC64->debugInterfaceNes)
	{
		if (c64SettingsPathToNES != NULL)
		{
			viewC64->mainMenuHelper->LoadNES(c64SettingsPathToNES, false);
		}
	}

	if (c64SettingsJmpOnStartupAddr > 0 && c64SettingsJmpOnStartupAddr < 0x10000)
	{
		LOGD("c64PerformStartupTasksThreaded: c64SettingsJmpOnStartupAddr=%04x", c64SettingsJmpOnStartupAddr);

		if (viewC64->debugInterfaceC64)
			viewC64->debugInterfaceC64->MakeJsrC64(c64SettingsJmpOnStartupAddr);
	}

	c64PostRunStartupCallbacks();
}

class C64PerformStartupTasksThread : public CSlrThread
{
	virtual void ThreadRun(void *data)
	{
		ThreadSetName("PerformStartupTasks");
		LOGM("C64PerformStartupTasksThread: ThreadRun");

		if (c64SettingsPathToViceSnapshot != NULL && c64SettingsWaitOnStartup < 150)
			c64SettingsWaitOnStartup = 150;

		if (c64SettingsPathToAtariSnapshot != NULL && c64SettingsWaitOnStartup < 150)
			c64SettingsWaitOnStartup = 150;

		if (c64dStartupTime == 0 || (SYS_GetCurrentTimeInMillis() - c64dStartupTime < 100))
		{
			LOGD("C64PerformStartupTasksThread: early run, wait 100ms");
			c64SettingsWaitOnStartup += 100;
		}

		LOGD("C64PerformStartupTasksThread: c64SettingsWaitOnStartup=%d", c64SettingsWaitOnStartup);
		SYS_Sleep(c64SettingsWaitOnStartup);

		c64PerformStartupTasksThreaded();
	}
};

void C64DebuggerParseCommandLine2()
{
	LOGD("C64DebuggerParseCommandLine2");

	if (sysCommandLineArguments.empty())
		return;

	c64cmdIt = sysCommandLineArguments.begin();

	LOGD("C64DebuggerParseCommandLine2: iterate");
	while(c64cmdIt != sysCommandLineArguments.end())
	{
		char *cmd = c64ParseCommandLineGetArgument();

		//LOGD("...cmd='%s'", cmd);

		if (cmd[0] == '-')
			++cmd;

		if (!strcmp(cmd, "c64"))
		{
			c64SettingsSelectEmulator = EMULATOR_TYPE_C64_VICE;
		}
		else if (!strcmp(cmd, "atari"))
		{
			c64SettingsSelectEmulator = EMULATOR_TYPE_ATARI800;
		}
		else if (!strcmp(cmd, "nes"))
		{
			c64SettingsSelectEmulator = EMULATOR_TYPE_NESTOPIA;
		}
		else if (!strcmp(cmd, "breakpoints") || !strcmp(cmd, "b"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToBreakpoints = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "symbols") || !strcmp(cmd, "vicesymbols") || !strcmp(cmd, "vs"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToSymbols = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "watch") || !strcmp(cmd, "w"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToWatches = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "debuginfo"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToDebugInfo = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "autojmp") || !strcmp(cmd, "autojump"))
		{
			cmdLineOptionDoAutoJmp = true;
		}
		else if (!strcmp(cmd, "unpause"))
		{
			c64SettingsForceUnpause = true;
		}
		else if (!strcmp(cmd, "autorundisk"))
		{
			c64SettingsAutoJmpFromInsertedDiskFirstPrg = true;
		}
		else if (!strcmp(cmd, "alwaysjmp") || !strcmp(cmd, "alwaysjump"))
		{
			c64SettingsAutoJmpAlwaysToLoadedPRGAddress = true;
		}
		else if (!strcmp(cmd, "d64"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToD64 = new CSlrString(arg);
			isD64InCommandLine = true;
		}
		else if (!strcmp(cmd, "tap"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToTAP = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "prg"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			LOGD("C64DebuggerParseCommandLine2: set c64SettingsPathToPRG=%s", arg);
			c64SettingsPathToPRG = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "cartridge") || !strcmp(cmd, "crt"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToCartridge = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "reu"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			LOGD("C64DebuggerParseCommandLine2: -reu found, path=%s", arg);
			c64SettingsPathToReu = new CSlrString(arg);
			c64SettingsReuEnabled = true;
		}
		else if (!strcmp(cmd, "snapshot"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			c64SettingsPathToViceSnapshot = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "xex"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			LOGD("C64DebuggerParseCommandLine2: set c64SettingsPathToXEX=%s", arg);
			c64SettingsPathToXEX = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "atr"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			LOGD("C64DebuggerParseCommandLine2: set c64SettingsPathToATR=%s", arg);
			c64SettingsPathToATR = new CSlrString(arg);
		}
		else if (!strcmp(cmd, "nes"))
		{
			char *arg = c64ParseCommandLineGetArgument();
			LOGD("C64DebuggerParseCommandLine2: set c64SettingsPathToNES=%s", arg);
			c64SettingsPathToNES = new CSlrString(arg);
		}

		else if (!strcmp(cmd, "jmp"))
		{
			int addr = 0;
			char *str = c64ParseCommandLineGetArgument();

			if (str[0] == '$' || str[0] == 'x')
			{
				str++;
				sscanf(str, "%x", &addr);
			}
			else
			{
				sscanf(str, "%d", &addr);
			}

			c64SettingsJmpOnStartupAddr = addr;
		}
		else if (!strcmp(cmd, "layout"))
		{
			char *str = c64ParseCommandLineGetArgument();
			int layoutId = atoi(str) - 1;
			if (layoutId < 0)
				layoutId = 0;
			c64SettingsDefaultScreenLayoutId = layoutId;

			LOGD("c64SettingsDefaultScreenLayoutId=%d", layoutId);
		}
		else if (!strcmp(cmd, "wait"))
		{
			char *str = c64ParseCommandLineGetArgument();
			c64SettingsWaitOnStartup = atoi(str);
		}
		else if (!strcmp(cmd, "soundout"))
		{
			char *str = c64ParseCommandLineGetArgument();
			LOGD("soundout='%s'", str);
			c64CommandLineAudioOutDevice = new CSlrString(str);
		}
		else if (!strcmp(cmd, "playlist") || !strcmp(cmd, "jukebox"))
		{
			char *str = c64ParseCommandLineGetArgument();
			LOGD("playlist='%s'", str);
			c64SettingsPathToJukeboxPlaylist = new CSlrString(str);
		}
		else if (!strcmp(cmd, "reset"))
		{
			c64CommandLineHardReset = true;
		}
		else if (!strcmp(cmd, "fullscreen"))
		{
			c64CommandLineWindowFullScreen = true;
		}
		// TODO: fixme, plugins should also parse command line options
		else if (!strcmp(cmd, "crtmaker"))
		{
			char *str = c64ParseCommandLineGetArgument();
			LOGD("crtMakerConfigFilePath='%s'", str);
			crtMakerConfigFilePath = STRALLOC(str);
			c64SettingsPathToCartridge = NULL;
		}
	}
}

void C64DebuggerPerformStartupTasks()
{
	LOGM("C64DebuggerPerformStartupTasks()");
	C64PerformStartupTasksThread *thread = new C64PerformStartupTasksThread();
	SYS_StartThread(thread, NULL);
}

void C64DebuggerPassConfigToRunningInstance()
{
	if (!c64CommandLineSkipConsoleAttach)
	{
		SYS_AttachConsole();
	}

	c64SettingsPassConfigToRunningInstance = true;
	printLine("-----< RetroDebugger v%s by Slajerek/Samar >------\n", RETRODEBUGGER_VERSION_STRING);
	fflush(stdout);

	LOGD("C64DebuggerPassConfigToRunningInstance: C64DebuggerParseCommandLine2");
	c64SettingsForceUnpause = false;
	C64DebuggerParseCommandLine2();

	LOGD("C64DebuggerPassConfigToRunningInstance: after C64DebuggerParseCommandLine2");

	CByteBuffer *byteBuffer = new CByteBuffer();
	LOGD("...C64D_PASS_CONFIG_DATA_MARKER");
	byteBuffer->PutU16(C64D_PASS_CONFIG_DATA_MARKER);
	byteBuffer->PutU16(C64D_PASS_CONFIG_DATA_VERSION);

	LOGD("... put folder");
	gUTFPathToCurrentDirectory->DebugPrint("gUTFPathToCurrentDirectory=");
	byteBuffer->PutSlrString(gUTFPathToCurrentDirectory);

	if (c64SettingsSelectEmulator != EMULATOR_TYPE_UNKNOWN)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_SELECT_EMULATOR);
		byteBuffer->PutU8(c64SettingsSelectEmulator);
	}

	if (c64SettingsPathToViceSnapshot)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_LOAD_SNAPSHOT_VICE);
		byteBuffer->PutSlrString(c64SettingsPathToViceSnapshot);
	}

	if (c64SettingsPathToBreakpoints)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_BREAKPOINTS_FILE);
		byteBuffer->PutSlrString(c64SettingsPathToBreakpoints);
	}

	if (c64SettingsPathToSymbols)
	{
		LOGD("c64SettingsPathToSymbols");
		c64SettingsPathToSymbols->DebugPrint("c64SettingsPathToSymbols=");
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_SYMBOLS_FILE);
		byteBuffer->PutSlrString(c64SettingsPathToSymbols);
	}

	if (c64SettingsPathToWatches)
	{
		LOGD("c64SettingsPathToWatches");
		c64SettingsPathToWatches->DebugPrint("c64SettingsPathToWatches=");
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_WATCHES_FILE);
		byteBuffer->PutSlrString(c64SettingsPathToWatches);
	}

	if (c64SettingsPathToDebugInfo)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_DEBUG_INFO);
		byteBuffer->PutSlrString(c64SettingsPathToDebugInfo);
	}

	if (c64CommandLineHardReset)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_HARD_RESET);
	}

	if (c64SettingsWaitOnStartup > 0)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_WAIT);
		byteBuffer->putInt(c64SettingsWaitOnStartup);
	}

	if (c64SettingsPathToCartridge)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_CRT);
		byteBuffer->PutSlrString(c64SettingsPathToCartridge);
	}

	if (c64SettingsPathToReu)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_REU);
		byteBuffer->PutSlrString(c64SettingsPathToReu);
	}

	if (c64SettingsPathToD64)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_D64);
		byteBuffer->PutSlrString(c64SettingsPathToD64);
	}

	if (c64SettingsPathToTAP)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_TAP);
		byteBuffer->PutSlrString(c64SettingsPathToTAP);
	}

	if (c64SettingsPathToXEX)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_XEX);
		byteBuffer->PutSlrString(c64SettingsPathToXEX);
	}

	if (c64SettingsPathToNES)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_NES);
		byteBuffer->PutSlrString(c64SettingsPathToNES);
	}

	if (c64SettingsPathToATR)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_ATR);
		byteBuffer->PutSlrString(c64SettingsPathToATR);
	}

	if (cmdLineOptionDoAutoJmp)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_SET_AUTOJMP);
		byteBuffer->PutBool(cmdLineOptionDoAutoJmp);
	}

	if (c64SettingsForceUnpause)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_FORCE_UNPAUSE);
		byteBuffer->PutBool(c64SettingsForceUnpause);
	}

	if (c64SettingsAutoJmpFromInsertedDiskFirstPrg)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_AUTO_RUN_DISK);
		byteBuffer->PutBool(c64SettingsAutoJmpFromInsertedDiskFirstPrg);
	}

	if (c64SettingsAutoJmpAlwaysToLoadedPRGAddress)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_ALWAYS_JMP);
		byteBuffer->PutBool(c64SettingsAutoJmpAlwaysToLoadedPRGAddress);
	}

	if (c64SettingsPathToPRG)
	{
		LOGD("c64SettingsPathToPRG");
		c64SettingsPathToPRG->DebugPrint("c64SettingsPathToPRG=");

		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_PATH_TO_PRG);
		byteBuffer->PutSlrString(c64SettingsPathToPRG);
	}

	if (c64SettingsJmpOnStartupAddr >= 0)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_JMP);
		byteBuffer->putInt(c64SettingsJmpOnStartupAddr);
	}

	if (c64SettingsDefaultScreenLayoutId >= 0)
	{
		LOGD("c64SettingsDefaultScreenLayoutId=%d", c64SettingsDefaultScreenLayoutId);
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_LAYOUT);
		byteBuffer->putInt(c64SettingsDefaultScreenLayoutId);
	}

	if (c64CommandLineAudioOutDevice != NULL)
	{
		LOGD("c64CommandLineAudioOutDevice");
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_SOUND_DEVICE_OUT);
		byteBuffer->PutSlrString(c64CommandLineAudioOutDevice);
	}

	if (c64CommandLineWindowFullScreen)
	{
		byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_FULL_SCREEN);
	}

	LOGD("...C64D_PASS_CONFIG_DATA_EOF");

	byteBuffer->PutU8(C64D_PASS_CONFIG_DATA_EOF);

	int pid = C64DebuggerSendConfiguration(byteBuffer);
	if (pid > 0)
	{
		printLine("Parameters sent to instance pid=%d. Bye.\n", pid);
		fflush(stdout);
		SYS_CleanExit();
	}
	else
	{
		printLine("Other instance was not found, performing regular startup instead.\n");
		fflush(stdout);
	}
}

void c64PerformNewConfigurationTasksThreaded(CByteBuffer *byteBuffer)
{
	LOGD("c64PerformNewConfigurationTasksThreaded");
	//byteBuffer->DebugPrint();
	byteBuffer->Rewind();

	u16 marker = byteBuffer->GetU16();
	if (marker != C64D_PASS_CONFIG_DATA_MARKER)
	{
		LOGError("Config data marker not found (received %04x, should be %04x)", marker, C64D_PASS_CONFIG_DATA_MARKER);
		delete byteBuffer; // prevent leak
		return;
	}

	u16 v = byteBuffer->GetU16();
	if (v != C64D_PASS_CONFIG_DATA_VERSION)
	{
		LOGError("Config data version not correct (received %04x, should be %04x)", v, C64D_PASS_CONFIG_DATA_VERSION);
		delete byteBuffer; // prevent leak
		return;
	}

	CSlrString *currentFolder = byteBuffer->GetSlrString();

	LOGD("... got folder");
	currentFolder->DebugPrint("currentFolder=");

	SYS_SetCurrentFolder(currentFolder);

	delete currentFolder;

	CDebugInterface *debugInterface = NULL;
	if (!viewC64->debugInterfaces.empty())
		debugInterface = viewC64->debugInterfaces.front();

	// select default emulator as the first that is running
	for (std::vector<CDebugInterface *>::iterator it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); ++it)
	{
		CDebugInterface *d = *it;
		if (d->isRunning)
		{
			debugInterface = d;
			break;
		}
	}

	if (viewC64->config->GetBool("uiRaiseWindowOnPass", true))
	{
		guiMain->RaiseMainWindow();
	}

	while(!byteBuffer->IsEof())
	{
		uint8 t = byteBuffer->GetU8();

		if (t == C64D_PASS_CONFIG_DATA_EOF)
			break;

		LOGD("Process passed message=%d", t);

		if (t == C64D_PASS_CONFIG_DATA_WAIT)
		{
			int wait = byteBuffer->getInt();
			LOGD("C64D_PASS_CONFIG_DATA_WAIT: %dms", wait);
			SYS_Sleep(wait);
		}
		else if (t == C64D_PASS_CONFIG_DATA_SELECT_EMULATOR)
		{
			u8 emulatorType = byteBuffer->GetU8();
			c64SettingsSelectEmulator = emulatorType;
			debugInterface = viewC64->GetDebugInterface(emulatorType);
			if (debugInterface && (debugInterface->isRunning == false))
			{
				viewC64->StartEmulationThread(debugInterface);
			}
		}
		else if (t == C64D_PASS_CONFIG_DATA_LOAD_SNAPSHOT_VICE)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (viewC64->viewC64Snapshots && viewC64->debugInterfaceC64)
				viewC64->viewC64Snapshots->LoadSnapshot(str, false, viewC64->debugInterfaceC64);
			delete str;

			SYS_Sleep(150);
		}
		else if (t == C64D_PASS_CONFIG_DATA_LOAD_SNAPSHOT_ATARI800)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (viewC64->viewC64Snapshots && viewC64->debugInterfaceAtari)
				viewC64->viewC64Snapshots->LoadSnapshot(str, false, viewC64->debugInterfaceAtari);
			delete str;

			SYS_Sleep(150);
		}
		else if (t == C64D_PASS_CONFIG_DATA_LOAD_SNAPSHOT_NESTOPIA)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (viewC64->viewC64Snapshots && viewC64->debugInterfaceNes)
				viewC64->viewC64Snapshots->LoadSnapshot(str, false, viewC64->debugInterfaceNes);
			delete str;

			SYS_Sleep(150);
		}
		else if (t == C64D_PASS_CONFIG_DATA_BREAKPOINTS_FILE)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (debugInterface && debugInterface->symbols)
			{
				debugInterface->symbols->DeleteAllBreakpoints();
				debugInterface->symbols->ParseBreakpoints(str);
			}
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_SYMBOLS_FILE)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (debugInterface && debugInterface->symbols)
			{
				debugInterface->symbols->DeleteAllSymbols();
				debugInterface->symbols->ParseSymbols(str);
			}
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_WATCHES_FILE)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (debugInterface && debugInterface->symbols)
			{
				debugInterface->symbols->DeleteAllWatches();
				debugInterface->symbols->ParseWatches(str);
			}
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_DEBUG_INFO)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			if (debugInterface && debugInterface->symbols)
			{
				debugInterface->symbols->DeleteAllSymbols();
				debugInterface->symbols->ParseSourceDebugInfo(str);
			}
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_D64)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->InsertD64(str, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_TAP)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->LoadTape(str, false, false, true);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_CRT)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->InsertCartridge(str, false);
			SYS_Sleep(666);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_REU)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->AttachReu(str, false, false);
			C64DebuggerSetSetting("ReuEnabled", &c64SettingsReuEnabled);
			SYS_Sleep(100);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_XEX)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->LoadXEX(str, cmdLineOptionDoAutoJmp, false, true);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_ATR)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->InsertATR(str, false, c64SettingsAutoJmpFromInsertedDiskFirstPrg, 0, true);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_NES)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->LoadNES(str, true);
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_SET_AUTOJMP)
		{
			bool b = byteBuffer->GetBool();
			cmdLineOptionDoAutoJmp = b;
		}
		else if (t == C64D_PASS_CONFIG_DATA_FORCE_UNPAUSE)
		{
			bool b = byteBuffer->GetBool();
			c64SettingsForceUnpause = b;
		}
		else if (t == C64D_PASS_CONFIG_DATA_AUTO_RUN_DISK)
		{
			bool b = byteBuffer->GetBool();
			c64SettingsAutoJmpFromInsertedDiskFirstPrg = b;
		}
		else if (t == C64D_PASS_CONFIG_DATA_ALWAYS_JMP)
		{
			bool b = byteBuffer->GetBool();
			c64SettingsAutoJmpAlwaysToLoadedPRGAddress = b;
		}
		else if (t == C64D_PASS_CONFIG_DATA_PATH_TO_PRG)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			viewC64->mainMenuHelper->LoadPRG(str, cmdLineOptionDoAutoJmp, false, true, false);
			delete str;
		}
//		else if (t == C64D_PASS_CONFIG_DATA_LAYOUT)
//		{
//			int layoutId = byteBuffer->getInt();
//			c64SettingsDefaultScreenLayoutId = layoutId;
//			if (c64SettingsDefaultScreenLayoutId >= SCREEN_LAYOUT_MAX)
//			{
//				c64SettingsDefaultScreenLayoutId = SCREEN_LAYOUT_C64_DEBUGGER;
//			}
//			LOGError("Layout selection not supported yet");
////			viewC64->SwitchToScreenLayout(c64SettingsDefaultScreenLayoutId);
//			
//		}
		else if (t == C64D_PASS_CONFIG_DATA_JMP)
		{
			int jmpAddr = byteBuffer->getInt();
			viewC64->debugInterfaceC64->MakeJsrC64(jmpAddr);
		}
		else if (t == C64D_PASS_CONFIG_DATA_SOUND_DEVICE_OUT)
		{
			CSlrString *str = byteBuffer->GetSlrString();
			char *cDeviceName = str->GetStdASCII();
			if (gSoundEngine->SetOutputAudioDevice(cDeviceName) == false)
			{
				printInfo("Selected sound out device not found, fall back to default output.\n");
			}
			delete [] cDeviceName;			
			delete str;
		}
		else if (t == C64D_PASS_CONFIG_DATA_HARD_RESET)
		{
			debugInterface->ResetHard();
		}
		else if (t == C64D_PASS_CONFIG_DATA_FULL_SCREEN)
		{
			viewC64->GoFullScreen(SetFullScreenMode::MainWindowEnterFullScreen, NULL);
		}
	}
	
	if (c64SettingsForceUnpause)
	{
		if (viewC64->debugInterfaceC64)
		{
			viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
		if (viewC64->debugInterfaceAtari)
		{
			viewC64->debugInterfaceAtari->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
		if (viewC64->debugInterfaceNes)
		{
			viewC64->debugInterfaceNes->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
	}
	
	//viewC64->ShowMessage("updated");
	delete byteBuffer;
}

class C64PerformNewConfigurationTasksThread : public CSlrThread
{
	virtual void ThreadRun(void *data)
	{
		LOGD("C64PerformNewConfigurationTasksThread: ThreadRun");
		CByteBuffer *byteBuffer = (CByteBuffer *)data;
		c64PerformNewConfigurationTasksThreaded(byteBuffer);
	};
};

void C64DebuggerPerformNewConfigurationTasks(CByteBuffer *byteBuffer)
{
	CByteBuffer *copyByteBuffer = new CByteBuffer(byteBuffer);

	C64PerformNewConfigurationTasksThread *thread = new C64PerformNewConfigurationTasksThread();
	SYS_StartThread(thread, copyByteBuffer);
}

void C64DebuggerStartupTaskCallback::PreRunStartupTaskCallback()
{
}

void C64DebuggerStartupTaskCallback::PostRunStartupTaskCallback()
{
}

