#include "CDebugInterfaceC64U.h"
#include "C64UTestFixture.h"
#include "CSnapshotsManager.h"
#include "CDebugSymbolsC64.h"
#include "CDebugMemory.h"
#include "C64SettingsStorage.h"
#include "State/C64UMemoryCache.h"
#include "State/C64UMediaStatusCache.h"
#include "State/C64ULogicalStateCache.h"
#include "State/C64URomBypass.h"
#include "Adapters/CDataAdapterC64U.h"
#include "Adapters/CDataAdapterC64UDirectRamBestEffort.h"
#include "Adapters/CDataAdapterC64UVicRam.h"
#include "Transport/C64URestClient.h"
#include "Transport/C64UPort64Client.h"
#include "Transport/C64UVideoStream.h"
#include "Transport/C64UDebugStream.h"
#include "Transport/C64UAudioStream.h"
#include "Transport/C64UFtpClient.h"
#include "Transport/C64ULocalIpDetect.h"
#include "Trace/C64UTraceBuffer.h"
#include "Trace/C64U6502Decoder.h"
#include "Audio/C64UAudioJitterBuffer.h"
#include "Audio/CAudioChannelC64U.h"
#include "SND_Main.h"
#include "C64Palette.h"
#include "CDebuggerApiC64U.h"

#include "SYS_Funct.h"
#include <chrono>

