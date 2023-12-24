extern "C" {
#include "sid.h"
}
#include "CViewC64SidTrackerHistory.h"
#include "C64Tools.h"
#include "C64SIDFrequencies.h"
#include "CDebugInterfaceVice.h"
#include "CViewC64.h"
#include "C64SIDFrequencies.h"
#include "CGuiMain.h"
#include "SND_Main.h"
#include "SYS_KeyCodes.h"
#include "SND_SoundEngine.h"
#include "CLayoutParameter.h"
#include "CViewC64SidPianoKeyboard.h"
#include "C64SIDDump.h"
#include "SYS_DefaultConfig.h"

// TODO: load tracker font from MG Tracker

CViewC64SidTrackerHistory::CViewC64SidTrackerHistory(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceVice *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;
	
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	for (int i = 0; i < C64_MAX_NUM_SIDS * 3; i++)
	{
		pressedKeys[i] = NULL;
	}

	this->selectedNumSteps = 1;
	this->fScrollPosition = 0;
	this->scrollPosition = 0;
	
	// sid dump settings
	viewC64->config->GetInt ("ViewC64SidTrackerHistorySidDumpBaseFreq", &basefreq, 0);
	viewC64->config->GetInt ("ViewC64SidTrackerHistorySidDumpBaseNote", &basenote, 0xB0);
	viewC64->config->GetBool("ViewC64SidTrackerHistorySidDumpTimeSeconds", &timeseconds, false);
	viewC64->config->GetInt ("ViewC64SidTrackerHistorySidDumpOldNoteFactor", &oldnotefactor, 1);
	viewC64->config->GetInt ("ViewC64SidTrackerHistorySidDumpPattSpacing", &pattspacing, 0);
	viewC64->config->GetInt ("ViewC64SidTrackerHistorySidDumpSpacing", &spacing, 0);
	viewC64->config->GetBool("ViewC64SidTrackerHistorySidDumpLowResolution", &lowres, false);

	siddumpFileExtensions.push_back(new CSlrString("txt"));
	csvFileExtensions.push_back(new CSlrString("csv"));

	// this value is calculated automatically
	this->numVisibleTrackLines = 1;
	
	this->font = viewC64->fontDisassembly;
	fontSize = 9;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	CSlrFont *fontButton = viewC64->fontCBMShifted;
	float fontScale = 1.8;
	float fontHeight = fontButton->GetCharHeight('@', fontScale) + 2;
	
	float psx = 2*fontScale;//0; //posX; // + fontSize * 65;
	float psy = 2*fontScale;//0; //posY;
	
	float px = psx;
	float py = psy;
	
	float buttonSizeX = 23.0f * fontScale;
	float buttonSizeY = 7.5f * fontScale;
	float textButtonOffsetY = buttonSizeY/2.0f - fontHeight*0.33;
	float gapX = 1.0f * fontScale;
	float gapY = 0.5f * fontScale;

	showController = true;
	AddLayoutParameter(new CLayoutParameterBool("Show controller", &showController));

	showChannels = false;
	AddLayoutParameter(new CLayoutParameterBool("Show channels", &showChannels));

	isScrub = true;
	AddLayoutParameter(new CLayoutParameterBool("Scrub on scroll", &isScrub));
	
	btnFade = NULL;
	btnScrub = NULL;
	
//	btnFade = new CGuiButtonSwitch(NULL, NULL, NULL,
//									px, py, posZ, buttonSizeX, buttonSizeY,
//									new CSlrString("FADE"),
//									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
//									fontButton, fontScale,
//									1.0, 1.0, 1.0, 1.0,
//									1.0, 1.0, 1.0, 1.0,
//									0.3, 0.3, 0.3, 1.0,
//									this);
//	btnFade->SetOn(true);
//	this->AddGuiElement(btnFade);
//
//	px += buttonSizeX + gapX;
////	py += buttonSizeY + gapY;
//
//	btnScrub = new CGuiButtonSwitch(NULL, NULL, NULL,
//								   px, py, posZ, buttonSizeX, buttonSizeY,
//								   new CSlrString("SCRUB"),
//								   FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
//								   fontButton, fontScale,
//								   1.0, 1.0, 1.0, 1.0,
//								   1.0, 1.0, 1.0, 1.0,
//								   0.3, 0.3, 0.3, 1.0,
//								   this);
//	btnScrub->SetOn(true);
//	this->AddGuiElement(btnScrub);

	//
	px = psx;
	py = psy;// + 17;
	gapY = 5.0f;
	buttonSizeY = 8.0f * fontScale;
//	lblStep = new CGuiLabel("STEPS", false, px, py, posZ, buttonSizeX, fontSize, LABEL_ALIGNED_LEFT, fontSize, fontSize, NULL);
//	this->AddGuiElement(lblStep);
//
//	py += fontSize;
	
	buttonSizeX = 32;
	btnStep1 = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("1"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
										  fontButton, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
	btnStep1->SetOn(true);
	this->AddGuiElement(btnStep1);
	btnsStepSwitches.push_back(btnStep1);

	px += buttonSizeX + gapX;

	btnStep2 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("2"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep2);
	btnsStepSwitches.push_back(btnStep2);

	px += buttonSizeX + gapX;
	
	btnStep3 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("3"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep3);
	btnsStepSwitches.push_back(btnStep3);

	px += buttonSizeX + gapX;
	
	btnStep4 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("4"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep4);
	btnsStepSwitches.push_back(btnStep4);

	px += buttonSizeX + gapX;

	btnStep5 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("5"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep5);
	btnsStepSwitches.push_back(btnStep5);

	px += buttonSizeX + gapX;
	
	btnStep6 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("6"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep6);
	btnsStepSwitches.push_back(btnStep6);
	
	px += buttonSizeX + gapX;

	btnStep8 = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("8"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	this->AddGuiElement(btnStep8);
	btnsStepSwitches.push_back(btnStep8);

	py += buttonSizeY + gapY;
	px = psx;
	gapY = 1.0f;
	
	///////////////
	
	
	bool showTrackerButtons = true;
	if (showTrackerButtons)
	{
		// WTF is this mess here??? TODO: BUG STATIC VALUES IN SIDTRACKER
		buttonSizeX = 24.6f * fontScale;
		px = psx; //320;
//		py = psy + 64*fontScale; //137.6;
		
		btnShowNotes = new CGuiButtonSwitch(NULL, NULL, NULL,
									   px, py, posZ, buttonSizeX, buttonSizeY,
									   new CSlrString("NOTES"),
									   FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									   fontButton, fontScale,
									   1.0, 1.0, 1.0, 1.0,
									   1.0, 1.0, 1.0, 1.0,
									   0.3, 0.3, 0.3, 1.0,
									   this);
		btnShowNotes->SetOn(true);
		this->AddGuiElement(btnShowNotes);

		px += buttonSizeX;

		btnShowInstruments = new CGuiButtonSwitch(NULL, NULL, NULL,
											px, py, posZ, buttonSizeX, buttonSizeY,
											new CSlrString("WAVE"),
											FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
											fontButton, fontScale,
											1.0, 1.0, 1.0, 1.0,
											1.0, 1.0, 1.0, 1.0,
											0.3, 0.3, 0.3, 1.0,
											this);
		btnShowInstruments->SetOn(true);
		this->AddGuiElement(btnShowInstruments);
		
		px += buttonSizeX;

		btnShowAdsr = new CGuiButtonSwitch(NULL, NULL, NULL,
										  px, py, posZ, buttonSizeX, buttonSizeY,
										  new CSlrString("ADSR"),
										  FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
										  fontButton, fontScale,
										  1.0, 1.0, 1.0, 1.0,
										  1.0, 1.0, 1.0, 1.0,
										  0.3, 0.3, 0.3, 1.0,
										  this);
		btnShowAdsr->SetOn(true);
		this->AddGuiElement(btnShowAdsr);
		
		px += buttonSizeX;

		btnShowPWM = new CGuiButtonSwitch(NULL, NULL, NULL,
												  px, py, posZ, buttonSizeX, buttonSizeY,
												  new CSlrString("PWM"),
												  FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
												  fontButton, fontScale,
												  1.0, 1.0, 1.0, 1.0,
												  1.0, 1.0, 1.0, 1.0,
												  0.3, 0.3, 0.3, 1.0,
												  this);
		btnShowPWM->SetOn(true);
		this->AddGuiElement(btnShowPWM);
		
		px += buttonSizeX;

		btnShowFilterCutoff = new CGuiButtonSwitch(NULL, NULL, NULL,
										   px, py, posZ, buttonSizeX, buttonSizeY,
										   new CSlrString("CUTOFF"),
										   FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
										   fontButton, fontScale,
										   1.0, 1.0, 1.0, 1.0,
										   1.0, 1.0, 1.0, 1.0,
										   0.3, 0.3, 0.3, 1.0,
										   this);
		btnShowFilterCutoff->SetOn(true);
		this->AddGuiElement(btnShowFilterCutoff);
		
		px += buttonSizeX;

		btnShowFilterCtrl = new CGuiButtonSwitch(NULL, NULL, NULL,
												   px, py, posZ, buttonSizeX, buttonSizeY,
												   new CSlrString("FILT"),
												   FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
												   fontButton, fontScale,
												   1.0, 1.0, 1.0, 1.0,
												   1.0, 1.0, 1.0, 1.0,
												   0.3, 0.3, 0.3, 1.0,
												   this);
		btnShowFilterCtrl->SetOn(true);
		this->AddGuiElement(btnShowFilterCtrl);
		
		px += buttonSizeX;

		btnShowVolume = new CGuiButtonSwitch(NULL, NULL, NULL,
												 px, py, posZ, buttonSizeX, buttonSizeY,
												 new CSlrString("VOLUME"),
												 FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
												 fontButton, fontScale,
												 1.0, 1.0, 1.0, 1.0,
												 1.0, 1.0, 1.0, 1.0,
												 0.3, 0.3, 0.3, 1.0,
												 this);
		btnShowVolume->SetOn(true);
		this->AddGuiElement(btnShowVolume);
		
		px += buttonSizeX;
	 }

	
	
	/////////////// MIDI
	
	lblMidiIn = new CGuiLabel("KEY IN", false, px, py, posZ, buttonSizeX, fontSize, LABEL_ALIGNED_LEFT, fontSize, fontSize, NULL);
	this->AddGuiElement(lblMidiIn);
	lblMidiIn->visible = false;
	
//	py -= 1.0f;
	py += fontSize * 2;
	
	px = psy; //fontSize * 6.5f;

	btnJazz = new CGuiButtonSwitch(NULL, NULL, NULL,
									px, py, posZ, buttonSizeX, buttonSizeY,
									new CSlrString("JAZZ"),
									FONT_ALIGN_CENTER, buttonSizeX/2, textButtonOffsetY,
									fontButton, fontScale,
									1.0, 1.0, 1.0, 1.0,
									1.0, 1.0, 1.0, 1.0,
									0.3, 0.3, 0.3, 1.0,
									this);
	btnJazz->SetOn(false);
	this->AddGuiElement(btnJazz);
	
	py += 1.0f;
	
	px = psx;
	py += fontSize + gapY;

	float lstFontSize = fontSize;
	float listWidth = lstFontSize*19;
	float listHeight = lstFontSize*9;

	this->lstMidiIn = new CGuiLockableList(px, py, posZ+0.01, listWidth, listHeight, lstFontSize,
										NULL, 0, false,
										viewC64->fontDisassembly,
										guiMain->theme->imgBackground, 1.0f,
										this);
	this->lstMidiIn->name = "CViewSIDTrackerHistory::lstMidiIn";
	this->lstMidiIn->SetGaps(0.0f, -0.25f);
	this->lstMidiIn->drawFocusBorder = false;
	this->lstMidiIn->allowFocus = false;
	this->AddGuiElement(this->lstMidiIn);

	py += listHeight;
	
	/*

	lblMidiOut = new CGuiLabel("MIDI OUT", false, px, py, posZ, buttonSizeX, fontSize, LABEL_ALIGNED_LEFT, fontSize, fontSize, NULL);
	this->AddGuiElement(lblMidiOut);

	py += fontSize + gapY;
	this->lstMidiOut = new CGuiLockableList(px, py, posZ+0.01, listWidth, listHeight, lstFontSize,
								   NULL, 0, false,
									viewC64->fontDisassembly,
								   guiMain->theme->imgBackground, 1.0f,
								   this);
	this->lstMidiOut->name = "CViewSIDTrackerHistory::lstMidiOut";
	this->lstMidiOut->SetGaps(0.0f, -0.25f);
	this->lstMidiOut->drawFocusBorder = false;
	this->lstMidiOut->allowFocus = false;
	this->AddGuiElement(this->lstMidiOut);
	 */
	
	txtSidChannels = NULL;
	UpdateMidiListSidChannels();
	
	///
	

	
}

void CViewC64SidTrackerHistory::UpdateMidiListSidChannels()
{
	guiMain->LockMutex();

	// TODO: when number of SIDs changes we need to rebuild the list
	int numSids = debugInterface->numSids;
	
	if (txtSidChannels != NULL)
	{
		for (int i = 0; i < numSids * 3; i++)
		{
			delete [] txtSidChannels[i];
		}
		delete txtSidChannels;
	}
	
	txtSidChannels = new char *[numSids * 3];

	int i = 0;
	for (int sidNum = 0; sidNum < numSids; sidNum++)
	{
		for (int chanNum = 0; chanNum < 3; chanNum++)
		{
			char *txtChan = new char[40];
			
			if (numSids == 1)
			{
				sprintf(txtChan, "CHANNEL #%d", chanNum+1);
			}
			else
			{
				sprintf(txtChan, "SID %d CHANNEL #%d", sidNum+1, chanNum+1);
			}
			
			txtSidChannels[i] = txtChan;
			i++;
		}
	}

	this->lstMidiIn->Init(txtSidChannels, numSids * 3, false);
//	this->lstMidiOut->Init(txtSidChannels, numSids * 3, false);
	
	this->lstMidiIn->SetElement(0, false, false);
//	this->lstMidiOut->SetElement(0, false, false);

	guiMain->UnlockMutex();
}

bool CViewC64SidTrackerHistory::DoTap(float x, float y)
{	
	return CGuiView::DoTap(x, y);
}

void CViewC64SidTrackerHistory::SetSidWithCurrentPositionData()
{
	gSoundEngine->LockMutex("CViewSIDTrackerHistory::SetSidWithCurrentPositionData");
	debugInterface->mutexSidDataHistory->Lock();

	std::list<CSidData *>::iterator it = debugInterface->sidDataHistory.begin();
	int sy = scrollPosition;
	std::advance(it, sy);
	
	CSidData *pSidDataCurrent  = *it;
	debugInterface->SetSid(pSidDataCurrent);
	
	debugInterface->mutexSidDataHistory->Unlock();
	gSoundEngine->UnlockMutex("CViewSIDTrackerHistory::SetSidWithCurrentPositionData");
}

void CViewC64SidTrackerHistory::PianoKeyboardNotePressed(CPianoKeyboard *pianoKeyboard, CPianoKey *pianoKey)
{
	LOGD("CViewSIDTrackerHistory::PianoKeyboardNotePressed: note=%d", pianoKey->keyNote);
	
	// TODO: SID SELECT
	
	int sidNum = 0;
	
	if (!btnJazz->IsOn() && lstMidiIn->selectedElement < 0)
		return;

	debugInterface->mutexSidDataHistory->Lock();

	int chanNum = lstMidiIn->selectedElement;

	if (btnJazz->IsOn())
	{
		// find available channel
		for (int i = 0; i < 3; i++)
		{
			if (pressedKeys[sidNum*3 + chanNum] == NULL)
			{
				break;
			}
			
			chanNum = (chanNum + 1) % 3;
		}
		
		LOGD("jazz PRESS chanNum=%d note=%d", chanNum, pianoKey->keyNote);
//		lstMidiIn->SetElement(chanNum, false, false);
	}
	
	std::list<CSidData *>::iterator it = debugInterface->sidDataHistory.begin();
	int sy = scrollPosition;
	std::advance(it, sy);
	
	CSidData *pSidDataCurrent  = *it;

	const sid_frequency_t *sidFrequencyData = SidNoteToFrequency(pianoKey->keyNote);
	
	int o = chanNum * 0x07;
	pSidDataCurrent->sidRegs[sidNum][o + 1] = (sidFrequencyData->sidValue & 0xFF00) >> 8;
	pSidDataCurrent->sidRegs[sidNum][o    ] = (sidFrequencyData->sidValue & 0x00FF);

	// gate on
	pSidDataCurrent->sidRegs[sidNum][o + 4] = pSidDataCurrent->sidRegs[sidNum][o + 4] | 0x01;

	debugInterface->SetSid(pSidDataCurrent);

	//
	pressedKeys[sidNum * 3 + chanNum] = pianoKey;
	
	if (btnJazz->IsOn())
	{
		chanNum = (chanNum + 1) % 3;

		lstMidiIn->SetElement(chanNum, false, false);
	}
	
	debugInterface->mutexSidDataHistory->Unlock();
}

// store new SidData in history at current position
void CViewC64SidTrackerHistory::UpdateHistoryWithCurrentSidData()
{
	int sidNum = 0;
	
	debugInterface->mutexSidDataHistory->Lock();
	
	std::list<CSidData *>::iterator it = debugInterface->sidDataHistory.begin();
	int sy = scrollPosition;
	std::advance(it, sy);
	
	CSidData *pSidDataCurrent  = *it;

	// update with current SID state
	if (debugInterface->sidDataToRestore)
	{
		pSidDataCurrent->CopyFrom(debugInterface->sidDataToRestore);
	}
	else
	{
		pSidDataCurrent->PeekFromSids();
	}

	debugInterface->mutexSidDataHistory->Unlock();
}

void CViewC64SidTrackerHistory::PianoKeyboardNoteReleased(CPianoKeyboard *pianoKeyboard, CPianoKey *pianoKey)
{
	LOGD("CViewSIDTrackerHistory::PianoKeyboardNoteReleased: note=%d", pianoKey->keyNote);
	if (lstMidiIn->selectedElement < 0)
		return;
	
	debugInterface->mutexSidDataHistory->Lock();
	
	std::list<CSidData *>::iterator it = debugInterface->sidDataHistory.begin();
	int sy = scrollPosition;
	std::advance(it, sy);
	
	CSidData *pSidDataCurrent  = *it;
	
	if (pSidDataCurrent == NULL)
	{
		debugInterface->mutexSidDataHistory->Unlock();
		return;
	}
	
//	const sid_frequency_t *sidFrequencyData = SidNoteToFrequency(pianoKey->keyNote);
	
	if (btnJazz->IsOn())
	{
		for (int sidNum = 0; sidNum < debugInterface->numSids; sidNum++)
		{
			for (int chanNum = 0; chanNum < 3; chanNum++)
			{
				// the below does not work:
//				int o = chanNum * 0x07;
//				if (	(pSidDataCurrent->sidData[sidNum][o + 1] = (sidFrequencyData->sidValue & 0xFF00) >> 8)
//					&&	(pSidDataCurrent->sidData[sidNum][o    ] = (sidFrequencyData->sidValue & 0x00FF))
//					&& ((pSidDataCurrent->sidData[sidNum][o + 4] & 0x01) == 0x01))
				
				// check pressed keys instead
				if (pressedKeys[sidNum * 3 + chanNum] == pianoKey)
				{
					LOGD("jazz REL   chanNum=%d note=%d", chanNum, pianoKey->keyNote);

					int o = chanNum * 0x07;

					// found frequency, gate off
					pSidDataCurrent->sidRegs[sidNum][o + 4] = pSidDataCurrent->sidRegs[sidNum][o + 4] & 0xFE;
					
					debugInterface->SetSid(pSidDataCurrent);
					
					pressedKeys[sidNum * 3 + chanNum] = NULL;
					break;
				}
			}
		}
	}
	else
	{
		int sidNum = 0; /// TODO: SELECT SID FIX ME
		int chanNum = lstMidiIn->selectedElement;
		int o = chanNum * 0x07;
		pSidDataCurrent->sidRegs[sidNum][o + 4] = pSidDataCurrent->sidRegs[sidNum][o + 4] & 0xFE;
		debugInterface->SetSid(pSidDataCurrent);
		pressedKeys[sidNum * 3 + chanNum] = NULL;
	}
	
	debugInterface->mutexSidDataHistory->Unlock();
}


bool CViewC64SidTrackerHistory::DoScrollWheel(float deltaX, float deltaY)
{
	LOGD("CViewSIDTrackerHistory::DoScrollWheel: deltaY=%f", deltaY);
	
	MoveTracksY(deltaY);
	return true;
}

bool CViewC64SidTrackerHistory::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
//	LOGD("CViewSIDTrackerHistory::DoMove: diffY=%f", diffY);
	
	if (IsInsideView(x, y))
	{
		MoveTracksY(diffY/4.0f);
	}
	return true;
}

// this is a callback from debug interface:
// when music plays we would like to keep the position, so we need to scroll it automatically up every time new data is fetched
// this is sanitized on render and scroll events
void CViewC64SidTrackerHistory::VSyncStepsAdded()
{
	if (scrollPosition > 0)
	{
		scrollPosition += (float)selectedNumSteps;
	}
}

void CViewC64SidTrackerHistory::EnsureCorrectScrollPosition()
{
	// when we reach end of buffer, then data will be scrolled, but we would like to see all screen filled with data
	if (scrollPosition >= (int)debugInterface->sidDataHistory.size()-numVisibleTrackLines)
		scrollPosition = (int)debugInterface->sidDataHistory.size()-numVisibleTrackLines-1;
	
	if (scrollPosition < 0)
		scrollPosition = 0;
}

void CViewC64SidTrackerHistory::ResetScroll()
{
	SetTracksScrollPos(0);
}

void CViewC64SidTrackerHistory::MoveTracksY(float deltaY)
{
	LOGD("MoveTracksY: %f", deltaY);
	fScrollPosition += deltaY;
	float d = floor(fScrollPosition);
	
	fScrollPosition -= d;
	
	int newPos = scrollPosition + (int)d * selectedNumSteps;
	SetTracksScrollPos(newPos);
}

void CViewC64SidTrackerHistory::SetTracksScrollPos(int newPos)
{
	guiMain->LockMutex();
	debugInterface->mutexSidDataHistory->Lock();
	
	scrollPosition = newPos;
	
	LOGD("new scrollPosition=%d",  scrollPosition);
	EnsureCorrectScrollPosition();
	
	LOGD("size=%d scrollPosition=%d",  debugInterface->sidDataHistory.size(), scrollPosition);
	
	if (isScrub)
	{
		SetSidWithCurrentPositionData();
	}
	
	debugInterface->mutexSidDataHistory->Unlock();
	guiMain->UnlockMutex();
}

void CViewC64SidTrackerHistory::RenderImGui()
{
	PreRenderImGui();

	btnJazz->SetVisible(showChannels);
	lstMidiIn->SetVisible(showChannels);

	
	Render();
	PostRenderImGui();
}

void CViewC64SidTrackerHistory::Render()
{
//	LOGD("CViewSIDTrackerHistory");
	float gap = -2.0f;
		
	// start rendering
	debugInterface->mutexSidDataHistory->Lock();

	EnsureCorrectScrollPosition();

	float py = posY + sizeY - fontSize + gap;
	float pEndY = posY - fontSize + gap;
	int skip = debugInterface->sidDataHistoryCurrentStep;

	
//		   0         1         2         3         4         5         6
//		   01234567890123456789012345678901234567890123456789012345678901234
//		  "C-5 CT PWPW ADSR  C-5 CT PWPW ADSR  C-5 CT PWPW ADSR  FILT FM VL\0";

//        NOTE: scanning is done backwards in history of SID states
//              we need to keep text buffer filled with spaces first because we scan history and display buffer only every selectedNumSteps step
//              this is to allow to not loose a note when it was played in-between current stepNum=0 and selectedNumSteps,
//              so when we scan and there was a note the text buffer is updated and thus it is kept even though we move back next steps, example:
//
//              ... <- display (stepNum == selectedNumSteps), note the note C-0 will be displayed even though it was in-between displayable steps
//              C-0 <- skip
//              ... <- skip
//              ... <- stepNum == 0
//
//        TODO: when channel is muted show it with darker color
//              to solve this we need different buffers for each channel (i.e. numSids * 3 buffers)
//              remember its pos and then display them when all data is collected. we need to keep buffers because of the note above.

	char *buf = SYS_GetCharBuf();
	
	char e = '.';
	bool checkNotesChange = true;	// check notes change, not freq
	
	int stepNum = 0;
	memset(buf, 0x20, 63);
	
	std::list<CSidData *>::iterator it = debugInterface->sidDataHistory.begin();
	
	int sy = scrollPosition;
//	LOGD("size=%d scrollPosition=%d",  debugInterface->sidDataHistory[sidNum].size(), sy);
	std::advance(it, sy);
	
	int currentLine = 0;
	while (it != debugInterface->sidDataHistory.end())
	{
		std::list<CSidData *>::iterator itPrevious = it;
		itPrevious++;
		
		if (itPrevious == debugInterface->sidDataHistory.end())
			break;
		
		if (skip > 0)
		{
			skip--;
			it++;
			continue;
		}
		
		int pos = 0;
		CSidData *pSidDataCurrent  = *it;
		CSidData *pSidDataPrevious = *itPrevious;

		for (int sidNum = 0; sidNum < debugInterface->numSids; sidNum++)
		{
			u8 *sidDataCurrent = pSidDataCurrent->sidRegs[sidNum];
			u8 *sidDataPrevious = pSidDataPrevious->sidRegs[sidNum];

			for (int chanNum = 0; chanNum < 3; chanNum++)
			{
				bool noteDisplayed = false;
				
				u16 chanAddr = chanNum * 0x07;
				
				int notePos = 0;
				u16 freqCurrent  = (sidDataCurrent[chanAddr + 1] << 8)  | sidDataCurrent[chanAddr];
				u16 freqPrevious = (sidDataPrevious[chanAddr + 1] << 8) | sidDataPrevious[chanAddr];
				
				if (btnShowNotes->IsOn())
				{
					notePos = pos;
					if (checkNotesChange == false)
					{
						// mark change on each freq change
						if (freqCurrent == freqPrevious)
						{
							if (stepNum == 0)
							{
								buf[pos] = e; buf[pos+1] = e; buf[pos+2] = e;
							}
						}
						else
						{
							if (buf[pos] == 0x20 || buf[pos] == e)
							{
								const sid_frequency_t *sidFrequencyData = SidFrequencyToNote(freqCurrent);
								
								if (sidFrequencyData->note >= 0 && sidFrequencyData->note < 96)
								{
									buf[pos  ] = sidFrequencyData->name[0];
									buf[pos+1] = sidFrequencyData->name[1];
									buf[pos+2] = sidFrequencyData->name[2];
									noteDisplayed = true;
								}
							}
						}
					}
					else
					{
						// mark change only on note change
						const sid_frequency_t *sidFrequencyDataCurrent  = SidFrequencyToNote(freqCurrent);
						const sid_frequency_t *sidFrequencyDataPrevious = SidFrequencyToNote(freqPrevious);
						
						//				LOGD("py=%-5.0f chan=%d sidDataCurrent=%x sidDataPrevious=%x | freqCurrent=%04x sidFrequencyDataCurrent=%x %s freqPrevious=%04x sidFrequencyDataPrevious=%x %s",
						//					 py, chanNum, sidDataCurrent, sidDataPrevious,
						//					 freqCurrent, sidFrequencyDataCurrent, sidFrequencyDataCurrent->name,
						//					 freqPrevious, sidFrequencyDataPrevious, sidFrequencyDataPrevious->name);
						
						if (sidFrequencyDataCurrent == sidFrequencyDataPrevious)
						{
							if (stepNum == 0)
							{
								buf[pos] = e; buf[pos+1] = e; buf[pos+2] = e;
							}
						}
						else
						{
							if (buf[pos] == 0x20 || buf[pos] == e)
							{
								if (sidFrequencyDataCurrent->note >= 0 && sidFrequencyDataCurrent->note < 96)
								{
									buf[pos  ] = sidFrequencyDataCurrent->name[0];
									buf[pos+1] = sidFrequencyDataCurrent->name[1];
									buf[pos+2] = sidFrequencyDataCurrent->name[2];
									noteDisplayed = true;
								}
							}
						}
					}
					
					pos += 4;
				}
				
				{
					u8 ctrlCurrent  = sidDataCurrent[chanAddr + 4];
					u8 ctrlPrevious = sidDataPrevious[chanAddr + 4];
					
					if (ctrlCurrent == ctrlPrevious)
					{
						if (btnShowInstruments->IsOn())
						{
							if (stepNum == 0)
							{
								buf[pos] = e; buf[pos + 1] = e;
							}
						}
					}
					else
					{
						if (btnShowInstruments->IsOn())
						{
							if (buf[pos] == 0x20 || buf[pos + 1] == e)
							{
								sprintfHexCode8WithoutZeroEnding(buf + pos, ctrlCurrent);
							}
						}
						
						if (btnShowNotes->IsOn())
						{
							if (buf[notePos] == 0x20 || buf[notePos] == e)
							{
								if ((ctrlCurrent & 0x01) == 0x01 && noteDisplayed == false)
								{
									const sid_frequency_t *sidFrequencyData = SidFrequencyToNote(freqCurrent);
									
									if (sidFrequencyData->note >= 0 && sidFrequencyData->note < 96)
									{
										buf[notePos  ] = sidFrequencyData->name[0];
										buf[notePos+1] = sidFrequencyData->name[1];
										buf[notePos+2] = sidFrequencyData->name[2];
										noteDisplayed = true;
									}
								}
							}
							
						}
					}
					
					if (btnShowInstruments->IsOn())
					{
						pos += 3;
					}
				}
				
				if (btnShowAdsr->IsOn())
				{
					u16 adsrCurrent  = (sidDataCurrent[chanAddr + 5] << 8)  | sidDataCurrent[chanAddr + 6];
					u16 adsrPrevious = (sidDataPrevious[chanAddr + 5] << 8) | sidDataPrevious[chanAddr + 6];
					
					if (adsrCurrent == adsrPrevious)
					{
						if (stepNum == 0)
						{
							buf[pos] = e; buf[pos + 1] = e; buf[pos + 2] = e; buf[pos + 3] = e;
						}
					}
					else
					{
						if (buf[pos] == 0x20 || buf[pos + 1] == e)
						{
							sprintfHexCode16WithoutZeroEnding(buf + pos, adsrCurrent);
						}
					}
					pos += 5;
				}

				if (btnShowPWM->IsOn())
				{
					u16 pwCurrent  = (sidDataCurrent[chanAddr + 3] << 8)  | sidDataCurrent[chanAddr + 2];
					u16 pwPrevious = (sidDataPrevious[chanAddr + 3] << 8) | sidDataPrevious[chanAddr + 2];
					
					if (pwCurrent == pwPrevious)
					{
						if (stepNum == 0)
						{
							buf[pos] = e; buf[pos + 1] = e; buf[pos + 2] = e;
						}
					}
					else
					{
						if (buf[pos] == 0x20 || buf[pos] == e)
						{
							sprintfHexCode12WithoutZeroEnding(buf + pos, pwCurrent);
						}
					}
					
					pos += 4;
				}
			}
			
			if (btnShowFilterCutoff->IsOn())
			{
				// filt cutoff
				u16 filtCurrent  = (sidDataCurrent[0x16] << 8)  | sidDataCurrent[0x15];
				u16 filtPrevious = (sidDataPrevious[0x16] << 8) | sidDataPrevious[0x15];
				
				if (filtCurrent == filtPrevious)
				{
					if (stepNum == 0)
					{
						buf[pos] = e; buf[pos + 1] = e; buf[pos + 2] = e; buf[pos + 3] = e;
					}
				}
				else
				{
					if (buf[pos] == 0x20 || buf[pos] == e)
					{
						sprintfHexCode16WithoutZeroEnding(buf + pos, filtCurrent);
					}
				}
				
				pos += 5;
			}
			
			if (btnShowFilterCtrl->IsOn())
			{
				// filt control
				u8 filtCtrlCurrent  = sidDataCurrent[0x17];
				u8 filtCtrlPrevious = sidDataPrevious[0x17];
				
				if (filtCtrlCurrent == filtCtrlPrevious)
				{
					if (stepNum == 0)
					{
						buf[pos] = e; buf[pos+1] = e;
					}
				}
				else
				{
					if (buf[pos] == 0x20 || buf[pos] == e)
					{
						sprintfHexCode8WithoutZeroEnding(buf + pos, filtCtrlCurrent);
					}
				}
				
				pos += 3;
			}
			
			if (btnShowVolume->IsOn())
			{
				// volume
				u8 volCurrent  = sidDataCurrent[0x18];
				u8 volPrevious = sidDataPrevious[0x18];
				
				if (volCurrent == volPrevious)
				{
					if (stepNum == 0)
					{
						buf[pos] = e; buf[pos + 1] = e;
					}
				}
				else
				{
					if (buf[pos] == 0x20 || buf[pos + 1] == e)
					{
						sprintfHexCode8WithoutZeroEnding(buf + pos, volCurrent);
					}
				}
				
				pos += 3;
			}
		}
		
		buf[pos] = 0x00;

		stepNum++;
		if (stepNum == selectedNumSteps)
		{
			currentLine++;
			
//	FIIIXME				printf("strlen=%d\n", strlen(buf));
			buf[64] = 0x00;
			font->BlitText(buf, posX, py, -1, fontSize);
			
			py -= fontSize;
			
			if (py <= (pEndY))
				break;

			stepNum = 0;
			memset(buf, 0x20, 63);
		}
		
		it++;
	}
	
	numVisibleTrackLines = currentLine;
	
	debugInterface->mutexSidDataHistory->Unlock();
	
	SYS_ReleaseCharBuf(buf);
	
	if (showController)
		CGuiView::Render();
}

bool CViewC64SidTrackerHistory::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewC64SidTrackerHistory::ButtonPressed(CGuiButton *button)
{
	LOGD("CViewSIDTrackerHistory::ButtonPressed");
	
	return false;
}

void CViewC64SidTrackerHistory::UpdateButtonsGroup(CGuiButtonSwitch *btn)
{
	// TODO: add group of buttons as GuiElement
	for (std::list<CGuiButtonSwitch *>::iterator it = btnsStepSwitches.begin(); it != btnsStepSwitches.end(); it++)
	{
		CGuiButtonSwitch *btn = *it;
		btn->SetOn(false);
	}
	
	btn->SetOn(true);
}

void CViewC64SidTrackerHistory::SetNumSteps(int numSteps)
{
	this->selectedNumSteps = numSteps;
	debugInterface->SetSidDataHistorySteps(numSteps);
}

bool CViewC64SidTrackerHistory::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGD("CViewSIDTrackerHistory::ButtonSwitchChanged");
	if (button == btnFade)
	{
		viewC64->viewC64SidPianoKeyboard->SetKeysFadeOut(button->IsOn());
	}
	else if (button == btnStep1)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(1);
	}
	else if (button == btnStep2)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(2);
	}
	else if (button == btnStep3)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(3);
	}
	else if (button == btnStep4)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(4);
	}
	else if (button == btnStep5)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(5);
	}
	else if (button == btnStep6)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(6);
	}
	else if (button == btnStep8)
	{
		UpdateButtonsGroup(button);
		SetNumSteps(8);
	}
	
	return false;
}

