#include "CViewTimeline.h"
#include "VID_Main.h"
#include "CDebugInterface.h"
#include "CSnapshotsManager.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CMainMenuBar.h"

CViewTimeline::CViewTimeline(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	fontSize = 8.0f;
	
	isLockedVisible = false;
	isScrubbing = false;
	
	scrubIntervalMS = 25;
	nextPossibleScrubTime = 0;

	/*btnDone = new CGuiButton("DONE", posEndX - (guiButtonSizeX + guiButtonGapX), 
							 posEndY - (guiButtonSizeY + guiButtonGapY), posZ + 0.04, 
							 guiButtonSizeX, guiButtonSizeY, 
							 BUTTON_ALIGNED_DOWN, this);
	this->AddGuiElement(btnDone);	
	 */
}

CViewTimeline::~CViewTimeline()
{
}

void CViewTimeline::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewTimeline::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewTimeline::Render()
{
//	LOGD("CViewTimeline::Render: pos=%f %f %f %f", posX, posY, sizeX, sizeY);
	float timeLineR = 0.15;
	float timeLineG = 0.15;
	float timeLineB = 1.0;
	float timeLineA = 0.7;
	
	float textR = 1.0f;
	float textG = 1.0f;
	float textB = 1.0f;
	float textA = 1.0f;
	
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, timeLineR, timeLineG, timeLineB, timeLineA);
	
	int currentFrame = GetCurrentFrameNum();
	int textFrame = currentFrame;
	int minFrame, maxFrame;
	GetFramesLimits(&minFrame, &maxFrame);

	if (IsInside(guiMain->mousePosX, guiMain->mousePosY))
	{
		textFrame = CalcFrameNumFromMousePos(minFrame, maxFrame);
		textR = 1.0f;
		textG = 0.60f;
		textB = 0.60f;
		textA = 1.00f;
	}
	
//	LOGD("GetFramesLimits: min=%6d max=%6d", minFrame, maxFrame);

	char *buf = SYS_GetCharBuf();
//	sprintf(buf, "%6d %6d", minFrame, maxFrame);
//	viewC64->fontDisassembly->BlitText(buf, 0, 0, 0, 11, 1.0);

	// draw scrubbing box
	float bwidth = 8.0f;
	float bwidth2 = 4.0f;
	
	float bx = sizeX * ((float)(textFrame - minFrame) / (float)(maxFrame-minFrame)) - bwidth2 + posX;

	float scrubBoxR = 0.75;
	float scrubBoxG = 0.25;
	float scrubBoxB = 0.25;
	float scrubBoxA = 0.9;
	BlitFilledRectangle(bx, posY, posZ, bwidth, sizeY, scrubBoxR, scrubBoxG, scrubBoxB, scrubBoxA);

	if (textFrame != currentFrame)
	{
		bx = sizeX * ((float)(currentFrame - minFrame) / (float)(maxFrame-minFrame)) - bwidth2 + posX;
		float frameBoxR = 0.55;
		float frameBoxG = 0.45;
		float frameBoxB = 0.45;
		float frameBoxA = 0.8;
		BlitFilledRectangle(bx, posY, posZ, bwidth, sizeY, frameBoxR, frameBoxG, frameBoxB, frameBoxA);
	}
	
	sprintf(buf, "%d", textFrame);
	float px = sizeX / 2.0f - (strlen(buf) / 2 * fontSize) + posX;
	
	float offsetY = sizeY/2.0f - fontSize/2.0f;
	viewC64->fontDisassembly->BlitTextColor(buf, px, posY + offsetY, posZ, fontSize, textR, textG, textB, textA);
	
	SYS_ReleaseCharBuf(buf);
	
	CGuiView::Render();
}

int CViewTimeline::CalcFrameNumFromMousePos(int minFrame, int maxFrame)
{
	float px = guiMain->mousePosX - posX;
	
	float t = px / sizeX;
	float tf = ((float)(maxFrame-minFrame) * t) + minFrame;
	
	return (int)tf;
}

void CViewTimeline::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewTimeline::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewTimeline::ButtonPressed(CGuiButton *button)
{
	/*
	if (button == btnDone)
	{
		guiMain->SetView((CGuiView*)guiMain->viewMainEditor);
		GUI_SetPressConsumed(true);
		return true;
	}
	*/
	return false;
}

int CViewTimeline::GetCurrentFrameNum()
{
	int currentFrame = debugInterface->GetEmulationFrameNumber();
	return currentFrame;
}

void CViewTimeline::GetFramesLimits(int *minFrame, int *maxFrame)
{
	return debugInterface->snapshotsManager->GetFramesLimits(minFrame, maxFrame);
}

void CViewTimeline::ScrubToFrame(int frameNum)
{
	u64 t = SYS_GetCurrentTimeInMillis();
	if (t > nextPossibleScrubTime)
	{
//		guiMain->LockMutex();
		nextPossibleScrubTime = t + scrubIntervalMS;
		
		if (debugInterface->snapshotsManager->IsPerformingSnapshotRestore() == false)
		{
			debugInterface->snapshotsManager->RestoreSnapshotByFrame(frameNum, -1);
		}
//		guiMain->UnlockMutex();

	}
	
}

