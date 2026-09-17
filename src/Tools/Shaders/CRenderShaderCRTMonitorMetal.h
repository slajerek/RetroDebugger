#ifndef _CRenderShaderCRTMonitorMetal_h_
#define _CRenderShaderCRTMonitorMetal_h_

#include "CRenderShaderMetal.h"
#include "SYS_Types.h"

// Metal port of CRenderShaderCRTMonitorOpenGL4.
//
// The GLSL's live effect is a single scanline modulation; the aperture-grille
// and chroma-distortion blocks around it are commented out in the original and
// are NOT ported, because MSL nobody runs is MSL nobody maintains. If they are
// ever revived, both backends have to change together -- that is the point of
// saying so here rather than silently porting dead code.
class CRenderShaderCRTMonitorMetal : public CRenderShaderMetal
{
public:
	CRenderShaderCRTMonitorMetal(CRenderBackendMetal *renderBackend, const char *shaderName,
								 float screenWidth, float screenHeight);

	virtual const char *GetMetalShaderSource() override;
	virtual const void *GetEmbeddedLibraryData(unsigned long *outLength) override;
	virtual void SetShaderVars(void *encoder) override;

	u64 startTime;
};

#endif
