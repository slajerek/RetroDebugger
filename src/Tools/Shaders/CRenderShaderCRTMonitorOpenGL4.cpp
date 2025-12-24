#include "CRenderShaderCRTMonitorOpenGL4.h"
#include "VID_Main.h"

CRenderShaderCRTMonitorOpenGL4::CRenderShaderCRTMonitorOpenGL4(CRenderBackendOpenGL4 *renderBackend, const char *shaderName, float screenWidth, float screenHeight)
: CRenderShaderOpenGL4(renderBackend, shaderName)
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;
	
	startTime = SYS_GetCurrentTimeInMillis();
}

const char *CRenderShaderCRTMonitorOpenGL4::GetVertexShaderSource()
{
	return R"(
 
	layout (location = 0) in vec2 Position;
	layout (location = 1) in vec2 UV;
	layout (location = 2) in vec4 Color;
	uniform mat4 ProjMtx;
	out vec2 Frag_UV;
	out vec4 Frag_Color;
	void main()
	{
		Frag_UV = UV;
		Frag_Color = Color;
		gl_Position = ProjMtx * vec4(Position.xy,0,1);
	}

	)";
}

const char *CRenderShaderCRTMonitorOpenGL4::GetFragmentShaderSource()
{
	return R"(
 
	in vec2 Frag_UV;
	in vec4 Frag_Color;

	uniform sampler2D iChannel0;
	uniform vec2 iResolution;
	uniform float iTime;
 
	layout (location = 0) out vec4 Out_Color;

	void mainImage(out vec4 fragColor, in vec2 fragCoord);

	void main() {
		mainImage(Out_Color, gl_FragCoord.xy);
	}
 
	void mainImage(out vec4 fragColor, in vec2 fragCoord)
	{
		vec2 uv = Frag_UV;

		vec4 color = texture(iChannel0, uv.st);

		float scanline = sin(uv.y * iResolution.y * 3.1415 * 3.5) * 0.3;

		color.r = color.r - scanline;
		color.g = color.g - scanline;
		color.b = color.b - scanline;

		/*
		float grille = step(0.66, mod(uv.x * iResolution.x, 1.2));
		color *= vec4(1.0 - grille * 0.3, 1.0 - grille * 0.1, 1.0, 1.0);

		float chromaDistort = 0.0015;
		vec4 chromaColor;
		chromaColor.r = texture(iChannel0, uv + vec2(chromaDistort, 0.0)).r;
		chromaColor.g = texture(iChannel0, uv).g;
		chromaColor.b = texture(iChannel0, uv - vec2(chromaDistort, 0.0)).b;
		chromaColor.a = 1.0;

		color = mix(color, chromaColor, 0.5);
		*/
 
//		color.r *= iTime;

		fragColor = Frag_Color * color;
	}

	)";
}

void CRenderShaderCRTMonitorOpenGL4::GetUniformsLocations()
{
	LOGD("CRenderShaderCRTMonitorOpenGL4::GetUniformsLocations");
	
	attribLocationResolution = GetUniformLocation("iResolution");
	attribLocationTime = GetUniformLocation("iTime");
}

void CRenderShaderCRTMonitorOpenGL4::SetShaderVars()
{
//	DebugPrintUniforms();

	if (attribLocationResolution != -1)
	{
		glUniform2f(attribLocationResolution, screenWidth, screenHeight);
		ASSERT_OPENGL();
	}
		
//	LOGD("screen=%f %f", screenWidth, screenHeight);
		
	if (attribLocationTime != -1)
	{
		u64 t = SYS_GetCurrentTimeInMillis() - startTime;
		float elapsedSeconds = (float)t / 1000.0f;
		
		glUniform1f(attribLocationTime, elapsedSeconds);
		ASSERT_OPENGL();
	}
}

CRenderShaderCRTMonitorOpenGL4::~CRenderShaderCRTMonitorOpenGL4()
{
}
	
