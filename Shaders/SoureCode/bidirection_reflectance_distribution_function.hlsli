#ifndef __BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION_HLSL__
#define __BIDIRECTIONAL_REFLECTANCE_DISTRIBUTION_FUNCTION_HLSL__ 

float3 f_schlick(float3 f0,float3 f90,float VoH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0f - VoH, 0.0f, 1.0f), 5.0f);
}

float v_ggx(float NoL, float NoV, float alpha_roughness)
{
    float alpha_roughness_sq = alpha_roughness * alpha_roughness;
    float ggxv = NoL * sqrt(NoV * NoV * (1.0f - alpha_roughness) + alpha_roughness_sq);
    float ggxl = NoL * sqrt(NoL * NoL * (1.0f - alpha_roughness) + alpha_roughness_sq);
    float ggx = ggxv + ggxl;
    return (ggx > 0.0f) ? 0.5f / ggx : 0.0f;
}

#endif