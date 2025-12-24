#include "CViewEmulatorScreen.h"
#include "C64SettingsStorage.h"
#include "VID_Main.h"
#include "CDebugInterface.h"
#include "CImageData.h"
#include "CSlrImage.h"
#include "CViewC64.h"
#include "CGamePad.h"
#include "CMainMenuBar.h"
#include "C64Tools.h"

// TODO: generalize CRenderBackendOpenGL4
#include "CRenderBackendOpenGL4.h"
#include "CRenderShaderCRTMonitorOpenGL4.h"
#include "CRenderShaderOpenGL4ShaderToy.h"

CViewEmulatorScreen::CViewEmulatorScreen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
										 CDebugInterface *debugInterface)
: CGuiViewMovingPaneImage(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;
	
	CRenderBackendOpenGL4 *renderBackend = CRenderBackendOpenGL4::GetRenderBackendOpenGL4();
	shaderCRT = new CRenderShaderCRTMonitorOpenGL4(renderBackend, "CRT Monitor", debugInterface->GetScreenSizeX(), debugInterface->GetScreenSizeY());
//	shaderCRT = new CRenderShaderOpenGL4ShaderToy(renderBackend, "Shader Toy", (float)debugInterface->GetScreenSizeX(), (float)debugInterface->GetScreenSizeY());

	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%s##EnableShaderCRT", name);
	viewC64->config->GetBool(buf, &settingEnableShaderCRT, false);
	SYS_ReleaseCharBuf(buf);

	if (settingEnableShaderCRT)
	{
		SetShader(shaderCRT);
	}
	
	SetMovingPaneStyle(MovingPaneStyle_None);
	
	InitImage();
}

void CViewEmulatorScreen::RefreshEmulatorScreenImageData()
{
	this->imageData = debugInterface->GetScreenImageData();
	this->paneWidth = debugInterface->GetScreenSizeX() * debugInterface->screenSupersampleFactor;
	this->paneHeight = debugInterface->GetScreenSizeY() * debugInterface->screenSupersampleFactor;
	
	// this is not correct sometimes
//	shader->SetResolution((float)paneWidth/4.0f, (float)paneHeight/4.0f);
	
	rasterWidth = imageData->width;
	rasterHeight = imageData->height;
	
	float w = (float)debugInterface->GetScreenSizeX();
	float h = (float)debugInterface->GetScreenSizeY();
	SetKeepAspectRatio(true, w/h);
	
	UpdatePositionAndZoom();
	
	RefreshRenderTextureParameters();
}

bool CViewEmulatorScreen::UpdateImageData()
{
	// we will update the image manually via CViewC64 before any render as other views may be using screen image
	// this is to avoid case when emulator screen is not visible, i.e. not refreshed
	return false;
}

void CViewEmulatorScreen::SetSupersampleFactor(int supersampleFactor)
{
	guiMain->LockMutex();
	debugInterface->LockRenderScreenMutex();

	debugInterface->SetSupersampleFactor(supersampleFactor);
	RefreshImageParameters();
	
	debugInterface->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
}

// called when supersample factor or other parameters are changed
void CViewEmulatorScreen::RefreshImageParameters()
{
	guiMain->LockMutex();
	debugInterface->LockRenderScreenMutex();
	
	RefreshEmulatorScreenImageData();
	
	image->RefreshImageParameters(imageData,  RESOURCE_PRIORITY_STATIC, false);
	image->linearScaling = !c64SettingsRenderScreenNearest;
	VID_PostImageBinding(image, NULL);
	
	float w = (float)debugInterface->GetScreenSizeX();
	float h = (float)debugInterface->GetScreenSizeY();
	SetKeepAspectRatio(true, w/h);

	debugInterface->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
}

void CViewEmulatorScreen::RefreshImage()
{
	image->ReBindImage();
}

bool CViewEmulatorScreen::HasContextMenuItems()
{
	return true;
}