static uint64_t C64U_GetCurrentTimeMillis()
{
	return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

CDebugInterfaceC64U::CDebugInterfaceC64U(CViewC64 *viewC64)
	: CDebugInterfaceC64(viewC64),
	  connectionStatus((int)C64U_CONNECTION_STATUS_DISCONNECTED),
	  reconnectRetries(0),
	  needsReconnect(false),
	  lastVideoStreamRetryMs(0),
	  endpointHost("192.168.1.64"),
	  endpointPassword(""),
	  endpointHttpPort(c64SettingsC64UHttpPort),
	  endpointTcpPort(c64SettingsC64UTcpPort),
	  endpointVideoPort(c64SettingsC64UVideoPort)
{
	// Read string settings from globals (they are initialized by C64DebuggerRestoreSettings)
	if (c64SettingsC64UHostname)
	{
		char *tmp = c64SettingsC64UHostname->GetStdASCII();
		endpointHost = tmp;
		delete [] tmp;
	}
	if (c64SettingsC64UPassword)
	{
		char *tmp = c64SettingsC64UPassword->GetStdASCII();
		endpointPassword = tmp;
		delete [] tmp;
	}

	isRunning = false;
	modeManager.SetMode(C64U_MODE_DISCONNECTED);
	debugMode = DEBUGGER_MODE_RUNNING;
	mediaStatus = "No media mounted";
	snapshotsManager = new CSnapshotsManager(this);

	// Memory cache and data adapters
	memoryCache = new C64UMemoryCache();
	mediaStatusCache = new C64UMediaStatusCache();
	logicalStateCache = new C64ULogicalStateCache();
	logicalStateCache->SetMemoryCache(memoryCache);

	symbolsC64 = new CDebugSymbolsC64(this);
	symbols = symbolsC64;

	CDataAdapterC64U *c64uAdapter = new CDataAdapterC64U(symbols, memoryCache);
	dataAdapterC64 = c64uAdapter;
	symbols->SetDataAdapter(dataAdapterC64);
	symbols->CreateDefaultSegment();

	CDataAdapterC64UDirectRamBestEffort *directRamAdapter = new CDataAdapterC64UDirectRamBestEffort(this, symbols, memoryCache);
	dataAdapterC64DirectRam = directRamAdapter;

	dataAdapterC64UVicRam = new CDataAdapterC64UVicRam(this, symbols, memoryCache);

	traceMemoryDrainIndex = 0;

	// Transport
	restClient = new C64URestClient();
	port64Client = new C64UPort64Client();
	videoStream = new C64UVideoStream();
	ftpClient = new C64UFtpClient();

	// Audio stream and jitter buffer
	int audioBufferSamples = (int)(47983.0f * (float)c64SettingsC64UAudioBufferMs / 1000.0f);
	if (audioBufferSamples < 4800)
		audioBufferSamples = 4800;
	audioJitterBuffer = new C64UAudioJitterBuffer(audioBufferSamples);
	audioStream = new C64UAudioStream();
	audioStream->SetJitterBuffer(audioJitterBuffer);
	audioStream->SetEndpoint(endpointHost, endpointVideoPort + 1);
	audioStream->SetUseMulticast(c64SettingsC64UUseMulticast);
	audioChannelC64U = new CAudioChannelC64U(this);
	audioChannelC64U->jitterBuffer = audioJitterBuffer;
	audioChannelC64U->prebufferThreshold = audioBufferSamples / 2;
	SND_AddChannel(audioChannelC64U);

	// Debug stream and trace
	int traceCapacity = c64SettingsC64UTraceBufferSize * 1024 * 1024;
	if (traceCapacity < 1024 * 1024)
		traceCapacity = 4 * 1024 * 1024;
	traceBuffer = new C64UTraceBuffer(traceCapacity);
	debugStream = new C64UDebugStream();
	debugStream->SetTraceBuffer(traceBuffer);
	decoder6510 = new C64U6502Decoder();
	romBypass = new C64URomBypass(restClient);
	memoryCache->SetRestClient(restClient);
	restClient->SetMemoryCache(memoryCache);
	restClient->SetEndpoint(endpointHost, endpointHttpPort, endpointPassword);
	port64Client->SetEndpoint(endpointHost, endpointTcpPort);
	videoStream->SetEndpoint(endpointHost, endpointVideoPort);

	// Build LUT from the currently selected palette
	uint8_t rgb[16][3];
	if (C64GetPaletteRGB(c64SettingsVicPalette, rgb))
	{
		videoStream->RebuildLUT(rgb);
	}

	CreateScreenData();
	RefreshScreenNoCallback();
}

CDebugInterfaceC64U::~CDebugInterfaceC64U()
{
	if (audioChannelC64U)
	{
		SND_RemoveChannel(audioChannelC64U);
		delete audioChannelC64U;
	}
	delete audioStream;
	delete audioJitterBuffer;
	delete decoder6510;
	delete debugStream;
	delete traceBuffer;
	delete romBypass;
	delete ftpClient;
	delete videoStream;
	delete port64Client;
	delete restClient;
	delete logicalStateCache;
	delete mediaStatusCache;

	// Signal the cache that no new lock attempts should start, then give any
	// thread that already passed the flag check time to complete its lock.
	if (memoryCache)
		memoryCache->BeginShutdown();
	SYS_Sleep(10);
	delete memoryCache;
}

int CDebugInterfaceC64U::GetEmulatorType()
{
	return EMULATOR_TYPE_C64U;
}

CSlrString *CDebugInterfaceC64U::GetEmulatorVersionString()
{
	return new CSlrString("Ultimate 64");
}

const char *CDebugInterfaceC64U::GetPlatformNameString()
{
	return "C64U";
}

const char *CDebugInterfaceC64U::GetPlatformNameEndpointString()
{
	return "c64u";
}

CDebuggerApi *CDebugInterfaceC64U::GetDebuggerApi()
{
	return new CDebuggerApiC64U(this);
}

CC64BackendCapabilities CDebugInterfaceC64U::GetC64BackendCapabilities()
{
	CC64BackendCapabilities capabilities;
	capabilities.isDefaultC64Backend = false;
	capabilities.supportsScreenMode = true;
	capabilities.supportsTraceMode = true;
	capabilities.screenAndTraceAreMutuallyExclusive = true;
	capabilities.screenStream = C64CAP_DIRECT;
	capabilities.mappedMemoryRead = C64CAP_DIRECT;
	capabilities.mappedMemoryWrite = C64CAP_DIRECT;
	capabilities.keyboardTextInput = C64CAP_DIRECT;
	capabilities.joystickInput = C64CAP_HELPER;
	return capabilities;
}

CDebugInterface::CDebugBackendCapabilities CDebugInterfaceC64U::GetExecutionCapabilities()
{
	CDebugBackendCapabilities capabilities;
	capabilities.fetchMapping = GetMemoryMappingResolver() != NULL;
	capabilities.executionTrace = true;
	return capabilities;
}

int CDebugInterfaceC64U::GetScreenSizeX()
{
	return 384;
}

int CDebugInterfaceC64U::GetScreenSizeY()
{
	return 272;
}

void CDebugInterfaceC64U::GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b)
{
	uint8 rgb[16][3];
	if (C64GetPaletteRGB(c64SettingsVicPalette, rgb))
	{
		uint8 index = colorNum & 0x0F;
		*r = rgb[index][0];
		*g = rgb[index][1];
		*b = rgb[index][2];
		return;
	}

	CDebugInterfaceC64::GetCBMColor(colorNum, r, g, b);
}

