#include "shape_renderer.hlsli"

VS_OUT main(float3 position : POSITION)
{
	VS_OUT vout;
    vout.position = mul(float4(position, 1.0f), worldViewProjection);
	vout.color = color;

	return vout;
}
