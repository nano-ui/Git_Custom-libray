#include "gltf_static_mesh.hlsli"

// gltf_static_mesh ピクセルシェーダー
// テクスチャスロット
Texture2D color_texture             : register(t0); // ベースカラー
Texture2D normal_texture            : register(t1); // ノーマルマップ
Texture2D metallic_roughness_texture: register(t2); // メタリック/ラフネス (B=metallic, G=roughness)
Texture2D occlusion_texture         : register(t3); // アンビエントオクルージョン
Texture2D emissive_texture          : register(t4); // エミッシブ

SamplerState linear_sampler         : register(s0);

// Gram-Schmidt法で届折話引数を湺劃羮する辝定ベクトルを構築する
// ノーマル N と任意の参照方向 ref から T を生成する
float3 BuildTangent(float3 N)
{
    // abs(N.z) < abs(N.x) の場合は Y軸を先死山に使用、それ以外は X軸を使用
    float3 ref = (abs(N.z) < abs(N.x)) ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(ref - N * dot(ref, N));
    return T;
}

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラー
    float4 base_color = color_texture.Sample(linear_sampler, pin.texcoord) * color_factor;
    float  alpha      = base_color.a;

    // ノーマルマッピング (Gram-Schmidt TBN)
    float3 N = normalize(pin.world_normal.xyz);
    float3 T = BuildTangent(N);
    float3 B = normalize(cross(N, T));

    float4 normal_sample = normal_texture.Sample(linear_sampler, pin.texcoord);
    float3 normal_ts     = (normal_sample.xyz * 2.0f - 1.0f) * float3(normal_scale, normal_scale, 1.0f);
    N = normalize(normal_ts.x * T + normal_ts.y * B + normal_ts.z * N);

    // メタリック/ラフネス
    float4 mr_sample  = metallic_roughness_texture.Sample(linear_sampler, pin.texcoord);
    float  metallic   = mr_sample.b * metallic_factor;
    float  roughness  = mr_sample.g * roughness_factor;

    // アンビエントオクルージョン
    float4 ao_sample  = occlusion_texture.Sample(linear_sampler, pin.texcoord);
    float  ao         = 1.0f + occlusion_strength * (ao_sample.r - 1.0f);

    // ライティング (Lambert + Blinn-Phong)
    float3 L          = normalize(-light_direction.xyz);
    float3 V          = normalize(camera_position.xyz - pin.world_position.xyz);
    float3 H          = normalize(V + L);

    float  NdotL      = max(0.0f, dot(N, L));
    float  NdotH      = max(0.0f, dot(N, H));

    float  shininess  = (1.0f - roughness) * 128.0f;
    float3 diffuse    = base_color.rgb * (1.0f - metallic) * NdotL;
    float3 specular   = base_color.rgb * metallic * pow(NdotH, max(shininess, 1.0f));

    // エミッシブ
    float4 emissive_sample = emissive_texture.Sample(linear_sampler, pin.texcoord);
    float3 emissive        = emissive_sample.rgb * emissive_factor;

    float3 color = (diffuse + specular) * ao + emissive;

    return float4(color, alpha);
}
