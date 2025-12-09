#include "fullscreen_quad.hlsli"

#define POINT 0//最近傍フィルタ（カクカク）
#define LINEAR 1//線形補間（なめらか）
#define ANISOTROPIC 2//異方性フィルタ（高品質）

SamplerState sampler_states[3] : register(s0);
Texture2D texture_maps[4] : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = texture_maps[0].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
    float alpha = color.a;
    
    //明るさによるマスク処理
    color.rgb = step(brightness_threshold, dot(color.rgb, float3(0.299, 0.587, 0.114))) * color.rgb;
    return float4(color.rgb, alpha);
}