void CDebugInterfaceC64U::RefreshScreenNoCallback()
{
	if (screenImageData == NULL)
		return;

	LockRenderScreenMutex();
	bool copiedFrame = videoStream->CopyLatestFrameToImage(screenImageData);
	if (!copiedFrame && videoStream->GetFrameCounter() == 0)
	{
		int factor = screenSupersampleFactor;
		for (int y = 0; y < GetScreenSizeY(); y++)
		{
			for (int x = 0; x < GetScreenSizeX(); x++)
			{
				uint8_t shade = (uint8_t)(((x / 12) + (y / 10)) & 1 ? 0x32 : 0x18);
				uint8_t r = shade;
				uint8_t g = (uint8_t)(shade + 20);
				uint8_t b = (uint8_t)(shade + 36);
				for (int fy = 0; fy < factor; fy++)
				{
					for (int fx = 0; fx < factor; fx++)
					{
						screenImageData->SetPixelResultRGBA(x * factor + fx, y * factor + fy, r, g, b, 0xff);
					}
				}
			}
		}
	}
	bool shouldPublish = copiedFrame || (videoStream->GetFrameCounter() == 0);
	UnlockRenderScreenMutex();

	if (shouldPublish)
	{
		PublishScreenImage();
	}

	// If no video frames received yet and in screen mode, periodically retry the video stream
	// start REST call. The initial attempt in Connect() may fail if the C64U is not yet
	// reachable on the network (e.g. app starts before the device is fully online).
	if (modeManager.GetMode() == C64U_MODE_SCREEN
		&& GetConnectionStatus() == C64U_CONNECTION_STATUS_STREAMING
		&& videoStream->GetFrameCounter() == 0
		&& lastVideoStreamRetryMs > 0)
	{
		uint64_t now = C64U_GetCurrentTimeMillis();
		if (now - lastVideoStreamRetryMs >= 2000)
		{
			lastVideoStreamRetryMs = now;
			std::string localIp = DetectLocalIp();
			if (!localIp.empty())
				restClient->ScheduleStreamStart("video", localIp, endpointVideoPort);
		}
	}

	// Drive demand-based memory refresh and coalesced writes
	if (memoryCache)
	{
		renderFrameCounter++;
		memoryCache->SetCurrentFrame(renderFrameCounter);

		bool inTrace = (modeManager.GetMode() == C64U_MODE_TRACE);
		if (inTrace)
		{
			// In trace mode: update memory from live bus data — non-disruptive, no REST reads needed
			DrainTraceBusData();

			// Refresh VIC/CIA/SID logical state from the now-updated memory cache
			if (logicalStateCache)
				logicalStateCache->RefreshFromMemory();
		}
		else if (c64SettingsC64UAutoRefreshMemory)
		{
			// In screen mode: demand-based REST reads (CPU PEEK — may affect running code)
			memoryCache->ScheduleVisiblePageRefreshes();
		}

		memoryCache->FlushPendingWrites();
	}

	EC64UConnectionStatus status = GetConnectionStatus();

	if (status == C64U_CONNECTION_STATUS_STREAMING && isRunning
		&& modeManager.GetMode() == C64U_MODE_SCREEN)
	{
		// Only check video timeout in screen mode — in trace mode video is intentionally stopped
		bool hasReceivedFrames = videoStream->GetFrameCounter() > 0;

		if (hasReceivedFrames && videoStream->HasTimedOut(3000))
		{
			// Was receiving frames but stopped — connection lost
			std::lock_guard<std::mutex> lock(connectionMutex);
			needsReconnect = true;
			SetConnectionStatus(C64U_CONNECTION_STATUS_DISCONNECTED);
			modeManager.SetMode(C64U_MODE_DISCONNECTED);
			SetMediaStatus("Connection lost (timeout)");
		}
		else if (!hasReceivedFrames && connectionStartTimeMillis > 0)
		{
			// Still waiting for first frame — use longer grace period (10s)
			uint64_t elapsed = C64U_GetCurrentTimeMillis() - connectionStartTimeMillis;
			if (elapsed > 10000)
			{
				std::lock_guard<std::mutex> lock(connectionMutex);
				needsReconnect = true;
				SetConnectionStatus(C64U_CONNECTION_STATUS_DISCONNECTED);
				modeManager.SetMode(C64U_MODE_DISCONNECTED);
				SetMediaStatus("Connection failed (no video received)");
			}
		}
	}
}

