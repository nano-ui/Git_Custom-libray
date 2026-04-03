#include "gltf_static_mesh.hlsli"

// gltf_static_mesh 頂点シェーダー
VS_OUT main(
    float3 position : POSITION,
    float3 normal   : NORMAL,
    float2 texcoord : TEXCOORD)
{
    VS_OUT vout;

    float4 wpos     = mul(float4(position, 1.0f), world);
    vout.position   = mul(wpos, view_projection);
    vout.world_position = wpos;

    float4 wnormal  = float4(normal, 0.0f);
    vout.world_normal = normalize(mul(wnormal, world));

    vout.texcoord = texcoord;

    return vout;
}
