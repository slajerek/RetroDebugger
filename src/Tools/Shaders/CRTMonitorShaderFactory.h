#ifndef _CRTMonitorShaderFactory_h_
#define _CRTMonitorShaderFactory_h_

class CRenderShader;

// Creates the CRT monitor shader for whichever render backend is active, or
// returns NULL when that backend has no port of it.
//
// A FACTORY rather than a direct construction, because the shader classes are
// per-backend and one of them is Objective-C++: CViewEmulatorScreen is plain
// C++ compiled on three platforms and must not name a Metal type. The macOS
// build compiles CRTMonitorShaderFactoryMacOS.mm, which knows about both
// backends; Windows and Linux compile CRTMonitorShaderFactory.cpp, which knows
// only about OpenGL because that is the only backend those platforms have.
//
// NULL is a normal outcome and callers must handle it by drawing the emulator
// screen unshaded. c64d without a CRT filter is fine; c64d with a blank screen
// is not; c64d exiting at launch -- which is what an unguarded
// GetRenderBackendOpenGL4() did under Metal -- is worst of all.
CRenderShader *CreateCRTMonitorShader(const char *shaderName, float screenWidth, float screenHeight);

// Same, for the engine's ShaderToy demo shader. Unused by default (the
// construction in CViewEmulatorScreen is commented out) but kept reachable on
// both backends so it is not a one-backend trap for whoever enables it next.
CRenderShader *CreateShaderToyShader(const char *shaderName, float screenWidth, float screenHeight);

#endif