void CDebugInterfaceC64U::DrainTraceBusData()
{
	if (!memoryCache || !traceBuffer || !symbolsC64 || !symbolsC64->memory)
		return;

	uint64_t writeIndex = traceBuffer->GetWriteIndex();
	if (traceMemoryDrainIndex >= writeIndex)
		return;

	// Cap the number of entries processed per frame to avoid stalls
	// At 1MHz CPU and 60fps, ~16000 entries per frame. 50000 is a generous cap.
	const uint64_t maxDrainPerFrame = 50000;
	uint64_t endIndex = writeIndex;
	if (endIndex - traceMemoryDrainIndex > maxDrainPerFrame)
		endIndex = traceMemoryDrainIndex + maxDrainPerFrame;

	CDebugMemory *debugMemory = symbolsC64->memory;
	bool observed = HasExecutionObservers();

	for (uint64_t idx = traceMemoryDrainIndex; idx < endIndex; idx++)
	{
		C64UDebugEntry entry = traceBuffer->GetEntry(idx);

		// Drive the shared decoder for PC tracking (respects its own mode filter)
		// Capture the annotation so we know when the CPU is fetching an opcode.
		C64UDecoderAnnotation ann;
		bool annotated = false;
		if (decoder6510->ShouldProcessEntry(entry))
		{
			ann = decoder6510->ProcessEntry(entry);
			annotated = true;
		}

		// Only update memory/cells from CPU bus cycles (phi2=1 = CPU phase)
		// phi2=0 entries are VIC-II accesses in a different address space
		if (!entry.GetPhi2())
			continue;

		int address = (int)entry.address;

		// Update memory cache from observed bus data
		memoryCache->UpdateByteFromTrace(address, entry.data);

		// Animate memory map cells and maintain execute/data markers
		if (!entry.rw)
		{
			// Write: clears isExecuteCode/isExecuteArgument automatically
			debugMemory->CellWrite(address, entry.data);
		}
		else if (annotated && ann.type == C64UDecoderAnnotation::OPCODE_FETCH)
		{
			// RD-045: the instruction boundary, after the fact
			if (observed)
				ReplayInstructionBoundary((u16)address, decoder6510->AreRegsValid() ? decoder6510->GetRegSP() : 0xFF, entry.data);

			// Opcode fetch: mark this cell as execute code.
			// CellExecute also marks the argument bytes (addr+1, addr+2) based
			// on the opcode's addressing length — same as the VICE C64 backend.
			debugMemory->CellExecute(address, entry.data);
		}
		else
		{
			// Operand fetch, data read, or address calculation cycle
			debugMemory->CellRead(address);
		}
	}

	traceMemoryDrainIndex = endIndex;
}

void CDebugInterfaceC64U::RunEmulationThread()
{
	isRunning = true;

	if (C64UTestFixture::IsEnabled() || c64SettingsC64UAutoConnect)
	{
		Connect();
	}
	else
	{
		SetMediaStatus("Ready to connect");
	}
}

u64 CDebugInterfaceC64U::GetMainCpuCycleCounter()
{
	// traceMemoryDrainIndex counts processed bus cycles — a monotonic proxy
	// for the CPU cycle counter used by memory cell execute history.
	return (u64)traceMemoryDrainIndex;
}

void CDebugInterfaceC64U::GetCpuRegs(u16 *PC, u8 *A, u8 *X, u8 *Y, u8 *P, u8 *S)
{
	if (decoder6510 && decoder6510->AreRegsValid())
	{
		*PC = decoder6510->GetCurrentPC();
		*A = decoder6510->GetRegA();
		*X = decoder6510->GetRegX();
		*Y = decoder6510->GetRegY();
		*P = decoder6510->GetRegP();
		*S = decoder6510->GetRegSP();
	}
	else
	{
		*PC = 0; *A = 0; *X = 0; *Y = 0; *P = 0; *S = 0;
	}
}

