#ifndef _VICEDITORUNDODATA_H_
#define _VICEDITORUNDODATA_H_

#include "SYS_Defs.h"

class CViewVicEditor;
class CByteBuffer;

class CVicEditorUndoData
{
public:
	CVicEditorUndoData(CViewVicEditor *vicEditor);
	~CVicEditorUndoData();
	
	CViewVicEditor *vicEditor;
	
	virtual void StoreUndoData();
	virtual void RestoreUndoData();

	CByteBuffer *undoDataBuffer;
};

#endif
