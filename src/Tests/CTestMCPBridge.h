#pragma once

#include "CTest.h"
#include <thread>
#include <atomic>

class CTestMCPBridge : public CTest
{
public:
	virtual const char *GetName() override { return "MCPBridge"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override;
};

// Runs endpoint requests off the render thread so the real ImGui driver can
// progress.  This is not an ImGui test waiting on its own serialized queue.
class CTestMCPGuiControl : public CTest
{
public:
	~CTestMCPGuiControl() override;
	const char *GetName() override { return "MCPGuiControl"; }
	void Run(ITestCallback *callback) override;
	void Cancel() override;
private:
	void RunEndpointTest();
	std::thread worker;
	std::atomic<bool> cancelled{false};
};