void CDebugInterfaceC64U::Shutdown()
{
	Disconnect();
	isRunning = false;
	SetMediaStatus("No media mounted");
	CDebugInterfaceC64::Shutdown();
}

void CDebugInterfaceC64U::ResetSoft()
{
	restClient->ScheduleReset(false);
	SetMediaStatus("Soft reset requested");
}

void CDebugInterfaceC64U::ResetHard()
{
	restClient->ScheduleReset(true);
	SetMediaStatus("Hard reset requested");
}

bool CDebugInterfaceC64U::LoadExecutable(char *fullFilePath)
{
	if (fullFilePath == NULL)
		return false;

	// Check if this is a local file (exists on disk) — if so, read and DMA load
	FILE *f = fopen(fullFilePath, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (fileSize > 0 && fileSize < 65536)
		{
			std::vector<uint8_t> data(fileSize);
			fread(data.data(), 1, fileSize, f);
			fclose(f);
			port64Client->ScheduleDmaLoadRun(data.data(), (int)data.size());
			SetMediaStatus(std::string("DMA Load+Run: ") + fullFilePath);
			return true;
		}
		fclose(f);
	}

	// Not a local file or too large — treat as remote path
	restClient->ScheduleLoadPrg(fullFilePath, true);
	port64Client->ScheduleLoadAndRunPrg(fullFilePath);
	SetMediaStatus(std::string("PRG requested: ") + fullFilePath);
	return true;
}

bool CDebugInterfaceC64U::MountDisk(char *fullFilePath, int diskNo, bool readOnly)
{
	if (fullFilePath == NULL)
		return false;

	// Check if this is a local file — upload via FTP first, then mount by remote path
	FILE *f = fopen(fullFilePath, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (fileSize > 0)
		{
			std::vector<uint8_t> data(fileSize);
			fread(data.data(), 1, fileSize, f);
			fclose(f);

			// Extract filename from path
			std::string pathStr(fullFilePath);
			size_t lastSlash = pathStr.rfind('/');
			std::string fileName = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
			std::string remotePath = "/tmp/" + fileName;

			if (ftpClient && ftpClient->IsConnected())
			{
				ftpClient->UploadFile(remotePath, data.data(), data.size());
				restClient->ScheduleMountDisk(remotePath, diskNo);
				SetMediaStatus(std::string("Uploaded+Mounted: ") + fileName);
				return true;
			}
		}
		else
		{
			fclose(f);
		}
	}

	// Not a local file or FTP not connected — treat as remote path
	restClient->ScheduleMountDisk(fullFilePath, diskNo);
	port64Client->ScheduleMountDisk(fullFilePath, diskNo);
	SetMediaStatus(std::string("Disk ") + std::to_string(diskNo) + (readOnly ? " mounted read-only: " : " mounted: ") + fullFilePath);
	return true;
}

bool CDebugInterfaceC64U::GetSettingIsWarpSpeed()
{
	// C64U has no warp speed concept
	return false;
}

void CDebugInterfaceC64U::SetSettingIsWarpSpeed(bool isWarpSpeed)
{
	// C64U has no warp speed concept — ignore
}

