#include "CViewJukeboxPlaylist.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include "CMainMenuBar.h"
#include "CDebugInterfaceC64.h"
#include "CJukeboxPlaylist.h"
#include "CSlrFileFromOS.h"
#include "CViewSnapshots.h"
#include "C64SettingsStorage.h"
#include "CViewC64Screen.h"

// JukeBox playlist is a tool for having fun. It was created for the first ever C64 emulator in VR created by slajerek
// and ought to be a demo player for VR. never finished, but definitely that was first C64 VR attempt i.e. world first :)
// At least now we have a tool to create playlist and watch demos in a loop...

CViewJukeboxPlaylist::CViewJukeboxPlaylist(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView("JukeBox playlist", posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->mutex = new CSlrMutex("CViewJukeboxPlaylist");
	this->font = viewC64->fontDisassembly;
	fontSize = 5.0f;

	this->playlist = NULL;
	this->currentEntry = NULL;
	this->currentAction = NULL;

	this->frameCounter = 0;
	this->emulationTime = 0;

	this->mode = JUKEBOX_PLAYLIST_MODE_LOOP;
	this->state = JUKEBOX_PLAYLIST_STATE_PAUSED;
	
	this->fadeState = JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE;
	this->textInfoFadeState = JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
	
}

void CViewJukeboxPlaylist::SetFont(CSlrFont *font, float fontSize)
{
	this->font = font;
	this->fontSize = fontSize;
	
	CGuiView::SetPosition(posX, posY, posZ, fontSize*51, fontSize*2);
}

void CViewJukeboxPlaylist::SetPosition(float posX, float posY)
{
	CGuiView::SetPosition(posX, posY, posZ, fontSize*51, fontSize*2);
}

void CViewJukeboxPlaylist::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewJukeboxPlaylist::DoLogic()
{
}

void CViewJukeboxPlaylist::Render()
{
	if (currentEntry != NULL && currentEntry->name != NULL)
	{
		if (fadeState != JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE)
		{
			BlitFilledRectangle(viewC64->viewC64Screen->posX,
								viewC64->viewC64Screen->posY, -1,
								viewC64->viewC64Screen->sizeX,
								viewC64->viewC64Screen->sizeY,
								currentEntry->fadeColorR, currentEntry->fadeColorG, currentEntry->fadeColorB,
								fadeValue);
		}

		if (currentEntry->name != NULL)
		{
			if (textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_IN
				|| textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_VISIBLE
				|| textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT)
			{
				guiMain->fntEngineDefault ->BlitTextColor(currentEntry->name, 60, SCREEN_HEIGHT-70, -1, 2.90, 1, 1, 1, textInfoFadeValue);
			}
		}
	}
	else if (fadeState != JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE)
	{
		BlitFilledRectangle(viewC64->viewC64Screen->posX,
							viewC64->viewC64Screen->posY, -1,
							viewC64->viewC64Screen->sizeX,
							viewC64->viewC64Screen->sizeY,
							0, 0, 0,
							fadeValue);
	}
	
//	char buf[256];
//	sprintf(buf, "%8.2f e=%2d %8.2f a=%2d %8.2f", this->emulationTime, this->entryIndex, this->entryTime, this->actionIndex, this->actionTime);
//	viewC64->fontDisassembly->BlitText(buf, 0, 0, -1, 10);
//
//	sprintf(buf, "%d %8.2f %8.2f", this->fadeState, this->fadeValue, this->fadeStep);
//	viewC64->fontDisassembly->BlitText(buf, 0, 10, -1, 10);
//
//	sprintf(buf, "%d %8.2f %8.2f", this->textInfoFadeState, this->textInfoFadeValue, this->textInfoFadeStep);
//	viewC64->fontDisassembly->BlitText(buf, 0, 20, -1, 10);
}

bool CViewJukeboxPlaylist::DoTap(float x, float y)
{
	return false;
}

bool CViewJukeboxPlaylist::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewJukeboxPlaylist::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewJukeboxPlaylist::WillReceiveFocus()
{
	return false;
}

void CViewJukeboxPlaylist::DeletePlaylist()
{
	mutex->Lock();
	if (this->playlist != NULL)
	{
		this->state = JUKEBOX_PLAYLIST_STATE_PAUSED;
		SYS_Sleep(50);
		delete this->playlist;
		this->playlist = NULL;
	}
	mutex->Unlock();
}

void CViewJukeboxPlaylist::InitFromFile(char *jsonFilePath)
{
	LOGD("CViewJukeboxPlaylist::InitFromFile: %s", jsonFilePath);
	
	viewC64->debugInterfaceC64->LockMutex();
	guiMain->LockMutex(); //"CViewJukeboxPlaylist::InitFromFile");

	this->DeletePlaylist();
	
	CSlrFileFromOS *file = new CSlrFileFromOS(jsonFilePath);
	CByteBuffer *byteBuffer = new CByteBuffer(file, false);
	byteBuffer->ForwardToEnd();
	byteBuffer->PutU8(0x00);
	byteBuffer->Rewind();
	
	char *json = (char*)byteBuffer->data;
	
	this->playlist = new CJukeboxPlaylist(json);

	delete byteBuffer;

	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::InitFromFile");
	viewC64->debugInterfaceC64->UnlockMutex();

	LOGD("CViewJukeboxPlaylist::InitFromFile done");
}

void CViewJukeboxPlaylist::StartPlaylist()
{
	LOGD("CViewJukeboxPlaylist::StartPlaylist");
	
	mutex->Lock();
	
	viewC64->debugInterfaceC64->SetPatchKernalFastBoot(this->playlist->fastBootPatch);
	
	this->frameCounter = 0;
	this->emulationTime = 0;
	InitEntry(0);
	
	this->state = JUKEBOX_PLAYLIST_STATE_RUNNING;
	
	mutex->Unlock();
}

void CViewJukeboxPlaylist::InitEntry(int newIndex)
{
	LOGD("CViewJukeboxPlaylist::InitEntry: newIndex=%d", newIndex);
	
	guiMain->LockMutex(); //"CViewJukeboxPlaylist::InitEntry");
	viewC64->debugInterfaceC64->LockMutex();
	
	entryIndex = newIndex;
	
	uint8 machineType = viewC64->debugInterfaceC64->GetC64MachineType();
	
	if (machineType == MACHINE_TYPE_PAL)
	{
		emulationFPS = 50.0f;
	}
	else
	{
		emulationFPS = 60.0f;
	}
	
	currentEntry = this->playlist->entries[entryIndex];

	if (currentEntry->actions.empty() == false)
	{
		InitAction(0);
	}
	else
	{
		currentAction = NULL;
		actionIndex = -1;
		actionTime = 0;
	}
	
	entryTime = this->emulationTime + currentEntry->waitTime;
	
	if (currentEntry->fadeInTime > 0.0f)
	{
		fadeValue = 1.0f;
		fadeStep = 1.0f / (currentEntry->fadeInTime * emulationFPS);
		fadeState = JUKEBOX_PLAYLIST_FADE_STATE_FADE_IN;
		
		UpdateAudioVolume();
	}
	else
	{
		fadeState = JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE;
		fadeValue = 0.0f;
		
		UpdateAudioVolume();
		
		StartTextFadeIn();
	}

	RunCurrentEntry();
	
	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::InitEntry");
	viewC64->debugInterfaceC64->UnlockMutex();
}

void CViewJukeboxPlaylist::RunCurrentEntry()
{
	LOGD("CViewJukeboxPlaylist::RunCurrentEntry");
	currentEntry->DebugPrint();

	mutex->Lock();
	SYS_StartThread(this, (void*)currentEntry);
	mutex->Unlock();
}

void CViewJukeboxPlaylist::ThreadRun(void *data)
{
	LOGD("CViewJukeboxPlaylist::ThreadRun");
	
	mutex->Lock();
	
	CJukeboxPlaylistEntry *entry = (CJukeboxPlaylistEntry *)data;
	entry->DebugPrint();
	
	if (entry->resetMode == MACHINE_RESET_HARD)
	{
		viewC64->debugInterfaceC64->HardReset();
		
		if (entry->delayAfterResetTime < 0)
		{
			SYS_Sleep(this->playlist->delayAfterResetMs);
		}
		else
		{
			SYS_Sleep(entry->delayAfterResetTime);
		}
	}
	else if (entry->resetMode == MACHINE_RESET_SOFT)
	{
		viewC64->debugInterfaceC64->Reset();
		
		if (entry->delayAfterResetTime < 0)
		{
			SYS_Sleep(this->playlist->delayAfterResetMs);
		}
		else
		{
			SYS_Sleep(entry->delayAfterResetTime);
		}
	}
	
	guiMain->LockMutex(); //"CViewJukeboxPlaylist::ThreadRun");

	if (entry->filePath != NULL)
	{
		entry->filePath->DebugPrint("  action filePath=");
		
		CSlrString *ext = entry->filePath->GetFileExtensionComponentFromPath();
		if (ext->CompareWith("prg") || ext->CompareWith("PRG"))
		{
			viewC64->viewC64MainMenu->LoadPRG(entry->filePath, currentEntry->autoRun, false, this->playlist->showLoadAddressInfo, false);
		}
		else if (ext->CompareWith("d64") || ext->CompareWith("D64")
				 || ext->CompareWith("g64") || ext->CompareWith("G64"))
		{
			viewC64->viewC64MainMenu->InsertD64(entry->filePath, false,
												currentEntry->autoRun, currentEntry->runFileNum-1,
												this->playlist->showLoadAddressInfo);
		}
		else if (ext->CompareWith("crt") || ext->CompareWith("CRT"))
		{
			viewC64->viewC64MainMenu->InsertCartridge(entry->filePath, false);
		}
		else if (ext->CompareWith("reu") || ext->CompareWith("REU"))
		{
			bool val = true;
			C64DebuggerSetSetting("ReuEnabled", &val);
			viewC64->viewC64MainMenu->AttachReu(entry->filePath, false, false);
		}
		else if (ext->CompareWith("snap") || ext->CompareWith("SNAP")
				 || ext->CompareWith("vsf") || ext->CompareWith("VSF"))
		{
			viewC64->viewC64Snapshots->LoadSnapshot(entry->filePath, false, viewC64->debugInterfaceC64);
		}
		
		delete ext;
	}
	
	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::ThreadRun");
	
	mutex->Unlock();
}

void CViewJukeboxPlaylist::InitAction(int newIndex)
{
	LOGD("CViewJukeboxPlaylist::InitAction: newIndex=%d (entry #%d)", newIndex, entryIndex);
	
	guiMain->LockMutex(); //"CViewJukeboxPlaylist::InitAction");
	viewC64->debugInterfaceC64->LockMutex();

	actionIndex = newIndex;

	currentAction = currentEntry->actions[actionIndex];
	
	actionTime = emulationTime + currentAction->doAfterDelay;
	
	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::InitAction");
	viewC64->debugInterfaceC64->UnlockMutex();
}

void CViewJukeboxPlaylist::RunCurrentAction()
{
	LOGD("CViewJukeboxPlaylist::RunCurrentAction");
	currentAction->DebugPrint();

	switch (currentAction->actionType)
	{
		case JUKEBOX_ACTION_KEY_DOWN:
			viewC64->debugInterfaceC64->KeyboardDown(currentAction->code);
			break;
		case JUKEBOX_ACTION_KEY_UP:
			viewC64->debugInterfaceC64->KeyboardUp(currentAction->code);
			break;
		case JUKEBOX_ACTION_JOYSTICK1_DOWN:
			viewC64->debugInterfaceC64->JoystickDown(0, currentAction->code);
			break;
		case JUKEBOX_ACTION_JOYSTICK1_UP:
			viewC64->debugInterfaceC64->JoystickUp(0, currentAction->code);
			break;
		case JUKEBOX_ACTION_JOYSTICK2_DOWN:
			viewC64->debugInterfaceC64->JoystickDown(1, currentAction->code);
			break;
		case JUKEBOX_ACTION_JOYSTICK2_UP:
			viewC64->debugInterfaceC64->JoystickUp(1, currentAction->code);
			break;
		case JUKEBOX_ACTION_SET_WARP:
			viewC64->debugInterfaceC64->SetSettingIsWarpSpeed(currentAction->code == 1 ? true : false);
			break;
		case JUKEBOX_ACTION_DUMP_C64_MEMORY:
			viewC64->debugInterfaceC64->DumpC64Memory(currentAction->text);
			break;
		case JUKEBOX_ACTION_DUMP_DISK_MEMORY:
			viewC64->debugInterfaceC64->DumpDisk1541Memory(currentAction->text);
			break;
		case JUKEBOX_ACTION_DETACH_CARTRIDGE:
			viewC64->mainMenuBar->DetachCartridge(false);
			break;
		case JUKEBOX_ACTION_SAVE_SCREENSHOT:
			LOGD("JUKEBOX_ACTION_SAVE_SCREENSHOT");
			currentAction->text->DebugPrint("SAVE_SCREENSHOT currentAction->text=");
			viewC64->viewVicEditor->ExportPNG(currentAction->text);
			break;
		case JUKEBOX_ACTION_EXPORT_SCREEN:
			currentAction->text->DebugPrint("EXPORT_SCREEN currentAction->text=");
			LOGTODO("TODO: complete refactor of viewC64->viewVicEditor->ExportScreen(currentAction->text)");
			viewC64->viewVicEditor->ExportScreen(currentAction->text);
			break;
		case JUKEBOX_ACTION_SHUTDOWN:
			SYS_Sleep(50);	// sanity sleep...
			SYS_Shutdown();
			break;
			
		default:
			LOGError("CViewJukeboxPlaylist::RunCurrentAction: unknown action %d", currentAction->actionType);
	}
}

void CViewJukeboxPlaylist::AdvanceEntry()
{
	LOGD("CViewJukeboxPlaylist::AdvanceEntry: time=%8.2f", emulationTime);
	
	guiMain->LockMutex(); //"CViewJukeboxPlaylist::AdvanceEntry");
	
	// advance, run will be performed by init
	if (entryIndex < playlist->entries.size()-1)
	{
		InitEntry(entryIndex + 1);
	}
	else
	{
		if (mode == JUKEBOX_PLAYLIST_MODE_RUN_ONCE)
		{
			currentEntry = NULL;
			entryIndex = -1;
			entryTime = 0;
		}
		else if (mode == JUKEBOX_PLAYLIST_MODE_LOOP)
		{
			StartPlaylist();
		}
	}

	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::AdvanceEntry");
}

void CViewJukeboxPlaylist::AdvanceAction()
{
	LOGD("CViewJukeboxPlaylist::AdvanceAction: time=%8.2f", emulationTime);

	guiMain->LockMutex(); //"CViewJukeboxPlaylist::AdvanceAction");

	// first run action, and then advance
	this->RunCurrentAction();
	
	if (actionIndex < currentEntry->actions.size()-1)
	{
		InitAction(actionIndex + 1);
	}
	else
	{
		currentAction = NULL;
		actionIndex = -1;
		actionTime = 0;
	}
	guiMain->UnlockMutex(); //"CViewJukeboxPlaylist::AdvanceAction");
}

void CViewJukeboxPlaylist::StartTextFadeIn()
{
	if (this->playlist->showTextInfo)
	{
		textInfoFadeState = JUKEBOX_PLAYLIST_FADE_STATE_FADE_IN;
		textInfoFadeValue = 0.0f;
		textInfoFadeStep = 1.0f / (playlist->showTextFadeTime * emulationFPS);
	}
}

void CViewJukeboxPlaylist::UpdateTextFade()
{
	if (textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_IN)
	{
		float newFade = textInfoFadeValue + textInfoFadeStep;
		if (newFade > 1.0f)
		{
			textInfoFadeState = JUKEBOX_PLAYLIST_FADE_STATE_VISIBLE;
			textInfoFadeValue = 1.0f;
			textInfoFadeStep = 1.0f / (playlist->showTextVisibleTime * emulationFPS);
			textInfoFadeVisibleCounter = 0.0f;
		}
		else
		{
			textInfoFadeValue = newFade;
		}
	}
	else if (textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_VISIBLE)
	{
		textInfoFadeVisibleCounter = textInfoFadeVisibleCounter + textInfoFadeStep;
		if (textInfoFadeVisibleCounter > 1.0f)
		{
			textInfoFadeState = JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT;
			textInfoFadeValue = 1.0f;
			textInfoFadeStep = 1.0f / (playlist->showTextFadeTime * emulationFPS);
		}
	}
	if (textInfoFadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT)
	{
		float newFade = textInfoFadeValue - textInfoFadeStep;
		if (newFade < 0.0f)
		{
			textInfoFadeState = JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE;
			textInfoFadeValue = 0.0f;
		}
		else
		{
			textInfoFadeValue = newFade;
		}
	}
}


// jukebox logic
void CViewJukeboxPlaylist::EmulationStartFrame()
{
//	LOGD("CViewJukeboxPlaylist::EmulationStartFrame: #%d", viewC64->emulationFrameCounter);

	if (state != JUKEBOX_PLAYLIST_STATE_RUNNING)
		return;
	
	this->frameCounter += 1.0f;
	this->emulationTime = frameCounter / emulationFPS;
	
	//LOGD("                                      time=%-8.2f", this->emulationTime);
	//LOGD("entryTime=%8.2f actionTime=%8.2f", entryTime, actionTime);
	
	if (currentAction && emulationTime >= actionTime)
	{
		AdvanceAction();
	}

	if (fadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_IN)
	{
		float newFade = fadeValue - fadeStep;
		if (newFade < 0.0f)
		{
			fadeState = JUKEBOX_PLAYLIST_FADE_STATE_NO_FADE;
			fadeValue = 0.0f;
			
			UpdateAudioVolume();

			StartTextFadeIn();
		}
		else
		{
			fadeValue = newFade;

			UpdateAudioVolume();
		}
	}
	
	if (fadeState == JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT)
	{
		float newFade = fadeValue + fadeStep;
		if (newFade > 1.0f)
		{
			fadeValue = 1.0f;
			
			UpdateAudioVolume();

			AdvanceEntry();
		}
		else
		{
			fadeValue = newFade;

			UpdateAudioVolume();
		}
	}
	else
	{
		if (currentEntry && emulationTime >= entryTime)
		{
			if (currentEntry->fadeOutTime > 0.0f)
			{
				fadeValue = 0.0f;
				fadeStep = 1.0f / (currentEntry->fadeOutTime * emulationFPS);
				fadeState = JUKEBOX_PLAYLIST_FADE_STATE_FADE_OUT;

				UpdateAudioVolume();
			}
			else
			{
				AdvanceEntry();
			}
		}
	}
	
	UpdateTextFade();
}

void CViewJukeboxPlaylist::UpdateAudioVolume()
{
	if (playlist->fadeSoundVolume)
	{
		viewC64->debugInterfaceC64->SetAudioVolume(1.0f - fadeValue);
	}
}