bool CViewC64SidTrackerHistory::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_ARROW_UP)
	{
		if (isShift)
		{
			MoveTracksY(16);
		}
		else
		{
			MoveTracksY(1);
		}
		return true;
	}
	if (keyCode == MTKEY_ARROW_DOWN)
	{
		if (isShift)
		{
			MoveTracksY(-16);
		}
		else
		{
			MoveTracksY(-1);
		}
		return true;
	}
	if (keyCode == MTKEY_PAGE_UP)
	{
		MoveTracksY(16);
		return true;
	}
	if (keyCode == MTKEY_PAGE_DOWN)
	{
		MoveTracksY(-16);
		return true;
	}
	if (keyCode == MTKEY_SPACEBAR)
	{
		if (debugInterface->GetDebugMode() == DEBUGGER_MODE_RUNNING)
		{
			debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
			return true;
		}
		if (scrollPosition == 0
			&& debugInterface->GetDebugMode() == DEBUGGER_MODE_PAUSED)
		{
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
			return true;
		}
		
		viewC64->viewC64SidTrackerHistory->ResetScroll();
		return true;
	}
	
	if (this->HasFocus())
	{
		return viewC64->viewC64SidPianoKeyboard->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
	}
	return false;
}

bool CViewC64SidTrackerHistory::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_ARROW_UP)
	{
		return true;
	}
	if (keyCode == MTKEY_ARROW_DOWN)
	{
		return true;
	}
	if (keyCode == MTKEY_PAGE_UP)
	{
		return true;
	}
	if (keyCode == MTKEY_PAGE_DOWN)
	{
		return true;
	}
	if (keyCode == MTKEY_SPACEBAR)
	{
		return true;
	}
	
	if (this->HasFocus())
	{
		return viewC64->viewC64SidPianoKeyboard->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
	}
	return false;
}