void CDebugInterfaceC64U::Connect()
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	if (GetConnectionStatus() != C64U_CONNECTION_STATUS_DISCONNECTED)
		return;

	// Read current settings into transport clients
	if (c64SettingsC64UHostname)
	{
		char *tmp = c64SettingsC64UHostname->GetStdASCII();
		endpointHost = tmp;
		delete [] tmp;
	}
	if (c64SettingsC64UPassword)
	{
		char *tmp = c64SettingsC64UPassword->GetStdASCII();
		endpointPassword = tmp;
		delete [] tmp;
	}
	endpointHttpPort = c64SettingsC64UHttpPort;
	endpointTcpPort = c64SettingsC64UTcpPort;
	endpointVideoPort = c64SettingsC64UVideoPort;

	restClient->SetEndpoint(endpointHost, endpointHttpPort, endpointPassword);
	port64Client->SetEndpoint(endpointHost, endpointTcpPort);
	port64Client->SetPassword(endpointPassword);
	videoStream->SetEndpoint(endpointHost, endpointVideoPort);
	videoStream->SetUseMulticast(c64SettingsC64UUseMulticast);
	audioStream->SetEndpoint(endpointHost, endpointVideoPort + 1);
	audioStream->SetUseMulticast(c64SettingsC64UUseMulticast);

	reconnectRetries = 0;
	SetConnectionStatus(C64U_CONNECTION_STATUS_CONNECTING);

	if (C64UTestFixture::IsEnabled())
	{
		// Fixture mode: skip network, go directly to STREAMING
		restClient->SetFixtureMode(true);
		port64Client->SetFixtureMode(true);
		restClient->Start();
		port64Client->Start();
		videoStream->Start(true);
		if (c64SettingsC64UAudioEnabled)
		{
			audioJitterBuffer->Reset();
			audioStream->Start(true);
			audioChannelC64U->Start();
		}
		memoryCache->SetFixtureMode(true);
		memoryCache->FillWithTestPattern();
		logicalStateCache->FillWithTestPattern();
		SetConnectionStatus(C64U_CONNECTION_STATUS_CONNECTED);
		modeManager.SetMode(C64U_MODE_SCREEN);
		SetConnectionStatus(C64U_CONNECTION_STATUS_STREAMING);
		SetMediaStatus("Fixture stream active");
		return;
	}

	// Real mode: start transport layers
	restClient->SetFixtureMode(false);
	port64Client->SetFixtureMode(false);
	restClient->Start();
	port64Client->Start();
	videoStream->Start(false);
	if (c64SettingsC64UAudioEnabled)
	{
		audioJitterBuffer->Reset();
		audioStream->Start(false);
		audioChannelC64U->Start();
	}
	modeManager.SetMode(C64U_MODE_SCREEN);

	// Schedule device info to validate reachability
	ScheduleDeviceInfoFetch();

	// If password is set, authenticate
	if (!endpointPassword.empty())
	{
		SetConnectionStatus(C64U_CONNECTION_STATUS_AUTHENTICATING);
		ScheduleAuthenticate();
	}

	// Detect our local IP and tell the device to start sending video to us
	std::string localIp = DetectLocalIp();
	if (!localIp.empty())
	{
		restClient->ScheduleStreamStart("video", localIp, endpointVideoPort);
		if (c64SettingsC64UAudioEnabled)
		{
			restClient->ScheduleStreamStart("audio", localIp, endpointVideoPort + 1);
		}
	}

	SetConnectionStatus(C64U_CONNECTION_STATUS_CONNECTED);
	SetConnectionStatus(C64U_CONNECTION_STATUS_STREAMING);
	connectionStartTimeMillis = C64U_GetCurrentTimeMillis();
	lastVideoStreamRetryMs = connectionStartTimeMillis;
	SetMediaStatus("Connecting to device...");
}

void CDebugInterfaceC64U::Disconnect()
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	if (GetConnectionStatus() == C64U_CONNECTION_STATUS_DISCONNECTED)
		return;

	audioChannelC64U->Stop();
	audioStream->Stop();
	debugStream->Stop();
	videoStream->Stop();
	port64Client->Stop();
	restClient->Stop();

	SetConnectionStatus(C64U_CONNECTION_STATUS_DISCONNECTED);
	modeManager.SetMode(C64U_MODE_DISCONNECTED);
	reconnectRetries = 0;
	connectionStartTimeMillis = 0;
	SetMediaStatus("Disconnected");
}

