     #ifndef _CC64TOOLS_H_
#define _CC64TOOLS_H_

#include "SYS_Defs.h"
#include "ViceWrapper.h"

#define CBMSHIFTEDFONT_INVERT	0x80

class CSlrFontProportional;
class CSlrString;
class CImageData;
class CDebugInterface;
class CDebugInterfaceC64;
class CDataAdapter;

CSlrFontProportional *ProcessFonts(uint8 *charsetData, bool useScreenCodes);
void InvertCBMText(CSlrString *text);
void ClearInvertCBMText(CSlrString *text);

void InvertCBMText(char *text);
void ClearInvertCBMText(char *text);

void ConvertCharacterDataToImage(u8 *characterData, CImageData *imageData);
void ConvertColorCharacterDataToImage(u8 *characterData, CImageData *imageData, u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800, CDebugInterfaceC64 *debugInterface);

void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, int gap);
void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 colorD021, u8 colorD027,
							  CDebugInterfaceC64 *debugInterface, int gap);
void ConvertSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 bkgColorR, u8 bkgColorG, u8 bkgColorB, u8 spriteColorR, u8 spriteColorG, u8 spriteColorB, int gap);

void ConvertColorSpriteDataToImage(u8 *spriteData, CImageData *imageData, u8 colorD021, u8 colorD025, u8 colorD026, u8 colorD027, CDebugInterfaceC64 *debugInterface, int gap, u8 alpha);

void GetCBMColor(u8 colorNum, float *r, float *g, float *b);

u8 ConvertPetsciiToScreenCode(u8 chr);

void CopyHiresCharsetToImage(u8 *charsetData, CImageData *imageData, int numColumns,
							 u8 colorBackground, u8 colorForeground, CDebugInterfaceC64 *debugInterface);

void CopyMultiCharsetToImage(u8 *charsetData, CImageData *imageData, int numColumns,
							 u8 colorD021, u8 colorD022, u8 colorD023, u8 colorD800,
							 CDebugInterfaceC64 *debugInterface);

// returns color number from palette that is nearest to rgb
u8 FindC64Color(u8 r, u8 g, u8 b, CDebugInterfaceC64 *debugInterface);
float GetC64ColorDistance(u8 color1, u8 color2, CDebugInterfaceC64 *debugInterface);

//
void RenderColorRectangle(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color,
						  CDebugInterfaceC64 *debugInterface);
void RenderColorRectangleWithHexCode(float px, float py, float ledSizeX, float ledSizeY, float gap, bool isLocked, u8 color, float fontSize,
						  CDebugInterfaceC64 *debugInterface);

//
uint16 GetSidAddressByChipNum(int chipNum);

// convert SID file to PRG, returns buffer with PRG
CByteBuffer *ConvertSIDtoPRG(CByteBuffer *sidFileData);
bool C64LoadSIDToRam(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr);
bool C64LoadSIDToBuffer(const char *filePath, u16 *fromAddr, u16 *toAddr, u16 *initAddr, u16 *playAddr, u8 **buffer);

//
bool C64SaveMemory(int fromAddr, int toAddr, bool isPRG, CDataAdapter *dataAdapter, const char *filePath);
int C64LoadMemory(int fromAddr, CDataAdapter *dataAdapter, const char *filePath);
bool C64SaveMemoryExomizerPRG(int fromAddr, int toAddr, int jmpAddr, const char *filePath);
u8 *C64ExomizeMemoryRaw(int fromAddr, int toAddr, int *compressedSize);

void GetC64VicAddrFromState(vicii_cycle_state_t *viciiState, int *screenAddr, int *charsetAddr, int *bitmapBank);

int ConvertSdlAxisToJoystickAxis(int sdlAxis);

#endif
