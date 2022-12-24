#ifndef _CVIEWSIDTRACKERHISTORY_H_
#define _CVIEWSIDTRACKERHISTORY_H_

#include "CGuiView.h"
#include "CGuiButtonSwitch.h"
#include "CGuiLockableList.h"
#include "CGuiLabel.h"
#include "CPianoKeyboard.h"
#include "DebuggerDefs.h"

class CDebugInterfaceVice;

class CViewC64SidTrackerHistory : public CGuiView, CGuiButtonSwitchCallback, CGuiListCallback, public CPianoKeyboardCallback
{
public:
	CViewC64SidTrackerHistory(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceVice *debugInterface);

	virtual void Render();
	virtual void RenderImGui();
	virtual bool DoScrollWheel(float deltaX, float deltaY);
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool DoTap(float x, float y);
	virtual void RenderFocusBorder();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	CDebugInterfaceVice *debugInterface;
	
	CSlrFont *font;
	float fontSize;
	int numVisibleTrackLines;
	
	CGuiButtonSwitch *btnFade;
	CGuiButtonSwitch *btnScrub;
	CGuiButtonSwitch *btnJazz;

	int selectedNumSteps;
	
	CGuiLabel *lblStep;
	CGuiButtonSwitch *btnStep1;
	CGuiButtonSwitch *btnStep2;
	CGuiButtonSwitch *btnStep3;
	CGuiButtonSwitch *btnStep4;
	CGuiButtonSwitch *btnStep5;
	CGuiButtonSwitch *btnStep6;
	CGuiButtonSwitch *btnStep8;
	std::list<CGuiButtonSwitch *> btnsStepSwitches;
	void UpdateButtonsGroup(CGuiButtonSwitch *btn);
	void SetNumSteps(int numSteps);
	
	char **txtSidChannels;
	CGuiLabel *lblMidiIn;
	CGuiLockableList *lstMidiIn;
	CGuiLabel *lblMidiOut;
	CGuiLockableList *lstMidiOut;
	
	//
	CGuiButtonSwitch *btnShowNotes;
	CGuiButtonSwitch *btnShowInstruments;
	CGuiButtonSwitch *btnShowPWM;
	CGuiButtonSwitch *btnShowAdsr;
	CGuiButtonSwitch *btnShowFilterCutoff;
	CGuiButtonSwitch *btnShowFilterCtrl;
	CGuiButtonSwitch *btnShowVolume;

	void UpdateMidiListSidChannels();
	float fScrollPosition;
	int scrollPosition;
	void EnsureCorrectScrollPosition();
	void SetSidWithCurrentPositionData();
	void SetTracksScrollPos(int newPos);
	
	void ResetScroll();
	void MoveTracksY(float deltaY);
	
	//
	void UpdateHistoryWithCurrentSidData();
	
	//
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);

	// callback from debug interface
	void VSyncStepsAdded();
	
	// callbacks from SidPianoKeyboard
	virtual void PianoKeyboardNotePressed(CPianoKeyboard *pianoKeyboard, CPianoKey *pianoKey);
	virtual void PianoKeyboardNoteReleased(CPianoKeyboard *pianoKeyboard, CPianoKey *pianoKey);
	
	CPianoKey *pressedKeys[C64_MAX_NUM_SIDS * 3];
	
	// layout params
	bool showController;
	bool showChannels;
	bool isScrub;
};

#endif