void CDebugInterfaceC64U::EnterTraceMode(int debugStreamMode)
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	if (modeManager.GetMode() == C64U_MODE_TRACE)
		return;

	if (modeManager.GetMode() == C64U_MODE_DISCONNECTED)
		return;

	modeManager.SetMode(C64U_MODE_SWITCHING);

	if (C64UTestFixture::IsEnabled())
	{
		// Fixture mode: skip real network, just set the mode and start fixture debug stream
		traceBuffer->Reset();
		traceMemoryDrainIndex = 0;
		decoder6510->Reset();
		decoder6510->SetTraceMode(debugStreamMode);
		debugStream->Start(true);
		modeManager.SetMode(C64U_MODE_TRACE);
		SetMediaStatus("Trace mode (fixture)");
		return;
	}

	// Real mode:
	// 1. Stop video stream
	videoStream->Stop();
	restClient->ScheduleStreamStop("video");

	// 2. Reset trace buffer, decoder, and memory drain index
	traceBuffer->Reset();
	traceMemoryDrainIndex = 0;
	decoder6510->Reset();
	decoder6510->SetTraceMode(debugStreamMode);

	// 3. Start debug stream
	debugStream->Start(false);
	std::string localIp = DetectLocalIp();
	if (!localIp.empty())
		restClient->ScheduleStreamStart("debug", localIp, endpointVideoPort + 2);

	// 4. Bootstrap VIC/SID/CIA register cache — the trace only sees registers
	//    when the CPU writes them; initial writes may be outside the trace window.
	memoryCache->SnapshotIORegisters();

	modeManager.SetMode(C64U_MODE_TRACE);
	SetMediaStatus("Trace mode active");
}

void CDebugInterfaceC64U::EnterScreenMode()
{
	std::lock_guard<std::mutex> lock(connectionMutex);

	if (modeManager.GetMode() == C64U_MODE_SCREEN)
		return;

	if (modeManager.GetMode() == C64U_MODE_DISCONNECTED)
		return;

	modeManager.SetMode(C64U_MODE_SWITCHING);

	if (C64UTestFixture::IsEnabled())
	{
		// Fixture mode: skip real network, set mode and restart fixture video
		debugStream->Stop();
		videoStream->Start(true);
		modeManager.SetMode(C64U_MODE_SCREEN);
		SetMediaStatus("Screen mode (fixture)");
		return;
	}

	// Real mode:
	// 1. Stop debug stream
	debugStream->Stop();
	restClient->ScheduleStreamStop("debug");

	// 2. Start video stream
	videoStream->Start(false);
	std::string localIp = DetectLocalIp();
	if (!localIp.empty())
		restClient->ScheduleStreamStart("video", localIp, endpointVideoPort);

	// Reset the first-frame grace period so the video timeout check
	// doesn't fire immediately (connectionStartTimeMillis was set during
	// the original Connect and may be >10s ago after a trace session).
	connectionStartTimeMillis = C64U_GetCurrentTimeMillis();
	lastVideoStreamRetryMs = connectionStartTimeMillis;

	modeManager.SetMode(C64U_MODE_SCREEN);
	SetMediaStatus("Screen mode active");
}

bool CDebugInterfaceC64U::IsInTraceMode() const
{
	return modeManager.GetMode() == C64U_MODE_TRACE;
}

C64UModeManager *CDebugInterfaceC64U::GetModeManager()
{
	return &modeManager;
}

EC64UMode CDebugInterfaceC64U::GetMode() const
{
	return modeManager.GetMode();
}

EC64UConnectionStatus CDebugInterfaceC64U::GetConnectionStatus() const
{
	return (EC64UConnectionStatus)connectionStatus.load();
}

const char *CDebugInterfaceC64U::GetConnectionStatusString() const
{
	switch (GetConnectionStatus())
	{
		case C64U_CONNECTION_STATUS_CONNECTING:
			return "Connecting";
		case C64U_CONNECTION_STATUS_AUTHENTICATING:
			return "Authenticating";
		case C64U_CONNECTION_STATUS_CONNECTED:
			return "Connected";
		case C64U_CONNECTION_STATUS_STREAMING:
			return "Streaming";
		case C64U_CONNECTION_STATUS_RECONNECTING:
			return "Reconnecting";
		case C64U_CONNECTION_STATUS_DISCONNECTED:
		default:
			return "Disconnected";
	}
}

const char *CDebugInterfaceC64U::GetModeString() const
{
	switch (modeManager.GetMode())
	{
		case C64U_MODE_SCREEN:
			return "Screen";
		case C64U_MODE_TRACE:
			return "Trace";
		case C64U_MODE_DISCONNECTED:
		default:
			return "Disconnected";
	}
}

const char *CDebugInterfaceC64U::GetMediaStatusString() const
{
	std::lock_guard<std::mutex> lock(mediaStatusMutex);
	mediaStatusSnapshot = mediaStatus;
	return mediaStatusSnapshot.c_str();
}

