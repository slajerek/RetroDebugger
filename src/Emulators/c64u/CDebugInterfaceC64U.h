#ifndef _CDEBUGINTERFACEC64U_H_
#define _CDEBUGINTERFACEC64U_H_

#include "../../DebugInterface/C64/CDebugInterfaceC64.h"
#include "C64UModeManager.h"
#include "C64UConnectionStatus.h"
#include "Adapters/CDataAdapterC64UVicRam.h"

#include <atomic>
#include <mutex>
#include <string>

class C64URestClient;
class C64UPort64Client;
class C64UVideoStream;
class C64UDebugStream;
class C64UAudioStream;
class C64UTraceBuffer;
class C64U6502Decoder;
class C64UMemoryCache;
class C64UMediaStatusCache;
class C64ULogicalStateCache;
class C64UAudioJitterBuffer;
class CAudioChannelC64U;
class C64UFtpClient;
class C64URomBypass;

class CDebugInterfaceC64U : public CDebugInterfaceC64
{
public:
	CDebugInterfaceC64U(CViewC64 *viewC64);
	virtual ~CDebugInterfaceC64U();

	virtual int GetEmulatorType() override;
	virtual CSlrString *GetEmulatorVersionString() override;
	virtual const char *GetPlatformNameString() override;
	virtual const char *GetPlatformNameEndpointString() override;
	virtual CC64BackendCapabilities GetC64BackendCapabilities() override;
	virtual int GetScreenSizeX() override;
	virtual int GetScreenSizeY() override;
	virtual void GetCBMColor(uint8 colorNum, uint8 *r, uint8 *g, uint8 *b) override;
	virtual void RefreshScreenNoCallback() override;
	virtual void RunEmulationThread() override;
	virtual void GetCpuRegs(u16 *PC, u8 *A, u8 *X, u8 *Y, u8 *P, u8 *S) override;
	virtual u64 GetMainCpuCycleCounter() override;
	virtual void Shutdown() override;
	virtual void ResetSoft() override;
	virtual void ResetHard() override;
	virtual bool LoadExecutable(char *fullFilePath) override;
	virtual bool MountDisk(char *fullFilePath, int diskNo, bool readOnly) override;
	virtual bool GetSettingIsWarpSpeed() override;
	virtual void SetSettingIsWarpSpeed(bool isWarpSpeed) override;
	virtual CDebuggerApi *GetDebuggerApi() override;
	// RD-045: observed only -- no stepping or breakpoints; the decoded bus
	// trace is replayed to execution observers (CDebugSourceTrace.h)
	virtual CDebugBackendCapabilities GetExecutionCapabilities() override;

	void Connect();
	void Disconnect();

	void EnterTraceMode(int debugStreamMode);
	void EnterScreenMode();
	bool IsInTraceMode() const;

	C64UModeManager *GetModeManager();
	EC64UMode GetMode() const;
	EC64UConnectionStatus GetConnectionStatus() const;
	const char *GetConnectionStatusString() const;
	const char *GetModeString() const;
	const char *GetMediaStatusString() const;
	uint64_t GetVideoFrameCounter() const;
	uint64_t GetMillisecondsSinceLastVideoFrame() const;
	bool IsRunningInTestFixtureMode() const;

	C64UMemoryCache *GetMemoryCache();
	C64UMediaStatusCache *GetMediaStatusCache();
	C64ULogicalStateCache *GetLogicalStateCache();
	C64UTraceBuffer *GetTraceBuffer();
	C64U6502Decoder *GetDecoder6510();
	C64UAudioStream *GetAudioStream();
	C64UAudioJitterBuffer *GetAudioJitterBuffer();
	CAudioChannelC64U *GetAudioChannel();
	C64UFtpClient *GetFtpClient();
	C64URomBypass *GetRomBypass();
	bool IsHelperAssisted() const;

	void ScheduleDeviceInfoFetch();
	void ScheduleAuthenticate();
	void ScheduleReboot();
	void ScheduleRunCrt(const std::string &path);
	void ScheduleRemoveDisk(int driveId);
	void ScheduleResetDisk(int driveId);
	void ScheduleGetDriveInfo();

	void RebuildVideoLUT(const uint8_t palette[16][3]);

protected:
	void SetConnectionStatus(EC64UConnectionStatus status);
	void SetMediaStatus(const std::string &status);
	std::string DetectLocalIp();

	C64UModeManager modeManager;
	uint32_t renderFrameCounter = 0;
	std::atomic<int> connectionStatus;
	mutable std::mutex connectionMutex;
	int reconnectRetries;
	bool needsReconnect;
	uint64_t connectionStartTimeMillis = 0;
	uint64_t lastVideoStreamRetryMs = 0;  // last auto-retry of video stream start; 0 = not yet sent
	static const int MAX_RECONNECT_RETRIES = 3;
	mutable std::mutex mediaStatusMutex;
	std::string mediaStatus;
	mutable std::string mediaStatusSnapshot;
	std::string endpointHost;
	std::string endpointPassword;
	int endpointHttpPort;
	int endpointTcpPort;
	int endpointVideoPort;
	uint64_t traceMemoryDrainIndex;
public:
	// trace mode, once per frame: the bus entries received since the last
	// drain into the memory cache, the memory map and, while observed, the
	// execution observers as replayed instruction boundaries (tests call it)
	void DrainTraceBusData();
protected:

	C64URestClient *restClient;
	C64UPort64Client *port64Client;
	C64UVideoStream *videoStream;
	C64UDebugStream *debugStream;
	C64UAudioStream *audioStream;
	C64UTraceBuffer *traceBuffer;
	C64U6502Decoder *decoder6510;
	C64UMemoryCache *memoryCache;
	C64UMediaStatusCache *mediaStatusCache;
	C64ULogicalStateCache *logicalStateCache;
	C64UAudioJitterBuffer *audioJitterBuffer;
	CAudioChannelC64U *audioChannelC64U;
	C64UFtpClient *ftpClient;
	C64URomBypass *romBypass;

public:
	CDataAdapterC64UVicRam *dataAdapterC64UVicRam;
};

#endif
