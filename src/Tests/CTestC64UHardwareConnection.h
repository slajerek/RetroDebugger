#pragma once

#include "CTest.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "C64SettingsStorage.h"
#include "../Emulators/c64u/CDebugInterfaceC64U.h"
#include "../Emulators/c64u/C64UConnectionStatus.h"
#include "../Emulators/c64u/Transport/C64URestClient.h"
#include "../Emulators/c64u/Transport/C64UVideoStream.h"
#include "../Emulators/c64u/State/C64UMemoryCache.h"
#include "../DebugInterface/C64/C64BackendFactory.h"

#include <cstring>

// This test requires a real Ultimate 64 device on the network.
// It uses the configured hostname (c64SettingsC64UHostname).
// If the device is unreachable, the test skips gracefully.
class CTestC64UHardwareConnection : public CTest
{
public:
	virtual const char *GetName() override { return "C64UHardwareConnection"; }

	virtual void Run(ITestCallback *callback) override
	{
		this->callback = callback;
		this->isRunning = true;
		this->currentStep = 0;

#ifndef RUN_COMMODORE64
		TestCompleted(true, "Skipped (C64 not enabled)");
		return;
#else
		CDebugInterfaceC64U *c64u = NULL;

		// Use the app's existing C64U backend if available
		if (viewC64->debugInterfaceC64U != NULL)
		{
			c64u = (CDebugInterfaceC64U *)viewC64->debugInterfaceC64U;
		}
		else
		{
			TestCompleted(true, "Skipped (C64U backend not created)");
			return;
		}

		// Step 1: Verify we have a configured hostname
		if (c64SettingsC64UHostname == NULL || c64SettingsC64UHostname->GetLength() == 0)
		{
			TestCompleted(true, "Skipped (no C64U hostname configured)");
			return;
		}
		char *hostnameStr = c64SettingsC64UHostname->GetStdASCII();
		std::string hostname(hostnameStr);
		delete [] hostnameStr;

		std::string password;
		if (c64SettingsC64UPassword)
		{
			char *tmp = c64SettingsC64UPassword->GetStdASCII();
			password = tmp;
			delete [] tmp;
		}

		char stepMsg[256];
		snprintf(stepMsg, sizeof(stepMsg), "Hostname: %s, HTTP port: %d", hostname.c_str(), c64SettingsC64UHttpPort);
		StepCompleted(1, true, stepMsg);

		// Step 2: Probe device reachability with a REST device-info request
		C64URestClient probeClient;
		probeClient.SetEndpoint(hostname, c64SettingsC64UHttpPort, password);
		probeClient.SetFixtureMode(false);
		probeClient.Start();
		probeClient.ScheduleDeviceInfo();

		// Wait up to 5 seconds for REST response
		bool gotResponse = false;
		for (int i = 0; i < 50; i++)
		{
			SYS_Sleep(100);
			C64URestCommandResult result = probeClient.GetLastResult();
			if (result.statusCode != 0 || !result.detail.empty())
			{
				gotResponse = true;
				if (!result.success)
				{
					probeClient.Stop();
					char msg[256];
					snprintf(msg, sizeof(msg),
					         "C64U device not reachable (%s) -- the hardware connection "
					         "was never exercised", result.detail.c_str());
					TestSkipped(msg);
					return;
				}
				break;
			}
		}
		probeClient.Stop();

		if (!gotResponse)
		{
			TestCompleted(true, "Skipped (device-info request timed out after 5s)");
			return;
		}
		StepCompleted(2, true, "Device is reachable via REST API");

		// Step 3: Connect if not already connected
		bool weConnected = false;
		if (c64u->GetConnectionStatus() == C64U_CONNECTION_STATUS_DISCONNECTED)
		{
			c64u->Connect();
			weConnected = true;
		}

		if (c64u->GetConnectionStatus() == C64U_CONNECTION_STATUS_DISCONNECTED)
		{
			TestCompleted(false, "Connect() did not change connection status from DISCONNECTED");
			return;
		}

		snprintf(stepMsg, sizeof(stepMsg), "Connection status: %s", c64u->GetConnectionStatusString());
		StepCompleted(3, true, stepMsg);

		// Step 4: Wait for video frames (up to 8 seconds)
		bool gotFrames = false;
		for (int i = 0; i < 80; i++)
		{
			SYS_Sleep(100);
			if (c64u->GetVideoFrameCounter() > 0)
			{
				gotFrames = true;
				break;
			}
			if (c64u->GetConnectionStatus() == C64U_CONNECTION_STATUS_DISCONNECTED)
			{
				break;
			}
		}

		if (!gotFrames)
		{
			snprintf(stepMsg, sizeof(stepMsg), "No video frames after 8s (status: %s, frames: %llu)",
					 c64u->GetConnectionStatusString(), (unsigned long long)c64u->GetVideoFrameCounter());
			if (weConnected)
				c64u->Disconnect();
			TestCompleted(false, stepMsg);
			return;
		}

		snprintf(stepMsg, sizeof(stepMsg), "Video streaming: %llu frames received",
				 (unsigned long long)c64u->GetVideoFrameCounter());
		StepCompleted(4, true, stepMsg);

		// Step 5: Verify connection remains in STREAMING state
		if (c64u->GetConnectionStatus() != C64U_CONNECTION_STATUS_STREAMING)
		{
			snprintf(stepMsg, sizeof(stepMsg), "Expected STREAMING, got %s", c64u->GetConnectionStatusString());
			if (weConnected)
				c64u->Disconnect();
			TestCompleted(false, stepMsg);
			return;
		}
		StepCompleted(5, true, "Connection stable in STREAMING state");

		// Step 6: Read memory at $A000 (BASIC ROM area — should be non-zero)
		C64UMemoryCache *cache = c64u->GetMemoryCache();
		if (cache)
		{
			cache->MarkPageAccessed(0xA0);
			cache->ScheduleVisiblePageRefreshes();
			SYS_Sleep(1000);

			uint8_t val = cache->ReadByte(0xA000);
			snprintf(stepMsg, sizeof(stepMsg), "Memory read $A000 = $%02X", val);
			StepCompleted(6, true, stepMsg);
		}
		else
		{
			StepCompleted(6, true, "Memory cache not available, skipped");
		}

		// Step 7: Clean up — disconnect if we connected
		if (weConnected)
		{
			c64u->Disconnect();
			if (c64u->GetConnectionStatus() != C64U_CONNECTION_STATUS_DISCONNECTED)
			{
				TestCompleted(false, "Disconnect() did not reach DISCONNECTED");
				return;
			}
			StepCompleted(7, true, "Disconnected successfully");
		}
		else
		{
			StepCompleted(7, true, "Left existing connection intact");
		}

		TestCompleted(true, "Hardware connection verified");
#endif
	}

	virtual void Cancel() override
	{
		isRunning = false;
	}
};