uint64_t CDebugInterfaceC64U::GetVideoFrameCounter() const
{
	return videoStream->GetFrameCounter();
}

uint64_t CDebugInterfaceC64U::GetMillisecondsSinceLastVideoFrame() const
{
	return videoStream->GetMillisecondsSinceLastFrame();
}

bool CDebugInterfaceC64U::IsRunningInTestFixtureMode() const
{
	return C64UTestFixture::IsEnabled();
}

C64UMemoryCache *CDebugInterfaceC64U::GetMemoryCache()
{
	return memoryCache;
}

C64UMediaStatusCache *CDebugInterfaceC64U::GetMediaStatusCache()
{
	return mediaStatusCache;
}

C64ULogicalStateCache *CDebugInterfaceC64U::GetLogicalStateCache()
{
	return logicalStateCache;
}

C64UTraceBuffer *CDebugInterfaceC64U::GetTraceBuffer()
{
	return traceBuffer;
}

C64U6502Decoder *CDebugInterfaceC64U::GetDecoder6510()
{
	return decoder6510;
}

void CDebugInterfaceC64U::ScheduleDeviceInfoFetch()
{
	restClient->ScheduleDeviceInfo();
	port64Client->ScheduleDeviceInfo();
}

void CDebugInterfaceC64U::ScheduleAuthenticate()
{
	restClient->ScheduleAuthenticate();
}

void CDebugInterfaceC64U::ScheduleReboot()
{
	restClient->ScheduleReboot();
	SetMediaStatus("Reboot requested");
}

void CDebugInterfaceC64U::ScheduleRunCrt(const std::string &path)
{
	restClient->ScheduleRunCrt(path);
	port64Client->ScheduleRunCrt(path);
	SetMediaStatus(std::string("CRT requested: ") + path);
}

void CDebugInterfaceC64U::ScheduleRemoveDisk(int driveId)
{
	restClient->ScheduleRemoveDisk(driveId);
	port64Client->ScheduleRemoveDisk(driveId);
	SetMediaStatus(std::string("Disk removed from drive ") + std::to_string(driveId));
}

void CDebugInterfaceC64U::ScheduleResetDisk(int driveId)
{
	restClient->ScheduleResetDisk(driveId);
	port64Client->ScheduleResetDisk(driveId);
	SetMediaStatus(std::string("Disk reset requested for drive ") + std::to_string(driveId));
}

void CDebugInterfaceC64U::ScheduleGetDriveInfo()
{
	restClient->ScheduleGetDriveInfo();
}

void CDebugInterfaceC64U::SetConnectionStatus(EC64UConnectionStatus status)
{
	connectionStatus.store((int)status);
}

std::string CDebugInterfaceC64U::DetectLocalIp()
{
	std::string localIp;
	if (c64SettingsC64ULocalIP && c64SettingsC64ULocalIP->GetLength() > 0)
	{
		char *tmp = c64SettingsC64ULocalIP->GetStdASCII();
		localIp = tmp;
		delete [] tmp;
	}
	if (localIp.empty())
	{
		localIp = C64ULocalIpDetect::Detect(endpointHost, endpointHttpPort);
	}
	return localIp;
}

void CDebugInterfaceC64U::RebuildVideoLUT(const uint8_t palette[16][3])
{
	if (videoStream)
	{
		videoStream->RebuildLUT(palette);
	}
}

void CDebugInterfaceC64U::SetMediaStatus(const std::string &status)
{
	std::lock_guard<std::mutex> lock(mediaStatusMutex);
	mediaStatus = status;
	mediaStatusSnapshot = status;
}

C64UAudioStream *CDebugInterfaceC64U::GetAudioStream()
{
	return audioStream;
}

C64UAudioJitterBuffer *CDebugInterfaceC64U::GetAudioJitterBuffer()
{
	return audioJitterBuffer;
}

CAudioChannelC64U *CDebugInterfaceC64U::GetAudioChannel()
{
	return audioChannelC64U;
}

C64UFtpClient *CDebugInterfaceC64U::GetFtpClient()
{
	return ftpClient;
}

C64URomBypass *CDebugInterfaceC64U::GetRomBypass()
{
	return romBypass;
}

bool CDebugInterfaceC64U::IsHelperAssisted() const
{
	return c64SettingsC64UHelperAssisted;
}
