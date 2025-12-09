
//頂点シェーダー出力構造体
struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};


cbuffer ThresholdBuffer : register(b0)
{
    float brightness_threshold; //明るさの閾値
    float3 padding; //16バイト境界合わせ
}

cbuffer BloomParams : register(b1)
{
    float gaussian_sigma;
    float bloom_intensity;
    float2 paddings;
};