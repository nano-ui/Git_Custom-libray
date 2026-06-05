#include "sky_box.hlsli"

//テクスチャとサンプラーの定義
TextureCube texture0 : register(t0);	//背景描画用のキューブマップテクスチャ
SamplerState sampler0 : register(s0);	//サンプラーステート

float4 main(VS_OUT pin) : SV_TARGET
{
	//視線ベクトルの取得と環境光の計算
    float3 E = normalize(pin.local_position);
    float3 ambient = ambient_color.rgb * ambient_color.a;
	
	//テクスチャのサンプリングと最終カラーの合成
    float4 sampled_color = texture0.SampleLevel(sampler0, E, 0);
	
    return float4(ambient, 1.0f) * sampled_color;
}