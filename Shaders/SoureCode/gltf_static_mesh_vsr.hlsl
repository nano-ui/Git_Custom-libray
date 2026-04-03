#include "gltf_static_mesh.hlsli"

VS_OUT main(
    float4 position : POSITION,
    float4 normal : NORMAL,
    float4 tangent: TANGENT,
    float2 texcoord : TEXCOORD)
{
    VS_OUT vout;
    
    // 最終的なスクリーン座標位置に変換
    vout.position = mul(position, mul(world, view_projection));
    
    // ワールド座標での位置
    vout.world_position = mul(position, world);
    
    // 行列変換の影響を受けないようにする
    normal.w = 0;
    
    // ワールド座標での法線
    vout.world_normal = normalize(mul(normal, world));
    
    // タンジェント
    float sigma = tangent.w;
    tangent.w = 0;
    vout.world_tangent = float4(normalize(mul(tangent, world)).xyz, sigma);
    
    // マテリアルカラーを設定
    vout.color = material_color;
    
    // テクスチャ座標をそのまま出力
    vout.texcoord = texcoord;
    
    return vout;
}