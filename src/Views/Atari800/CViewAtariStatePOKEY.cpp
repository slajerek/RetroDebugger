#include "EmulatorsConfig.h"
#if defined(RUN_ATARI)

extern "C" {
#include "pokey.h"
#include "pokeysnd.h"
}
#endif

#include "CViewAtariStatePOKEY.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterfaceAtari.h"
#include "CViewC64StateSID.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CViewWaveform.h"
#include "CLayoutParameter.h"

// KOLORY BY KK do PIANO
// RED
// GREEN
// BLUE
// YELLOW

/// KK powiedział:
//1. Liczysz numer nuty na podstawie częstotliwości.
//2. Rysujesz pixel w określonym miejscu.
//3. Scrollujesz bitmapę 1px w górę.
//I tak co ramkę.

//Zamiast tysiąca słów - paint.
//Czerwony - dźwięk ze slidem (końcówka tez powinna być czerwona, my bad).
//Zielony - dźwięk z vibrato.
//Niebieski - arp.
//Całośc przesuwa się w górę i zostawia taki ślad, np na 1s lub coś koło tego. I widać wszystko jak na dłoni. 🙂

// ok, ten 16MHz to był 1.77MHz 🙂

//bo niektóre nuty czasami na Atari grają inne waveformy z uwagi na fakt, że jest sample & hold zrobiony zamiast generatorów poly4/poly5
//POKEY ma dwa generatory poly (4- i 5- bitowy) zasilane z zegara 16MHz (czy ile on miał)
//A kanały robią S&H tych wartości.
//Wszystko działa w miarę spoko, dopóki częstotliwości są względnie pierwsze.

//ale waveform na nute mi nie wplynie raczej
//jasne że wpływa
//4 i 5 bitowy

//Masz opcje:
//- pure tone (:2)
//- 4-bit poly (:15)
//- 5-bit poly (:31)
//- 17-bit poly (szum)

// https://www.atarimagazines.com/compute/issue34/112_1_16-BIT_ATARI_MUSIC.php
// http://krap.pl/mirrorz/atari/homepage.ntlworld.com/kryten_droid/Atari/800XL/atari_hw/pokey.htm


/*
 TODO: POKEY NOTES VIEW
 Zoltraks — 09/10/2022
 Częstotliwości można wyliczyć, ale zależą one od trybu PAL/NTSC, zegara 64kHz/15kHz oraz ewentualnego połączenia kanałów 1+3/2+4 oraz 1+2/3+4 (rejestr AUDCTL). Nie słyszałem o gotowej procedurze, ale... RMT w wersji VinsCool wyświetla częstotliwości (chociaż chyba też nie do końca dobrze)
 Myślę, że może tam coś będzie, kod RMT to w dużej mierze C++ i także open source https://github.com/VinsCool/RASTER-Music-Tracker a poniżej zrzut ekranu (po prawej stronie są wyświetlane częstotliwości i wartości rejestrów)
 GitHub
 GitHub - VinsCool/RASTER-Music-Tracker
 Contribute to VinsCool/RASTER-Music-Tracker development by creating an account on GitHub.
 GitHub - VinsCool/RASTER-Music-Tracker
 Image
 ZWiSU — 09/10/2022
 Ping @mono
 mono — 09/10/2022
 mozna to policzyc generalnie dla fali prostokatnej http://atariki.krap.pl/index.php/Rejestry_POKEY-a#AUDF1 (konfiguracja pokeya dokonywana jest generalnie przez AUDCTL a kanalu przez AUDCx - rodzaj dzwieku i AUDFx - czestotliwosc), ewentualnie dla fali z wlaczona synchronizacja (SKCTL tzw. transmisja dwutonowa), bo wtedy zmienia sie wypelnienie; gorzej bedzie z szumem, bo tam mozna chyba tylko podac czestotliwosc maksymalna
 transmisja dwutonowa dziala tak, ze jak AUDF2 doliczy do 0 to resetowany jest kanal 1 - czyli wyjscie audio ustawiane jest w stan wysoki i przeladowywany AUDF1 - jesli AUDF2 jest mniejsze niz 2*AUDF1 to w konsekwencji dostaje sie fale o okresie w AUDF2 (nie wg powyzszego wzoru), ale o wypelnieniu wynikajacym z ilorazu czasu trwania stanu wysokiego (AUDF1) do okresu fali (AUDF2)
 te rejestry tylko sie nazywaja "frequency" a w rzeczywistosci to sa period 🙂 bo to zwykly licznik odliczajacy tikniecia zegara bazowego - czyli dzielnik czestotliwosci
 mono — 09/10/2022
 pamietaj ze sa dwie kwestie
 czestotliwosc nuty granej kanalem kiedy grasz dzwiek
 i druga rzecz to okres miedzy przerwaniami kiedy generujesz licznikiem irq
 trzecia rzecz to taktowanie transmisji SIO czyli predkosc transmisji w bitach na sekunde
 Image
 mono — 09/10/2022
 mozna, choc wzor jest w http://atariki.krap.pl/index.php/POKEY#Szybko.C5.9B.C4.87_transmisji
 oni tam pisza o parze dzielnikow 3+4 bo tak OS ustawia tryb komunikacji, ale naprawde konfiguracje dla transmisji ustawia sie w SKCTL
 podobienstwo wzoru (Q/2)/(N+M) do tego z generowaniem dzwieku polega na tym, ze bit jest probkowany 2x
 uwaga na kwarc systemowy, bo wystepuja 4: dwa dla NTSC i jeden dla PAL http://atariki.krap.pl/index.php/NTSC_vs_PAL oraz jeden dla SECAM http://atariki.krap.pl/index.php/130XE_SECAM

 */




