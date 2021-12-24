#ifndef _C64COLODORE_H_
#define _C64COLODORE_H_

#include "SYS_Defs.h"

class CImageData;
class CDebugInterfaceC64;

class C64ColodoreScreen
{
public:
	C64ColodoreScreen(CDebugInterfaceC64 *debugInterface);
	~C64ColodoreScreen();

	void RefreshColodoreScreen(CImageData *imageC64Screen);
	void InitColodoreScreen();
	
	CImageData *imageDataColodoreScreen;
	
	CDebugInterfaceC64 *debugInterface;

	int canvasSizeWidth;
	int canvasSizeHeight;
	
	// parameters
	float brightness;
	float contrast;
	float saturation;
	
	bool scanLines;
	bool hanoverBars;
	bool delayLine;
	bool chromaSubsampling;
	
	bool earlyLuma;
	bool delay1084;
	
private:
	float gammasrc;
	float gammatgt;
	float phase;
	
	float odd;
	float scanshade;
	float sub;
	
	float contrastBoost;
	bool invert;
	
	float **resultrgb;
	float **resultyuv;
	float **resultyuv_hanbar_even;
	float **resultyuv_hanbar_odd;
	
	float lumas[16];
	float angles[16];
	
	CImageData *imgData_luma;
	CImageData *imgData_luma_inverted;
	CImageData *imgData_luma_inverted_scaled;
	
	CImageData *imgData_chroma_u;
	CImageData *imgData_chroma_u2;
	CImageData *imgData_chroma_u_scaled;

	CImageData *imgData_chroma_v;
	CImageData *imgData_chroma_v_scaled;

	CImageData *imgData_rgb;

	float gamma_pepto(float value);
	float yuv2r( float y, float v );
	float yuv2g( float y, float u, float v );
	float yuv2b( float y, float u );
	float luma( float input );
	float angle( float input, float phs );
	void setupPalette();

};

#endif
