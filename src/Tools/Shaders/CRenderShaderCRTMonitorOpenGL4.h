#ifndef _CRenderShaderCRTMonitorOpenGL3_h_
#define _CRenderShaderCRTMonitorOpenGL3_h_

#include "CRenderShaderOpenGL4.h"

class CRenderShaderCRTMonitorOpenGL4 : public CRenderShaderOpenGL4
{
public:
	CRenderShaderCRTMonitorOpenGL4(CRenderBackendOpenGL4 *renderBackend, const char *shaderName, float screenWidth, float screenHeight);
	virtual ~CRenderShaderCRTMonitorOpenGL4();
	
	// to be overriden by shader
	virtual const char *GetVertexShaderSource();
	virtual const char *GetFragmentShaderSource();
	virtual void GetUniformsLocations();

	virtual void SetShaderVars();
	
	GLint attribLocationResolution;
	GLint attribLocationTime;
	
	u64 startTime;
};

#endif