#if defined(RUN_ATARI)

CViewAtariStatePOKEY::CViewAtariStatePOKEY(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	fontBytes = viewC64->fontDisassembly;
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	showRegistersOnly = false;
	editHex = new CGuiEditHex(this);
	editHex->isCapitalLetters = false;
	editingRegisterValueIndex = -1;

	for (int pokeyNum = 0; pokeyNum < MAX_NUM_POKEYS; pokeyNum++)
	{
		for (int chanNum = 0; chanNum < 4; chanNum++)
		{
			viewChannelWaveform[pokeyNum][chanNum] = new CViewWaveform("CViewAtariStatePOKEY::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->pokeyChannelWaveform[pokeyNum][chanNum]);
		}
		viewMixWaveform[pokeyNum] = new CViewWaveform("CViewAtariStatePOKEY::CViewWaveform", 0, 0, 0, 0, 0, debugInterface->pokeyMixWaveform[pokeyNum]);
	}
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewAtariStatePOKEY::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	UpdateFontSize();
}

void CViewAtariStatePOKEY::UpdateFontSize()
{
	// TODO: fix the size, it is to default now. we need to calculate size like in CViewNesStateAPU
	// UX workaround for now...
//	if (viewC64->currentScreenLayoutId != SCREEN_LAYOUT_ATARI_MEMORY_MAP)
	{
		// POKEY
		
		float dx = 1.0f;
		
		float wgx = 4.0f;
		float wgy = fontSize*5.714f;
		float wsx = fontSize*14.2f;
		float wsy = fontSize*5.5f;

		float px = posX + wgx;
		float py = posY + wgy;
		
		for (int pokeyNum = 0; pokeyNum < MAX_NUM_POKEYS; pokeyNum++)
		{
			px = posX + wgx;
			for (int chanNum = 0; chanNum < 4; chanNum++)
			{
				viewChannelWaveform[pokeyNum][chanNum]->SetPosition(px, py, posZ, wsx, wsy);
				px += wsx + dx;
			}
			
			py += wgy + wsy;
		}
		
		px += fontSize*1;
		
		py = posY + wgy;
		for (int pokeyNum = 0; pokeyNum < MAX_NUM_POKEYS; pokeyNum++)
		{
			viewMixWaveform[pokeyNum]->SetPosition(px, py, posZ, wsx, wsy);
			
			py += wgy + wsy;
		}
		
	}
	/*
	else
	{
		//
		//
		// memory map

		
		float px = posX;
		float py = posY + fontSize*7.0f;
		
		float wsx = fontSize*11.5f;
		float wsy = fontSize*3.7f;

		float dx = 1.0f;
		
		for (int chanNum = 0; chanNum < 4; chanNum++)
		{
			for (int pokeyNum = 0; pokeyNum < MAX_NUM_POKEYS; pokeyNum++)
			{
				pokeyChannelWaveform[pokeyNum][chanNum]->SetPosition(px, py, posZ, wsx, wsy);
			}
			px += wsx + dx;
			if (chanNum == 1)
			{
				px = posX;
				py += wsy;
			}
		}
		
		py += wsy + 1.0f;
		px = posX;
		
		for (int pokeyNum = 0; pokeyNum < MAX_NUM_POKEYS; pokeyNum++)
		{
			pokeyMixWaveform[pokeyNum]->SetPosition(px, py, posZ, wsx*2 + dx, wsy);
		}

	}
	 */
}

void CViewAtariStatePOKEY::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	UpdateFontSize();
}

void CViewAtariStatePOKEY::SetVisible(bool isVisible)
{
	CGuiElement::SetVisible(isVisible);
}

void CViewAtariStatePOKEY::DoLogic()
{
}

