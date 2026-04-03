#include "gltf_static_mesh.hlsli"

//マテリアル用のテクスチャとサンプラー
Texture2D color_texture : register(t0); // ベースカラー
Texture2D normal_texture : register(t1); // 法線

SamplerState point_sampler_state : register(s0);
SamplerState linear_sampler_state : register(s1);
SamplerState anisotropic_sampler_state : register(s2);

float4 main(VS_OUT pin) : SV_TARGET
{
	
	//ベースカラーをテクスチャから取得
    float4 base_color = color_texture.Sample(anisotropic_sampler_state, pin.texcoord);
    base_color *= color;
    float alpha = base_color.a;
	
	//TBN行列の作成
    float3 N = normalize(pin.world_normal.xyz);
    float3 T = normalize(pin.world_tangent.xyz);
    float sigma = pin.world_tangent.w;
    float3 B = normalize(cross(N, T)) * sigma;
	
	//法線をテクスチャから取得
    float4 normal_sample = normal_texture.Sample(linear_sampler_state, pin.texcoord);
	
	//法線テクスチャから法線ベクトルへ変換
    float3 tangent_normal = (normal_sample.xyz * 2.0f) - 1.0f;
	
	//法線スケールを適用
    tangent_normal.xy *= normal_scale;
	
	//接空間での法線をワールド空間の法線に変換
    N = normalize(tangent_normal.x * T + tangent_normal.y * B + tangent_normal.z * N);
	
	//ライティング計算
    float3 L = normalize(-light_direction.xyz);
    float3 diffuse = base_color.rgb * max(0, dot(N, L));
	
	//スペキュラーハイライト
    float3 V = normalize(camera_position.xyz - pin.world_position.xyz);
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 128) * 0.5f;
	
	//最終カラー
    float3 final_color = diffuse + specular;
	
    return float4(final_color, alpha) * pin.color;
}