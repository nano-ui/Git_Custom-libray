#include "gltf_dynamic_model.hlsli"

//----------------------------------
//リソース定義
//----------------------------------

Texture2D base_map : register(t0);          // 基本色テクスチャ
Texture2D normal_map : register(t1);        // 法線マップ
SamplerState common_sampler : register(s0);  // サンプラーステート

float4 main(VS_OUT input) : SV_TARGET
{
    //----------------------------------
    //色のサンプリング
    //----------------------------------
    
    float4 tex_color = base_map.Sample(common_sampler,
    input.texcoord); // テクスチャから色を取得 [cite: 2025-11-26]
    float4 out_color = tex_color * material_color; // マテリアルの色を掛け合わせる

    //----------------------------------
    //簡易的なライティング
    //----------------------------------
    
    float3 L = normalize(-light_direction.xyz);     // ライトの方向を逆転・正規化
    float3 N = normalize(input.normal);             // 法線ベクトルを再度正規化
    float diffuse = saturate(dot(N, L));            // 内積で光の当たり具合を計算 [cite: 2025-11-26]
    diffuse = diffuse * 0.5f + 0.5f;                // 陰を明るく補正して見た目を滑らかにする
    out_color.rgb *= diffuse;                       // 明るさを反映

    return out_color; // ピクセルの最終色を出力
}