void CViewAtariStatePOKEY::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewAtariStatePOKEY::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;

	int numPokeys = debugInterfaceAtari->IsPokeyStereo() ? 2 : 1;

	// calc trigger pos, render
	for (int pokeyNumber = 0; pokeyNumber < numPokeys; pokeyNumber++)
	{
		for (int chanNum = 0; chanNum < 4; chanNum++)
		{
			viewChannelWaveform[pokeyNumber][chanNum]->Render();
		}
		
		viewMixWaveform[pokeyNumber]->Render();
	}

	this->RenderState(posX, posY, posZ, fontBytes, fontSize, 1);
}


void CViewAtariStatePOKEY::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int pokeyNum)
{
	float startX = px + fontSize*0.5;
	float startY = py + fontSize*0.5f;
	
	char buf[256] = {0};
	
	px = startX;
	py = startY;
	
#ifdef STEREO_SOUND
	if (debugInterfaceAtari->IsPokeyStereo())
	{
		sprintf(buf, "POKEY 1");
	}
	else
	{
		sprintf(buf, "POKEY");
	}
#endif
	
	fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	py += fontSize*0.5f;

	/*
	 
	 if (showRegistersOnly)
	 {
		float fs2 = fontSize;
		
		float plx = px;
		float ply = py;
		for (int i = 0; i < 0x10; i++)
		{
	 if (editingCIAIndex == ciaId && editingRegisterValueIndex == i)
	 {
	 sprintf(buf, "D%c%02x", addr, i);
	 fontBytes->BlitText(buf, plx, ply, posZ, fs2);
	 fontBytes->BlitTextColor(editHex->textWithCursor, plx + fontSize*5.0f, ply, posZ, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);
	 }
	 else
	 {
	 u8 v = c64d_ciacore_peek(cia_context, i);
	 sprintf(buf, "D%c%02x %02x", addr, i, v);
	 fontBytes->BlitText(buf, plx, ply, posZ, fs2);
	 }
	 
	 ply += fs2;
	 
	 if (i % 0x08 == 0x07)
	 {
	 ply = py;
	 plx += fs2 * 9;
	 }
		}
		
		return;
	 }*/
	
	const char *space = "     ";

	// workaround for now
//	if (viewC64->currentScreenLayoutId != SCREEN_LAYOUT_ATARI_MEMORY_MAP)
	{
		sprintf(buf, "AUDF1  %02X%sAUDF2  %02X%sAUDF3  %02X%sAUDF4  %02X%sAUDCTL %02X%sKBCODE %02X",
				POKEY_AUDF[POKEY_CHAN1], space, POKEY_AUDF[POKEY_CHAN2], space, POKEY_AUDF[POKEY_CHAN3], space, POKEY_AUDF[POKEY_CHAN4], space,
				POKEY_AUDCTL[0], space, POKEY_KBCODE);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "AUDC1  %02X%sAUDC2  %02X%sAUDC3  %02X%sAUDC4  %02X%sIRQEN  %02X",
				POKEY_AUDC[POKEY_CHAN1], space, POKEY_AUDC[POKEY_CHAN2], space, POKEY_AUDC[POKEY_CHAN3], space, POKEY_AUDC[POKEY_CHAN4], space,
				POKEY_IRQEN);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "SKSTAT %02X%sSKCTL  %02X%sRANDOM %02X%sALLPOT %02X%sIRQST  %02X",
				POKEY_SKSTAT, space, POKEY_SKCTL, space, POKEY_GetByte(POKEY_OFFSET_RANDOM, TRUE), space, POKEY_GetByte(POKEY_OFFSET_ALLPOT, TRUE), space, POKEY_IRQST);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
	}
	/*
	else
	{
		// memory map
		char *space = "  ";
		
		sprintf(buf, "AUDF1  %02X%sAUDC1  %02X",
				POKEY_AUDF[POKEY_CHAN1], space, POKEY_AUDC[POKEY_CHAN1]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, "AUDF2  %02X%sAUDC2  %02X",
				POKEY_AUDF[POKEY_CHAN2], space, POKEY_AUDC[POKEY_CHAN2]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, "AUDF3  %02X%sAUDC3  %02X",
				POKEY_AUDF[POKEY_CHAN3], space, POKEY_AUDC[POKEY_CHAN3]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, "AUDF4  %02X%sAUDC4  %02X",
				POKEY_AUDF[POKEY_CHAN4], space, POKEY_AUDC[POKEY_CHAN4]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		sprintf(buf, "AUDCTL %02X", POKEY_AUDCTL[0]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;

	}*/
	

