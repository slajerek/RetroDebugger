#ifndef _C64DebuggerPluginCrtMaker_H_
#define _C64DebuggerPluginCrtMaker_H_

#include "CDebuggerEmulatorPluginVice.h"
#include "CDebuggerApi.h"
#include <list>

extern char *crtMakerConfigFilePath;

class CImageData;
class CViewC64CrtMaker;

enum CCrtMakerFileType : u8
{
	TypeBIN = 0,
	TypeEXO,
	TypePRG,
	TypeSID,
	TypeASM
};

class CCrtMakerFile
{
public:
	// destinationAddr = -1 means that it will be set based on detected file type
	CCrtMakerFile(const char *filePath, const char *displayName);
	CCrtMakerFile(const char *filePath, const char *displayName, int destinationAddr);
	~CCrtMakerFile();

	void Init(const char *filePath, const char *displayName, int destinationAddr);
	void UpdateDestinationAddrFromPrg();
	
	char *filePath;
	char *displayName;
	char *fileName;
	char *prgFilePath;
	char *exoFilePath;
	
	CCrtMakerFileType type;

	u8 *data;
	int size;

	int destinationAddr;

	int sourceCartImageAddr;

	int sourceBankNum;
	int sourceBankAddr;

	u16 fileId;
};

class C64DebuggerPluginCrtMaker : public CDebuggerEmulatorPluginVice, CSlrThread
{
public:
	C64DebuggerPluginCrtMaker();
	
	virtual void Init();
	virtual void ThreadRun(void *data);

	virtual void DoFrame();
	virtual u32 KeyDown(u32 keyCode);
	virtual u32 KeyUp(u32 keyCode);

	CViewC64CrtMaker *view;
	
	int tuneLoadAddr;
	
	CImageData *imageDataRef;
	
	// assemble
	u16 addrAssemble;
	void Assemble(char *buf);
	void PutDataByte(u8 v);
};

extern C64DebuggerPluginCrtMaker *pluginCrtMaker;

void PLUGIN_CrtMakerInit();
void PLUGIN_CrtMakerSetVisible(bool isVisible);


#endif
