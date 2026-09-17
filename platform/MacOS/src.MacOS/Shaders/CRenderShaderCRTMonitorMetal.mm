#include "CRenderShaderCRTMonitorMetal.h"
#include "Generated/C64DCRTMonitorMetallib.h"
#include "CRenderBackendMetal.h"
#include "SYS_Main.h"
#include "DBG_Log.h"

#import <Metal/Metal.h>

CRenderShaderCRTMonitorMetal::CRenderShaderCRTMonitorMetal(CRenderBackendMetal *renderBackend,
														   const char *shaderName,
														   float screenWidth, float screenHeight)
: CRenderShaderMetal(renderBackend, shaderName)
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	this->startTime = SYS_GetCurrentTimeInMillis();
}

// The GLSL this ports, line for line:
//
//   uv       = Frag_UV
//   color    = texture(iChannel0, uv)
//   scanline = sin(uv.y * iResolution.y * PI * 3.5) * 0.3
//   color.rgb -= scanline
//   fragColor = Frag_Color * color
//
// Note the scanline uses the interpolated UV, not gl_FragCoord, so it is tied
// to the emulator screen's own vertical extent rather than to physical pixels
// -- which means it needs NO dpi scaling here, unlike the masked-tile port. The
// alpha channel is deliberately left untouched by the subtraction, exactly as
// in the GLSL where only .r/.g/.b are modified.
const char *CRenderShaderCRTMonitorMetal::GetMetalShaderSource()
{
	return kC64DCRTMonitorMetalSource;
}

const void *CRenderShaderCRTMonitorMetal::GetEmbeddedLibraryData(unsigned long *outLength)
{
	if (outLength != NULL)
		*outLength = kC64DCRTMonitorMetallibLength;
	return kC64DCRTMonitorMetallibData;
}

void CRenderShaderCRTMonitorMetal::SetShaderVars(void *encoder)
{
	id<MTLRenderCommandEncoder> enc = (__bridge id<MTLRenderCommandEncoder>)encoder;

	u64 t = SYS_GetCurrentTimeInMillis() - startTime;
	float uniforms[4] = { screenWidth, screenHeight, (float)t / 1000.0f, 0.0f };
	[enc setFragmentBytes:uniforms length:sizeof(uniforms) atIndex:0];
}
