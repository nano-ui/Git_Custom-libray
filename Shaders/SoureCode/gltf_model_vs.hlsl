#include "gltf_model.hlsli"

float4x4 FetchBoneMatrix(uint joint_id, uint row_y)
{
    uint base_x = joint_id * 4;
    float4 r0 = bone_texture.Load(int3(base_x + 0, row_y, 0));
    float4 r1 = bone_texture.Load(int3(base_x + 1, row_y, 0));
    float4 r2 = bone_texture.Load(int3(base_x + 2, row_y, 0));
    float4 r3 = bone_texture.Load(int3(base_x + 3, row_y, 0));
    return float4x4(r0, r1, r2, r3);
}

VS_OUT main(VS_IN vin)
{
    float sigma = vin.tangent.w;

    if (skin > -1 && dot(vin.weights, float4(1.0f, 1.0f, 1.0f, 1.0f)) > 0.001f)
    {
        uint row_y = (uint) (vin.instance_anim.x + 0.5f);

        float4x4 b0 = FetchBoneMatrix(vin.joints.x, row_y);
        float4x4 b1 = FetchBoneMatrix(vin.joints.y, row_y);
        float4x4 b2 = FetchBoneMatrix(vin.joints.z, row_y);
        float4x4 b3 = FetchBoneMatrix(vin.joints.w, row_y);

        float4x4 skin_matrix = b0 * vin.weights.x +
                               b1 * vin.weights.y +
                               b2 * vin.weights.z +
                               b3 * vin.weights.w;

        vin.position = mul(float4(vin.position.xyz, 1.0f), skin_matrix);
        vin.normal = normalize(mul(float4(vin.normal.xyz, 0.0f), skin_matrix));
        vin.tangent = normalize(mul(float4(vin.tangent.xyz, 0.0f), skin_matrix));
    }
    
    VS_OUT vout;

    vin.position.w = 1.0f;
    vout.w_position = mul(vin.position, vin.instance_world);
    vout.position = mul(vout.w_position, view_projection);

    vin.normal.w = 0.0f;
    vout.w_normal = normalize(mul(vin.normal, vin.instance_world));

    vin.tangent.w = 0.0f;
    vout.w_tangent = normalize(mul(vin.tangent, vin.instance_world));
    vout.w_tangent.w = sigma;

    vout.texcoord = vin.texcoord;

    float4 light_space_position = mul(vout.w_position, light_view_projection);
    vout.shadow_texcoord = light_space_position.xyz / light_space_position.w;

    return vout;
}