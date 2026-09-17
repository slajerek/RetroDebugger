#include <metal_stdlib>
using namespace metal;

struct MTVertexIn
{
	float2 position [[attribute(0)]];
	float2 uv       [[attribute(1)]];
	uchar4 color    [[attribute(2)]];
};

struct MTVertexOut
{
	float4 position [[position]];
	float2 uv;
	float4 color;
};

struct MTUniforms
{
	float4x4 projectionMatrix;
};

struct MTCrtUniforms
{
	float2 resolution;
	float  time;
	float  padding;
};

vertex MTVertexOut mtVertexMain(MTVertexIn in [[stage_in]],
								constant MTUniforms &uniforms [[buffer(1)]])
{
	MTVertexOut out;
	out.position = uniforms.projectionMatrix * float4(in.position, 0, 1);
	out.uv = in.uv;
	out.color = float4(in.color) / float4(255.0);
	return out;
}

fragment float4 mtFragmentMain(MTVertexOut in [[stage_in]],
							   constant MTCrtUniforms &crt [[buffer(0)]],
							   texture2d<float, access::sample> screenTexture [[texture(0)]])
{
	constexpr sampler linearSampler(coord::normalized, address::clamp_to_edge,
									min_filter::linear, mag_filter::linear);

	float2 uv = in.uv;
	float4 color = screenTexture.sample(linearSampler, uv);

	float scanline = sin(uv.y * crt.resolution.y * 3.1415 * 3.5) * 0.3;

	color.r = color.r - scanline;
	color.g = color.g - scanline;
	color.b = color.b - scanline;

	return in.color * color;
}
