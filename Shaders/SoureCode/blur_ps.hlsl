 #include "fullscreen_quad.hlsli" 

#define POINT 0//最近傍フィルタ（カクカク）
#define LINEAR 1//線形補間（なめらか）
#define ANISOTROPIC 2//異方性フィルタ（高品質）

SamplerState sampler_states[3] : register(s0);
Texture2D texture_maps[4] : register(t0);


float4 main(VS_OUT pin) : SV_TARGET
{
    //テクスチャサイズの取得
    uint mip_level = 0, width, height, number_of_levels;
    texture_maps[1].GetDimensions(mip_level, width, height, number_of_levels);
    
    //元のカラーを取得
    float4 color = texture_maps[0].Sample(sampler_states[LINEAR], pin.texcoord);
    float alpha = color.a;
    
    //ガウスブラーの準備
    float3 blur_color = 0;
    float gaussian_kernel_total = 0;
    
    const int gaussian_half_kernel_size = 3;
    
    //ガウスカーネルを使って周囲を平均化
    [unroll]
    for (int x = -gaussian_half_kernel_size; x <= +gaussian_half_kernel_size; x += 1)
    {
        [unroll]
        for (int y = -gaussian_half_kernel_size; y <= +gaussian_half_kernel_size; y += 1)
        {
            float gaussian_kernel = exp(-(x * x + y * y) / (2.0f * gaussian_sigma * gaussian_sigma)) /
                (2 * 3.14159265358979 * gaussian_sigma * gaussian_sigma);
            blur_color += texture_maps[1].Sample(sampler_states[LINEAR], pin.texcoord +
                float2(x * 1.0f / width, y * 1.0f / height)).rgb * gaussian_kernel;
            gaussian_kernel_total += gaussian_kernel;
        }
    }
    
    //正規化して最終ブラー色を計算
    blur_color /= gaussian_kernel_total;
    
    //元画像にブラーを加算（ブルーム合成）
    color.rgb += blur_color * bloom_intensity;
     
#if 1
    // Tone mapping : HDR -> SDR 
    const float exposure = 1.2f;
    color.rgb = 1 - exp(-color.rgb * exposure);
#endif
    
#if 1
    //Gamma process
    const float GAMMA = 2.2f;
    color.rgb = pow(color.rgb, 1.0 / GAMMA);
#endif
    
    //出力
    return float4(color.rgb, alpha);
}