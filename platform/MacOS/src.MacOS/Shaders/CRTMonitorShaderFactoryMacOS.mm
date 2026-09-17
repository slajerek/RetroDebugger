// macOS build of the CRT shader factory: OpenGL and Metal.
//
// Compiled INSTEAD of CRTMonitorShaderFactory.cpp, which Windows and Linux use.
// The two files must define the same symbols; a shader added to one and not the
// other becomes a silent NULL on the platforms building the other.
#include "CRTMonitorShaderFactory.h"
#include "CRenderBackend.h"
#include "CRenderBackendOpenGL4.h"
#include "CRenderShaderCRTMonitorOpenGL4.h"
#include "CRenderShaderOpenGL4ShaderToy.h"
#include "CRenderBackendMetal.h"
#include "CRenderShaderCRTMonitorMetal.h"
#include "CRenderShaderShaderToyMetal.h"
#include "VID_Main.h"
#include "DBG_Log.h"

// Distinguishing the backends by SupportsOpenGLShaders() rather than by a name
// string: it is the capability the GL shader classes actually need, and it is
// the same predicate every other call site in this port uses.
CRenderShader *CreateCRTMonitorShader(const char *shaderName, float screenWidth, float screenHeight)
{
	CRenderBackend *backend = VID_GetRenderBackend();
	if (backend == NULL)
		return NULL;

	if (backend->SupportsOpenGLShaders())
	{
		return new CRenderShaderCRTMonitorOpenGL4(CRenderBackendOpenGL4::GetRenderBackendOpenGL4(),
												  shaderName, screenWidth, screenHeight);
	}

	return new CRenderShaderCRTMonitorMetal((CRenderBackendMetal *)backend,
											shaderName, screenWidth, screenHeight);
}

CRenderShader *CreateShaderToyShader(const char *shaderName, float screenWidth, float screenHeight)
{
	CRenderBackend *backend = VID_GetRenderBackend();
	if (backend == NULL)
		return NULL;

	if (backend->SupportsOpenGLShaders())
	{
		return new CRenderShaderOpenGL4ShaderToy(CRenderBackendOpenGL4::GetRenderBackendOpenGL4(),
												 shaderName, screenWidth, screenHeight);
	}

	return new CRenderShaderShaderToyMetal((CRenderBackendMetal *)backend,
										   shaderName, screenWidth, screenHeight);
}
