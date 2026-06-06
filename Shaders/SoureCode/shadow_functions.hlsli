

//シャドウマップのテクスチャ座標を計算
float3 CalculateShadowTexcoord(float4 world_pos, row_major float4x4 light_view_projection)
{
    //ライトの視点に基づいたNDC座標への変換
    float4 wvp_pos = mul(world_pos, light_view_projection);
    wvp_pos /= wvp_pos.w;
    
    //テクスチャUV座標系へのマッピング公式の適用
    wvp_pos.y = -wvp_pos.y;
    static const float scale_half = 0.5f;
    wvp_pos.xy = scale_half * wvp_pos.xy + scale_half;
    
    return wvp_pos.xyz;
}

//影の判定と色の乗算値を計算
float3 CalculateShadow(float3 shadow_texcoord, Texture2D shadow_map, SamplerState shadow_sampler, float shadow_bias, float3 shadow_color)
{    
    //シャドウマップからの深度値サンプリング
    float depth = shadow_map.Sample(shadow_sampler, shadow_texcoord.xy).r;

    //深度比較による影判定とシャドウアクネ軽減
    if (shadow_texcoord.z - shadow_bias > depth)
    {
        return shadow_color;
    }
    return float3(1.0f, 1.0f, 1.0f);
}