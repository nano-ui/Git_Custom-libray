#include "sprite.hlsli"

Texture2D color_map : register(t0);
SamplerState point_sumpler_state : register(s0);
SamplerState linear_sumpler_state : register(s1);
SamplerState anisotropic_sumpler_state : register(s2);

float4 main(VS_OUT pin) : SV_TARGET
{ 
    float4 color = color_map.Sample(anisotropic_sumpler_state, pin.texcoord);
    float alpha = color.a;
    
#if 1
    //Inverse gamma process
    const float GAMMA = 2.2f;
    color.rgb = pow(color.rgb, GAMMA);
        
#endif
    return float4(color.rgb, alpha) * pin.color;
    
    //return color_map.Sample(
    //point_sumpler_state,
    //pin.texcoord) * pin.color;
    //const float2 center = float2(1280 / 2, 720 / 2);
    //float distance = length(center - pin.position.xy);
    //if (distance > 200)
    //    return 1;
    //else
    //    return float4(1, 0, 0, 1);
    
    
  } 