void CViewTimeline::ScrubToPos(float x)
{
	int minFrame, maxFrame;
	GetFramesLimits(&minFrame, &maxFrame);
	int frameNum = CalcFrameNumFromMousePos(minFrame, maxFrame);

	ScrubToFrame(frameNum);
}

//@returns is consumed
bool CViewTimeline::DoTap(float x, float y)
{
	LOGG("CViewTimeline::DoTap:  x=%f y=%f", x, y);
	
	if (!IsInside(x, y))
		return false;
	
	isScrubbing = true;
	
	ScrubToPos(x);
	
	return CGuiView::DoTap(x, y);
}

bool CViewTimeline::DoFinishTap(float x, float y)
{
	LOGG("CViewTimeline::DoFinishTap: %f %f", x, y);
	isScrubbing = false;
	return CGuiView::DoFinishTap(x, y);
}

bool CViewTimeline::HasContextMenuItems()
{
	return true;
}

void CViewTimeline::RenderContextMenuItems()
{
	if (ImGui::MenuItem("Load timeline"))
	{
		viewC64->mainMenuBar->OpenDialogLoadTimeline();
	}
	if (ImGui::MenuItem("Save timeline"))
	{
		viewC64->mainMenuBar->OpenDialogSaveTimeline(debugInterface);
	}
	if (ImGui::MenuItem("Clear timeline"))
	{
		debugInterface->snapshotsManager->ClearSnapshotsHistory();
	}
	if (ImGui::MenuItem("Free pool memory"))
	{
		debugInterface->snapshotsManager->DeleteAllPools();
	}
	if (ImGui::BeginMenu("Stats"))
	{
		int minFrame, maxFrame;
		debugInterface->snapshotsManager->GetFramesLimits(&minFrame, &maxFrame);
		ImGui::Text("Frames from %d to %d", minFrame, maxFrame);
		ImGui::Text("Chips snapshots stored: %d pool: %d", debugInterface->snapshotsManager->chipSnapshotsByCycle.size(),
					debugInterface->snapshotsManager->chipsSnapshotsToReuse.size());
		ImGui::Text("Disk snapshots stored: %d pool: %d", debugInterface->snapshotsManager->diskSnapshotsByCycle.size(),
					debugInterface->snapshotsManager->diskSnapshotsToReuse.size());
		ImGui::Text("Input events stored: %d pool: %d", debugInterface->snapshotsManager->inputEventsByCycle.size(),
					debugInterface->snapshotsManager->inputEventsToReuse.size());
		ImGui::EndMenu();
	}
}

//@returns is consumed
bool CViewTimeline::DoDoubleTap(float x, float y)
{
	LOGG("CViewTimeline::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewTimeline::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewTimeline::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewTimeline::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (isScrubbing)
	{
		ScrubToPos(x);
	}
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewTimeline::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	isScrubbing = false;
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewTimeline::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewTimeline::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewTimeline::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiView::DoScrollWheel(deltaX, deltaY);
}

bool CViewTimeline::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewTimeline::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewTimeline::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

//void CViewTimeline::FinishTouches()
//{
//	isScrubbing = false;
//	return CGuiView::FinishTouches();
//}

bool CViewTimeline::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewTimeline::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewTimeline::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewTimeline::ActivateView()
{
	LOGG("CViewTimeline::ActivateView()");
}

void CViewTimeline::DeactivateView()
{
	LOGG("CViewTimeline::DeactivateView()");
}

#define TIMELINE_ASYNC_STORE	1
#define TIMELINE_ASYNC_RESTORE	2

void CViewTimeline::LoadTimeline(CSlrString *path)
{
	LOGM("CViewTimeline::LoadTimeline");
	CTimelineIoThread *timelineThread = new CTimelineIoThread();
	timelineThread->asyncOperation = TIMELINE_ASYNC_RESTORE;
	timelineThread->timelinePath = new CSlrString(path);
	SYS_StartThread(timelineThread);
}

void CViewTimeline::SaveTimeline(CSlrString *path, CDebugInterface *debugInterface)
{
	LOGM("CViewTimeline::SaveTimeline");
	CTimelineIoThread *timelineThread = new CTimelineIoThread();
	timelineThread->debugInterface = debugInterface;
	timelineThread->asyncOperation = TIMELINE_ASYNC_STORE;
	timelineThread->timelinePath = new CSlrString(path);
	SYS_StartThread(timelineThread);
}

// saving & loading timeline
void CTimelineIoThread::ThreadRun(void *passData)
{
	if (asyncOperation == TIMELINE_ASYNC_STORE)
	{
		debugInterface->snapshotsManager->StoreTimelineToFile(timelinePath);
		delete timelinePath;
	}
	else if (asyncOperation == TIMELINE_ASYNC_RESTORE)
	{
		CSnapshotsManager::RestoreTimelineFromFile(timelinePath);
		delete timelinePath;
	}
}
