#include "static_mesh.hlsli"


VS_OUT main(
float4 position : POSITION,
float4 normal : NORMAL,
float2 texcoord:TEXCOORD)
{
    VS_OUT vont;
    
    //最終的なスクリーン上の位置に変換
    vont.position = mul(position, mul(world, view_projection));
    
    vont.world_position = mul(position, mul(world, view_projection));
    
    //平行移動の影響を受けないようにする
    normal.w = 0;
    vont.world_normal = normalize(mul(normal, world));
    
    vont.color = material_color;
    
    vont.texcoord = texcoord;
    
    return vont;
}