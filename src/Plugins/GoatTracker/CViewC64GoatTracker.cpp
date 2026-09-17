#include "CViewC64.h"
#include "CViewC64GoatTracker.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CGT2RenoiseInput.h"
#include "CViewGT2Patterns.h"
#include "CViewGT2Instrument.h"
#include "GT2ViewCommon.h"
#include "CPluginsManager.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "C64SettingsStorage.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"
#include "VID_ImageBinding.h"
#include "CViewDataMap.h"
#include "CGuiMain.h"
#include "CGuiEvent.h"
#include "SYS_Funct.h"

extern "C" {
#include "bme_cfg.h"
#include "bme_main.h"
#include "gconsole.h"
#include "gcommon.h"
#include "gplay.h"
#include "gpattern.h"
#include "bme_win.h"

unsigned int mapSdlKeyToBmeKey(unsigned int sdlKey);

// From gsong.c / goattrk2.c — declared explicitly to avoid pulling in
// goattrk2.h (which redefines BME structs in this translation unit).
extern int numarpcolumns;
extern int pattlen[];
extern int epnum[];
extern int editmode;
}

#define EDIT_PATTERN 0

CViewC64GoatTracker::CViewC64GoatTracker(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "Goat Tracker plugin";
	
	mutex = new CSlrMutex("CViewC64GoatTracker");
	
	font = viewC64->fontDefaultCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	int rasterW = NextPow2(MAX_COLUMNS * 8);
	int rasterH = NextPow2(MAX_ROWS * 16);
	imageDataScreen = new CImageData(rasterW, rasterH, IMG_TYPE_RGBA);
	imageDataScreen->AllocImage(false, true);

	screenTexEndX = (float)(MAX_COLUMNS * 8) / (float)rasterW;
	screenTexEndY = (float)(MAX_ROWS * 16) / (float)rasterH;

	imageScreen = new CSlrImage(true, false);
	imageScreen->LoadImageForRebinding(imageDataScreen, RESOURCE_PRIORITY_STATIC);
	imageScreen->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	imageScreen->resourceIsActive = true;
	VID_PostImageBinding(imageScreen, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);
}

CViewC64GoatTracker::~CViewC64GoatTracker()
{
}

void CViewC64GoatTracker::DoLogic()
{
	
}

void CViewC64GoatTracker::Render()
{
}

void CViewC64GoatTracker::RenderImGui()
{
//	LOGD("CViewC64GoatTracker::RenderImGui");
	
	float w = (float)(MAX_COLUMNS * 8);
	float h = (float)(MAX_ROWS * 16);

	this->imGuiWindowAspectRatio = w/h;
	this->imGuiWindowKeepAspectRatio = true;

	PreRenderImGui();

	if (!this->visible)
	{
		PostRenderImGui();
		gPluginsManager->Deactivate("GoatTracker");
		return;
	}

//		for (int x = 0; x < imageDataScreen->width; x++)
//		{
//			for (int y = 0; y < imageDataScreen->height; y++)
//			{
//				float wx = ((float)x / (float)imageDataScreen->width) * 255.0f;
//				float wy = ((float)y / (float)imageDataScreen->height) * 255.0f;
//
//				imageDataScreen->SetPixelResultRGBA(x, y, (int)wx, (int)wy, 0, 255);
//			}
//		}

	imageScreen->ReBindImageData(imageDataScreen);

	// blit texture of the screen
	Blit(imageScreen,
		 posX,
		 posY, -1,
		 sizeX,
		 sizeY,
		 0.0f, 0.0f, screenTexEndX, screenTexEndY);
	
	
//	ImGui::Text("Test");
	
	PostRenderImGui();
	
//	LOGD("CViewC64GoatTracker::RenderImGui done");
}

void CViewC64GoatTracker::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewC64GoatTracker::DoTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoTap:  x=%f y=%f", x, y);
	
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
//	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN, xi, yi);
	AddEvent(eventMouse);

	return true; //CGuiView::DoTap(x, y);
}

bool CViewC64GoatTracker::DoFinishTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoFinishTap: %f %f", x, y);

	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
//	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_UP, xi, yi);
	AddEvent(eventMouse);

	return true; //CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64GoatTracker::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64GoatTracker::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64GoatTracker::DoRightClick(float x, float y)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_RIGHT_BUTTON_DOWN, xi, yi);
	AddEvent(eventMouse);

	return true;
}

