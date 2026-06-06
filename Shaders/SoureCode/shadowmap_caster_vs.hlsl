
//モデル描画時に受け取るワールド行列の定数バッファ
cbuffer SCENE_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;
}

//シャドウマップ専用の定数バッファ
cbuffer SHADOWMAP_CONSTANT_BUFFER : register(b6)
{
    row_major float4x4 light_view_projection;
    float3 shadow_color;
    float shadow_bias;
}

//シャドウマップへの深度焼き付け専用頂点シェーダー
float4 main(float4 position : POSITION) : SV_POSITION
{
    //頂点座標のライト視点変換
    float4 world_pos = mul(position, world);
    float4 result_pos = mul(world_pos, light_view_projection);
    
    return result_pos;
}