void CViewC64SidTrackerHistory::RenderFocusBorder()
{
//	// TODO: fix me, this is because allsids view does not conform protocol
//	if (visible && drawFocusBorder && viewC64->viewC64AllSids->IsVisible())
//	{
//		// TODO: make border offsets as parameters of the generic view
//		const float lineWidth = 0.7f;
//		BlitRectangle(this->posX, this->posY, this->posZ, this->sizeX, this->sizeY + 5, 1.0f, 0.0f, 0.0f, 0.5f, lineWidth);
//	}
}

bool CViewC64SidTrackerHistory::HasContextMenuItems()
{
   return true;
}

void CViewC64SidTrackerHistory::RenderContextMenuItems()
{
	if (ImGui::MenuItem("Clear history"))
	{
		debugInterface->mutexSidDataHistory->Lock();
		while(!debugInterfaceVice->sidDataHistory.empty())
		{
			CSidData *sidData = debugInterfaceVice->sidDataHistory.front();
			debugInterfaceVice->sidDataHistory.pop_front();
			delete sidData;
		}
		debugInterface->mutexSidDataHistory->Unlock();
	}
	
	ImGui::Separator();
	if (ImGui::BeginMenu("Save SID history to file"))
	{
		if (ImGui::MenuItem("SidDump format"))
		{
			sidDumpFormat = SID_HISTORY_FORMAT_SIDDUMP;
			CSlrString *defaultFileName = new CSlrString("sid-history");
			
			CSlrString *windowTitle = new CSlrString("Export SID history as SidDump");
			CSlrString *defaultFolder;
			viewC64->config->GetSlrString("ViewC64SidTrackerHistorySidDumpFolder", &defaultFolder, gUTFPathToDocuments);
			viewC64->ShowDialogSaveFile(this, &siddumpFileExtensions, defaultFileName, defaultFolder, windowTitle);
			delete windowTitle;
			delete defaultFileName;
		}
		if (ImGui::MenuItem("CSV format"))
		{
			sidDumpFormat = SID_HISTORY_FORMAT_CSV;
			CSlrString *defaultFileName = new CSlrString("sid-history");
			
			CSlrString *windowTitle = new CSlrString("Export SID history as CSV");
			CSlrString *defaultFolder;
			viewC64->config->GetSlrString("ViewC64SidTrackerHistorySidDumpFolder", &defaultFolder, gUTFPathToDocuments);
			viewC64->ShowDialogSaveFile(this, &csvFileExtensions, defaultFileName, defaultFolder, windowTitle);
			delete windowTitle;
			delete defaultFileName;
		}
		ImGui::EndMenu();
	}

	if (ImGui::Checkbox("Dump SID time in minutes:seconds:frame format", &timeseconds))
	{
		viewC64->config->SetBool("ViewC64SidTrackerHistorySidDumpTimeSeconds", &timeseconds);
	}
	
	if (ImGui::InputInt("Note spacing, default 0 (none)", &spacing))
	{
		viewC64->config->SetInt ("ViewC64SidTrackerHistorySidDumpSpacing", &spacing);
	}
	
	if (ImGui::InputInt("Pattern spacing, default 0 (none)", &pattspacing))
	{
		viewC64->config->SetInt ("ViewC64SidTrackerHistorySidDumpPattSpacing", &pattspacing);
	}
	
	if (ImGui::InputInt("\"Oldnote-sticky\" factor. Default 1, increase for better vibrato display", &oldnotefactor))
	{
		viewC64->config->SetInt ("ViewC64SidTrackerHistorySidDumpOldNoteFactor", &oldnotefactor);
	}
	
	if (ImGui::Checkbox("Low-resolution mode (only display 1 row per note)", &lowres))
	{
		viewC64->config->SetBool("ViewC64SidTrackerHistorySidDumpLowResolution", &lowres);
	}
	
	if (ImGui::InputScalar("Frequency recalibration (in hex)", ImGuiDataType_U16, &basefreq, NULL, NULL, "%04X"))
	{
		viewC64->config->SetInt ("ViewC64SidTrackerHistorySidDumpBaseFreq", &basefreq);
	}
	
	if (ImGui::InputScalar("Calibration note (abs.notation 80-DF). Default middle-C (B0)", ImGuiDataType_U8, &basenote, NULL, NULL, "%02X"))
	{
		viewC64->config->SetInt ("ViewC64SidTrackerHistorySidDumpBaseNote", &basenote);
	}
	
	ImGui::Separator();
}

void CViewC64SidTrackerHistory::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGD("CViewC64SidTrackerHistory: save");
//	path->DebugPrint();
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	debugInterface->mutexSidDataHistory->Lock();

	C64SIDHistoryToByteBuffer(&debugInterfaceVice->sidDataHistory, byteBuffer, sidDumpFormat,
						   basefreq, basenote, spacing ? 1:0, oldnotefactor, pattspacing, timeseconds ? 1:0, lowres);

	debugInterface->mutexSidDataHistory->Unlock();
	
	byteBuffer->storeToFile(path);
	delete byteBuffer;
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	viewC64->ShowMessageInfo(str);
	delete str;
	
	path->DebugPrint();
	str = path->GetFilePathWithoutFileNameComponentFromPath();
	viewC64->config->SetSlrString("ViewC64SidTrackerHistorySidDumpFolder", &str);
	delete str;
}

void CViewC64SidTrackerHistory::SystemDialogFileSaveCancelled()
{
	
}