void CViewEmulatorScreen::RenderContextMenuItems()
{
	CGuiViewMovingPaneImage::RenderContextMenuItems();

	if (VID_IsViewportsEnable())
	{
		if (VID_IsWindowAlwaysOnTop(this))
		{
			if (ImGui::MenuItem("Remove always on top"))
			{
				VID_SetWindowAlwaysOnTop(this, false);
			}
		}
		else
		{
			if (ImGui::MenuItem("Set always on top"))
			{
				VID_SetWindowAlwaysOnTop(this, true);
			}
		}
	}
	
	if (guiMain->IsViewFullScreen())
	{
		if (ImGui::MenuItem("Leave fullscreen"))
		{
			viewC64->ToggleFullScreen(this);
		}
	}
	else
	{
		float sx = debugInterface->GetScreenSizeX();
		float sy = debugInterface->GetScreenSizeY();
		
		if (ImGui::MenuItem("Go fullscreen##c64Screen"))
		{
			viewC64->ToggleFullScreen(this);
		}
		
		if (ImGui::BeginMenu("Scale##c64Screen"))
		{
			std::vector<float> sizes;
			std::vector<const char *> names;
			sizes.push_back(0.25f); names.push_back("25%##c64Screen");
			sizes.push_back(0.50f); names.push_back("50%##c64Screen");
			sizes.push_back(1.00f); names.push_back("100%##c64Screen");
			sizes.push_back(2.00f); names.push_back("200%##c64Screen");
			sizes.push_back(3.00f); names.push_back("300%##c64Screen");
			sizes.push_back(4.00f); names.push_back("400%##c64Screen");

			std::vector<float>::iterator itSize = sizes.begin();
			for (std::vector<const char *>::iterator itName = names.begin(); itName != names.end(); itName++)
			{
				const char *name = *itName;
				if (ImGui::MenuItem(name))
				{
					float f = *itSize;
					this->SetNewImGuiWindowSize( (sx * f) + 1, (sy * f) + 1);
				}
				
				itSize++;
			}
			
			ImGui::EndMenu();
		}
	}
	
	if (ImGui::MenuItem("Enable CRT emulation", NULL, (shader != NULL)))
	{
		if (shader == NULL)
		{
			SetShader(shaderCRT);
			settingEnableShaderCRT = true;
		}
		else
		{
			SetShader(NULL);
			settingEnableShaderCRT = false;
		}
		
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "%s::EnableShaderCRT", name);
		viewC64->config->SetBool(buf, &settingEnableShaderCRT);
		SYS_ReleaseCharBuf(buf);
	}
	
	ImGui::Separator();
	if (ImGui::MenuItem("Bypass keyboard shortcuts", NULL, &c64SettingsEmulatorScreenBypassKeyboardShortcuts))
	{
		C64DebuggerStoreSettings();
	}
}

bool CViewEmulatorScreen::DoGamePadButtonDown(CGamePad *gamePad, u8 button)
{
	LOGI("CViewEmulatorScreen::DoGamePadButtonDown: %s %d", gamePad->name, button);
	if (gamePad->index == c64SettingsSelectedJoystick1-2)
	{
		debugInterface->JoystickDown(0, ConvertSdlAxisToJoystickAxis(button));
	}
	if (gamePad->index == c64SettingsSelectedJoystick2-2)
	{
		debugInterface->JoystickDown(1, ConvertSdlAxisToJoystickAxis(button));
	}
	return false;
}

bool CViewEmulatorScreen::DoGamePadButtonUp(CGamePad *gamePad, u8 button)
{
	LOGI("CViewEmulatorScreen::DoGamePadButtonUp: %s %d", gamePad->name, button);
	if (gamePad->index == c64SettingsSelectedJoystick1-2)
	{
		debugInterface->JoystickUp(0, ConvertSdlAxisToJoystickAxis(button));
	}
	if (gamePad->index == c64SettingsSelectedJoystick2-2)
	{
		debugInterface->JoystickUp(1, ConvertSdlAxisToJoystickAxis(button));
	}
	return false;
}

bool CViewEmulatorScreen::DoGamePadAxisMotion(CGamePad *gamePad, u8 axis, int value)
{
//	LOGD("CViewEmulatorScreen::DoGamePadAxisMotion");
	return false;
}

int CViewEmulatorScreen::GetJoystickAxis(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// because Windows is totally messed up with right-Alt key, let's compare only keyCodes
	if (viewC64->mainMenuBar->kbsJoystickFire->keyCode == keyCode)
	{
		return JOYPAD_FIRE;
	}
	if (viewC64->mainMenuBar->kbsJoystickUp->keyCode == keyCode)
	{
		return JOYPAD_N;
	}
	if (viewC64->mainMenuBar->kbsJoystickDown->keyCode == keyCode)
	{
		return JOYPAD_S;
	}
	if (viewC64->mainMenuBar->kbsJoystickLeft->keyCode == keyCode)
	{
		return JOYPAD_W;
	}
	if (viewC64->mainMenuBar->kbsJoystickRight->keyCode == keyCode)
	{
		return JOYPAD_E;
	}
	return JOYPAD_IDLE;
}

