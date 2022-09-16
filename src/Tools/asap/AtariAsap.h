#ifndef _AtariAsap_h
#define _AtariAsap_h

class CByteBuffer;

// Converts ASAP formats to XEX, fileName extension is used only to determine format
CByteBuffer *ConvertASAPtoXEX(const char *fileName, CByteBuffer *asapFileData, bool *isStereo);

#endif