bool CViewC64GoatTracker::DoFinishRightClick(float x, float y)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_RIGHT_BUTTON_UP, xi, yi);
	AddEvent(eventMouse);

	return true;
}

bool CViewC64GoatTracker::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
//	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_MOVE, xi, yi);
	AddEvent(eventMouse);

	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64GoatTracker::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64GoatTracker::DoNotTouchedMove(float x, float y)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
//	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_MOVE, xi, yi);
	AddEvent(eventMouse);

	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewC64GoatTracker::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64GoatTracker::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64GoatTracker::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64GoatTracker::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64GoatTracker::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64GoatTracker::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64GoatTracker::KeyDown: keyCode=%d");

	// Global undo / redo — one shared GT2 history.
	if ((isControl || isSuper) && !isAlt && pluginGoatTracker && pluginGoatTracker->viewPatterns)
	{
		if (!isShift && (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z))
		{
			pluginGoatTracker->viewPatterns->UndoPatternEdit();
			return true;
		}
		if (keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
		{
			pluginGoatTracker->viewPatterns->RedoPatternEdit();
			return true;
		}
	}

	// Sustain column edit (when the cursor is parked there) — must run
	// before renoiseInput / HandleArpKey / native GT2 so the hex digit
	// goes to CMD_SETSR instead of the instrument byte.
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleSustainColumnKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Renoise keyboard layout: every Renoise rebinding goes through the
	// dispatcher so the binding takes effect whether this main text-mode
	// window or one of the ImGui GT2 windows has focus. See CGT2RenoiseInput.
	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Enter on an instrument's table-pointer field navigates the ImGui
	// table view (and allocates a row when the pointer is zero) instead
	// of falling through to native GT2's legacy table editor.
	if (keyCode == MTKEY_ENTER && pluginGoatTracker && pluginGoatTracker->viewInstrument
		&& pluginGoatTracker->viewInstrument->HandleInstrumentTablePointerEnter(
			isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Arp-column nav + note entry. Routes through CViewGT2Patterns so the
	// behavior is identical whether the main GT2 window or the ImGui
	// patterns window has focus. GT2's own cursor logic has no concept
	// of arp columns, so without this intercept Left/Right at the main↔arp
	// boundary and note keys typed in arp columns would be lost.
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleArpKey(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Main-track note entry + main-track cursor navigation. Without these,
	// bare arrow keys from this view fall through to native gpattern.c
	// which advances eppos but does NOT run our cursor-leads-playback
	// SeekPlayerToCursorIfPlaying hook — so scrubbing during playback
	// worked from the ImGui patterns view but did nothing from the
	// embedded GT2 main view (user-reported regression).
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleMainTrackNoteEntry(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns
		&& pluginGoatTracker->viewPatterns->HandleMainTrackNavigation(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	// Forwarding non-modifiers to GT2 leaves any arp-column cursor state.
	// Pure modifiers only update GT2's key state and must not move the cursor.
	if (pluginGoatTracker && pluginGoatTracker->viewPatterns)
	{
		if (!GT2_IsModifierKey(keyCode))
		{
			pluginGoatTracker->viewPatterns->eparpcol = -1;
		}
	}

	CGuiEventKeyboard *eventKeyboard = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_DOWN, keyCode, keyCode);
	AddEvent(eventKeyboard);

	return true;
}

bool CViewC64GoatTracker::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64GoatTracker::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64GoatTracker::KeyUp: keyCode=%d", keyCode);
	if (pluginGoatTracker && pluginGoatTracker->renoiseInput
		&& pluginGoatTracker->renoiseInput->HandleKeyUp(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	CGuiEventKeyboard *eventKeyboard = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_UP, keyCode, keyCode);
	AddEvent(eventKeyboard);

	return true;
}

bool CViewC64GoatTracker::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64GoatTracker::AddEvent(CGuiEvent *event)
{
	mutex->Lock();
	events.push_back(event);
	mutex->Unlock();
}

u32 CViewC64GoatTracker::GetForwardedAsciiKeyForEvent(u32 mtKey)
{
	if (GT2_IsModifierKey(mtKey))
		return 0;
	if (mtKey > 0xFFFF)
		return (mtKey & 0xFF) | 0x80;
	return mtKey;
}

void CViewC64GoatTracker::ForwardEvents()
{
//	LOGD("CViewC64GoatTracker::ForwardEvents");

	// Re-sync native GT2's win_keystate modifier flags from the engine
	// every frame, BEFORE draining the event queue. The engine itself
	// already mirrors the live OS state into guiMain->is*Pressed each
	// frame, but native GT2 only learns about a modifier through
	// GT2_ForwardKeyDown / GT2_ForwardKeyUp — which fire only when a
	// GT2 view has focus AT THE EXACT TIME of the SDL key event. If
	// the user releases Shift mid-drag (Shift+drag-docking an instrument
	// onto patterns is the canonical reproducer), the SDL_EVENT_KEY_UP is
	// either consumed by the drag handover or delivered while focus
	// is not on any GT2 view, so win_keystate[KEY_LEFTSHIFT] stays at
	// 1 forever. Subsequent 'Q' is then interpreted as Shift+Q (Renoise
	// transpose-up), 'F' is interpreted as Shift+F instead of a bare
	// hex digit, etc. — exactly the chaos the user reported. Mirroring
	// the live modifier state every frame recovers from any missed
	// modifier KEYUP on the very next ForwardEvents tick.
	// The GT2 thread calls this; guiMain is gone once the app is tearing down.
	if (guiMain == NULL)
		return;

	win_keystate[KEY_LEFTSHIFT]  = guiMain->isShiftPressed   ? 1 : 0;
	win_keystate[KEY_RIGHTSHIFT] = guiMain->isShiftPressed   ? 1 : 0;
	win_keystate[KEY_CTRL]       = guiMain->isControlPressed ? 1 : 0;
	win_keystate[KEY_LEFTCTRL]   = guiMain->isControlPressed ? 1 : 0;
	win_keystate[KEY_RIGHTCTRL]  = guiMain->isControlPressed ? 1 : 0;
	win_keystate[KEY_ALT]        = guiMain->isAltPressed     ? 1 : 0;
	win_keystate[KEY_RIGHTALT]   = guiMain->isAltPressed     ? 1 : 0;

	// transfer accumulated events to gt2
	mutex->Lock();
	
	while(!events.empty())
	{
		CGuiEvent *event = events.front();
		events.pop_front();
		
//		LOGD("event type=%d", event->type);
		if (event->type == GUI_EVENT_TYPE_MOUSE)
		{
			CGuiEventMouse *eventMouse = (CGuiEventMouse *)event;
			
//			LOGD("CGuiEventMouse mouseState=%d x=%d y=%d", eventMouse->mouseState, eventMouse->x, eventMouse->y);
			
			gt2SetMousePosition(eventMouse->x, eventMouse->y);

			if (eventMouse->mouseState == GUI_EVENT_MOUSE_MOVE)
			{
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN)
			{
				LOGD("GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN");
				win_mousebuttons |= MOUSEB_LEFT;
				
				// that's all in this loop, as we may consume button up in next iteration
				break;
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_LEFT_BUTTON_UP)
			{
				LOGD("GUI_EVENT_MOUSE_LEFT_BUTTON_UP");
				win_mousebuttons &= ~MOUSEB_LEFT;
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_RIGHT_BUTTON_DOWN)
			{
				LOGD("GUI_EVENT_MOUSE_RIGHT_BUTTON_DOWN");
				win_mousebuttons |= MOUSEB_RIGHT;
				break;
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_RIGHT_BUTTON_UP)
			{
				LOGD("GUI_EVENT_MOUSE_RIGHT_BUTTON_UP");
				win_mousebuttons &= ~MOUSEB_RIGHT;
			}
			
//			LOGD("eventMouse->x=%d eventMouse->y=%d win_mousexpos=%d win_mouseypos=%d",
//				 eventMouse->x, eventMouse->y, win_mousexpos, win_mouseypos);
		}
		else if (event->type == GUI_EVENT_TYPE_KEYBOARD)
		{
			CGuiEventKeyboard *eventKeyboard = (CGuiEventKeyboard *)event;
			
			u32 bmeKey = mapSdlKeyToBmeKey(eventKeyboard->mtKey);
			
			if (eventKeyboard->keyboardState == GUI_EVENT_KEYBOARD_KEY_DOWN)
			{
				win_asciikey = GetForwardedAsciiKeyForEvent(eventKeyboard->mtKey);
				virtualkeycode = win_asciikey ? win_asciikey : 0xff;

				u32 keynum = bmeKey; //event.key.keysym.sym;

				LOGD("ForwardEvents: eventKeyboard->mtKey=0x%x win_asciikey=0x%x bmeKey=%d", eventKeyboard->mtKey, win_asciikey, bmeKey);

				if (keynum < MAX_KEYS)
				{
					win_keytable[keynum] = 1;
					win_keystate[keynum] = 1;
//					if ((keynum == KEY_ENTER) && ((win_keystate[KEY_ALT]) || (win_keystate[KEY_RIGHTALT])))
//					{
//						win_fullscreen ^= 1;
//						gfx_reinit();
//					}
				}
				
				// that's all in this loop as we may consume key up in next iteration
				break;
			}
			else if (eventKeyboard->keyboardState == GUI_EVENT_KEYBOARD_KEY_UP)
			{
				u32 keynum = bmeKey; //eventKeyboard->mtKey; //event.key.keysym.sym;
				if (keynum < MAX_KEYS)
				{
					win_keytable[keynum] = 0;
					win_keystate[keynum] = 0;
				}
			}
		}
	
		/*
		 
		 enum GuiEventKeyboardState : u8
		 {
			 GUI_EVENT_KEYBOARD_KEY_DOWN,
			 GUI_EVENT_KEYBOARD_KEY_UP,
			 GUI_EVENT_KEYBOARD_KEY_REPEAT
		 };

		 enum GuiEventMouseState : u8
		 {
			 GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_LEFT_BUTTON_UP,
			 GUI_EVENT_MOUSE_RIGHT_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_RIGHT_BUTTON_UP,
			 GUI_EVENT_MOUSE_MID_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_MID_BUTTON_UP,
			 
	case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
	joybuttons[event.jbutton.which] |= 1 << event.jbutton.button;
	break;

	case SDL_EVENT_JOYSTICK_BUTTON_UP:
	joybuttons[event.jbutton.which] &= ~(1 << event.jbutton.button);
	break;

	case SDL_EVENT_JOYSTICK_AXIS_MOTION:
	switch (event.jaxis.axis)
	{
		case 0:
		joyx[event.jaxis.which] = event.jaxis.value;
		break;

		case 1:
		joyy[event.jaxis.which] = event.jaxis.value;
		break;
	}
	break;


	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	switch(event.button.button)
	{
		case SDL_BUTTON_MIDDLE:
		win_mousebuttons |= MOUSEB_MIDDLE;
		break;

		case SDL_BUTTON_RIGHT:
		win_mousebuttons |= MOUSEB_RIGHT;
		break;
	}
	break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
	switch(event.button.button)
	{
		case SDL_BUTTON_MIDDLE:
		win_mousebuttons &= ~MOUSEB_MIDDLE;
		break;

		case SDL_BUTTON_RIGHT:
		win_mousebuttons &= ~MOUSEB_RIGHT;
		break;
	}
	break;

	case SDL_EVENT_QUIT:
	win_quitted = 1;
	break;

	case SDL_EVENT_KEY_DOWN:
   // win_virtualkey = event.key.keysym.sym;
	win_asciikey = event.key.keysym.unicode;
	keynum = event.key.keysym.sym;
	if (keynum < MAX_KEYS)
	{
		win_keytable[keynum] = 1;
		win_keystate[keynum] = 1;
		if ((keynum == KEY_ENTER) && ((win_keystate[KEY_ALT]) || (win_keystate[KEY_RIGHTALT])))
		{
			win_fullscreen ^= 1;
			gfx_reinit();
		}
	}
	break;

	case SDL_EVENT_KEY_UP:
	keynum = event.key.keysym.sym;
	if (keynum < MAX_KEYS)
	{
		win_keytable[keynum] = 0;
		win_keystate[keynum] = 0;
	}
	break;

	case SDL_VIDEORESIZE:
	case SDL_VIDEOEXPOSE:
	gfx_redraw = 1;
	break;
				 
		 */
	
	
		delete event;
	}

	mutex->Unlock();
}

void CViewC64GoatTracker::ActivateView()
{
	LOGG("CViewC64GoatTracker::ActivateView()");
}

void CViewC64GoatTracker::DeactivateView()
{
	LOGG("CViewC64GoatTracker::DeactivateView()");
}