bool CViewEmulatorScreen::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewEmulatorScreen::KeyDown: keyCode=%d", keyCode);
	
	if (c64SettingsEmulatorScreenBypassKeyboardShortcuts == false)
	{
		if (guiMain->CheckKeyboardShortcut(keyCode))
			return true;
	}

	if (keyCode == MTKEY_ENTER && isAlt)
	{
		viewC64->ToggleFullScreen(this);
		return true;
	}
	
	if (IsSkipKey(keyCode, isShift, isAlt, isControl, isSuper))
		return false;
	
	if (c64SettingsSelectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard
		|| c64SettingsSelectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
	{
		int joyAxis = GetJoystickAxis(keyCode, isShift, isAlt, isControl, isSuper);
		if (joyAxis != JOYPAD_IDLE)
		{
			debugInterface->LockIoMutex();
			if (c64SettingsSelectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickDown(0, joyAxis);
			}
			if (c64SettingsSelectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickDown(1, joyAxis);
			}
			debugInterface->UnlockIoMutex();
			return true;
		}
	}

	debugInterface->LockIoMutex();
	
	/*
	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);

	if (it == pressedKeyCodes.end())
	{
		pressedKeyCodes[keyCode] = true;
	}
	else
	{
		// key is already pressed
		LOGD("key %d is already pressed, skipping...", keyCode);
		debugInterface->UnlockIoMutex();
		return true;
	}
	 */
	
	
	keyCode = ConvertKeyCode(keyCode, isShift, isAlt, isControl, isSuper);

	bool consumed = debugInterface->KeyboardDown(keyCode);
	PostDebugInterfaceKeyDown(keyCode, isShift, isAlt, isControl, isSuper);
	debugInterface->UnlockIoMutex();
	return consumed;
}

bool CViewEmulatorScreen::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (c64SettingsEmulatorScreenBypassKeyboardShortcuts == false)
	{
		if (guiMain->CheckKeyboardShortcut(keyCode))
			return true;
	}

	return false;
}

bool CViewEmulatorScreen::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI(".......... CViewEmulatorScreen::KeyUp: keyCode=%d", keyCode);

	if (c64SettingsSelectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard
		|| c64SettingsSelectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
	{
		int joyAxis = GetJoystickAxis(keyCode, isShift, isAlt, isControl, isSuper);
		if (joyAxis != JOYPAD_IDLE)
		{
			debugInterface->LockIoMutex();
			if (c64SettingsSelectedJoystick1 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickUp(0, joyAxis);
			}
			if (c64SettingsSelectedJoystick2 == SelectedJoystick::SelectedJoystickKeyboard)
			{
				debugInterface->JoystickUp(1, joyAxis);
			}
			debugInterface->UnlockIoMutex();
			return true;
		}
	}

	debugInterface->LockIoMutex();
	
	/*
	std::map<u32, bool>::iterator it = pressedKeyCodes.find(keyCode);
	
	if (it == pressedKeyCodes.end())
	{
		// key is already not pressed
		LOGD("key %d is already not pressed, skipping...", keyCode);
		debugInterface->UnlockIoMutex();
		
		return true;
	}
	else
	{
		pressedKeyCodes.erase(it);
	}
	 */
	 

	keyCode = ConvertKeyCode(keyCode, isShift, isAlt, isControl, isSuper);
	
	bool consumed = debugInterface->KeyboardUp(keyCode);
	PostDebugInterfaceKeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	
	debugInterface->UnlockIoMutex();
	
	return consumed;
}

void CViewEmulatorScreen::JoystickDown(int port, u32 axis)
{
	LOGI("CViewEmulatorScreen::JoystickDown: axis=%02x", axis);
	debugInterface->LockIoMutex();
	debugInterface->JoystickDown(port, axis);
	debugInterface->UnlockIoMutex();
}

void CViewEmulatorScreen::JoystickUp(int port, u32 axis)
{
	debugInterface->LockIoMutex();
	debugInterface->JoystickUp(port, axis);
	debugInterface->UnlockIoMutex();
}

bool CViewEmulatorScreen::IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

u32 CViewEmulatorScreen::ConvertKeyCode(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return keyCode;
}

void CViewEmulatorScreen::PostDebugInterfaceKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
}

void CViewEmulatorScreen::PostDebugInterfaceKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
}

bool CViewEmulatorScreen::DoTap(float x, float y)
{
	return CGuiViewMovingPaneImage::DoTap(x, y);
}

bool CViewEmulatorScreen::DoFinishTap(float x, float y)
{
	return CGuiViewMovingPaneImage::DoFinishTap(x, y);
}

bool CViewEmulatorScreen::DoRightClick(float x, float y)
{
	return CGuiViewMovingPaneImage::DoRightClick(x, y);
}

bool CViewEmulatorScreen::DoFinishRightClick(float x, float y)
{
	return CGuiViewMovingPaneImage::DoFinishRightClick(x, y);
}

bool CViewEmulatorScreen::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiViewMovingPaneImage::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewEmulatorScreen::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiViewMovingPaneImage::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewEmulatorScreen::DoScrollWheel(float deltaX, float deltaY)
{
	return CGuiViewMovingPaneImage::DoFinishTap(deltaX, deltaY);
}

CViewEmulatorScreen::~CViewEmulatorScreen()
{
	CGuiViewMovingPaneImage::~CGuiViewMovingPaneImage();
}

