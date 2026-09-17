// OpenGL-only build of the CRT shader factory, for Windows and Linux.
//
// macOS compiles CRTMonitorShaderFactoryMacOS.mm INSTEAD of this file -- it has
// the same two symbols plus the Metal branch. Keep the two in step: a shader
// added to one and not the other becomes a silent NULL on the platforms that
// build the other file.
#include "CRTMonitorShaderFactory.h"
#include "CRenderBackend.h"
#include "CRenderBackendOpenGL4.h"
#include "CRenderShaderCRTMonitorOpenGL4.h"
#include "CRenderShaderOpenGL4ShaderToy.h"
#include "VID_Main.h"
#include "DBG_Log.h"

CRenderShader *CreateCRTMonitorShader(const char *shaderName, float screenWidth, float screenHeight)
{
	CRenderBackend *backend = VID_GetRenderBackend();
	if (backend == NULL || !backend->SupportsOpenGLShaders())
	{
		LOGError("CreateCRTMonitorShader: no OpenGL backend; the emulator screen will draw unshaded");
		return NULL;
	}
	return new CRenderShaderCRTMonitorOpenGL4(CRenderBackendOpenGL4::GetRenderBackendOpenGL4(),
											  shaderName, screenWidth, screenHeight);
}

CRenderShader *CreateShaderToyShader(const char *shaderName, float screenWidth, float screenHeight)
{
	CRenderBackend *backend = VID_GetRenderBackend();
	if (backend == NULL || !backend->SupportsOpenGLShaders())
		return NULL;
	return new CRenderShaderOpenGL4ShaderToy(CRenderBackendOpenGL4::GetRenderBackendOpenGL4(),
											 shaderName, screenWidth, screenHeight);
}