#ifdef STEREO_SOUND
	if (debugInterfaceAtari->IsPokeyStereo())
	{
		px = startX;
		py += fontSize*7.5f;
		sprintf(buf, "POKEY 2");
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		py += fontSize*0.5f;

		sprintf(buf, "AUDF1  %02X%sAUDF2  %02X%sAUDF3  %02X%sAUDF4  %02X%sAUDCTL %02X",
				POKEY_AUDF[POKEY_CHAN1 + POKEY_CHIP2], space, POKEY_AUDF[POKEY_CHAN2 + POKEY_CHIP2], space,
				POKEY_AUDF[POKEY_CHAN3 + POKEY_CHIP2], space, POKEY_AUDF[POKEY_CHAN4 + POKEY_CHIP2], space, POKEY_AUDCTL[1]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
		sprintf(buf, "AUDC1  %02X%sAUDC2  %02X%sAUDC3  %02X%sAUDC4  %02X",
				POKEY_AUDC[POKEY_CHAN1 + POKEY_CHIP2], space, POKEY_AUDC[POKEY_CHAN2 + POKEY_CHIP2], space,
				POKEY_AUDC[POKEY_CHAN3 + POKEY_CHIP2], space, POKEY_AUDC[POKEY_CHAN4 + POKEY_CHIP2]);
		fontBytes->BlitText(buf, px, py, posZ, fontSize); py += fontSize;
		
	}
#endif

	//
	py += fontSize;

	
}

bool CViewAtariStatePOKEY::DoTap(float x, float y)
{
	guiMain->LockMutex();
	
	if (editingRegisterValueIndex != -1)
	{
		editHex->FinalizeEntering(MTKEY_ENTER, true);
	}

	/*
	// check if tap register
	if (showRegistersOnly)
	{
		float px = posX;
		
		if (x >= posX && x < posX+190)
		{
			float fs2 = fontSize;
			float sx = fs2 * 9;
			
			float plx = posX;	//+ fontSize * 5.0f
			float plex = posX + fontSize * 7.0f;
			float ply = posY + fontSize;
			for (int i = 0; i < 0x10; i++)
			{
				if (x >= plx && x <= plex
					&& y >= ply && y <= ply+fontSize)
				{
					LOGD("CViewAtariStatePOKEY::DoTap: tapped register %02x", i);
					
					editingRegisterValueIndex = i;

					u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
					editHex->SetValue(v, 2);
					
					guiMain->UnlockMutex();
					return true;
				}
				
				ply += fs2;
				
				if (i % 0x08 == 0x07)
				{
					ply = posY + fontSize;
					plx += sx;
					plex += sx;
				}
			}
		}
	}
	*/
	showRegistersOnly = !showRegistersOnly;
	
	guiMain->UnlockMutex();
	return false;
}

void CViewAtariStatePOKEY::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	if (isCancelled)
		return;
	
	/*
	if (editingRegisterValueIndex != -1)
	{
		byte v = editHex->value;
		debugInterface->SetCiaRegister(editingCIAIndex, editingRegisterValueIndex, v);
		
		editHex->SetCursorPos(0);
	}
	 */

}

bool CViewAtariStatePOKEY::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	/*
	if (editingRegisterValueIndex != -1)
	{
		if (keyCode == MTKEY_ARROW_UP)
		{
			if (editingRegisterValueIndex > 0)
			{
				editingRegisterValueIndex--;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_DOWN)
		{
			if (editingRegisterValueIndex < 0x0F)
			{
				editingRegisterValueIndex++;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (editHex->cursorPos == 0 && editingRegisterValueIndex > 0x08)
			{
				editingRegisterValueIndex -= 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (editHex->cursorPos == 1 && editingRegisterValueIndex < 0x10-0x08)
			{
				editingRegisterValueIndex += 0x08;
				u8 v = debugInterface->GetCiaRegister(editingCIAIndex, editingRegisterValueIndex);
				editHex->SetValue(v, 2);
				return true;
			}
		}
		
		editHex->KeyDown(keyCode);
		return true;
	}
	 */
	return false;
}

bool CViewAtariStatePOKEY::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

void CViewAtariStatePOKEY::RenderFocusBorder()
{
//	CGuiView::RenderFocusBorder();
	//
}

#else
// dummy

CViewAtariStatePOKEY::CViewAtariStatePOKEY(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
}

void CViewAtariStatePOKEY::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY) {}
void CViewAtariStatePOKEY::DoLogic() {}
void CViewAtariStatePOKEY::Render() {}
void CViewAtariStatePOKEY::RenderImGui() {}
void CViewAtariStatePOKEY::RenderState(float px, float py, float posZ, CSlrFont *fontBytes, float fontSize, int pokeyNum) {}
bool CViewAtariStatePOKEY::DoTap(float x, float y) { return false; }
void CViewAtariStatePOKEY::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled) {}
bool CViewAtariStatePOKEY::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
bool CViewAtariStatePOKEY::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper) { return false; }
void CViewAtariStatePOKEY::RenderFocusBorder() {}
void CViewAtariStatePOKEY::SetVisible(bool isVisible) {}
void CViewAtariStatePOKEY::AddWaveformData(int pokeyNumber, int v1, int v2, int v3, int v4, short mix) {}

#endif

