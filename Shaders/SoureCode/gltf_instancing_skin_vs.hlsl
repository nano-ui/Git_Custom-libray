#include "gltf_model.hlsli"

StructuredBuffer<float4x4> bone_matrices_buffer : register(t10); //全インスタンスのボーン行列バッファ

//頂点シェーダーへの入力構造体
struct VS_INPUT
{
    //頂点ごとの基本データ
    float3 position : POSITION;     //頂点座標
    float3 normal : NORMAL;         //法線ベクトル
    float4 tangent : TANGENT;       //接線ベクトル
    float2 texcoord : TEXCOORD;     //UV座標
    uint4 joints : JOINTS;          //影響を受けるボーンのインデックス番号
    float4 weights : WEIGHTS;       //ボーンの影響度合い（ウェイト）
    
    //インスタンスごとのデータ
    row_major float4x4 instance_world : INSTANCE_TRANSFORM;     //インスタンス固有のワールド行列
    uint instance_bone_offset : BONE_OFFSET;                    //構造化バッファ内のこのインスタンスのボーン開始位置
    uint instance_id : SV_InstanceID;                           //インスタンスの識別番号（システム自動割り当て）
};


VS_OUT main(VS_INPUT input)
{
    VS_OUT output = (VS_OUT) 0;
    output.texcoord = input.texcoord;
    float sigma = input.tangent.w;
    
    //最終的な頂点座標と法線を計算するための一時変数
    float4 blended_position = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float3 blended_normal = float3(0.0f, 0.0f, 0.0f);
    float3 blended_tangent = float3(0.0f, 0.0f, 0.0f);
    
    // スキニングの計算
    if (skin > -1)
    {
        // 頂点が影響を受ける最大4つのボーンに対してループ計算
        for (int i = 0; i < 4; i++)
        {
            // ウェイトが0以下の場合は計算をスキップ
            if (input.weights[i] <= 0.0f) continue;

            uint matrix_index = input.instance_bone_offset + input.joints[i];
            float4x4 bone_matrix = bone_matrices_buffer[matrix_index];

            blended_position += mul(float4(input.position, 1.0f), bone_matrix) * input.weights[i];
            blended_normal += mul(input.normal, (float3x3) bone_matrix) * input.weights[i];
            blended_tangent += mul(input.tangent.xyz, (float3x3) bone_matrix) * input.weights[i];
        }
    }
    else
    {
        // スキニングを持たないモデルの場合は元の座標をそのまま使用
        blended_position = float4(input.position.xyz, 1.0f);
        blended_normal = input.normal.xyz;
        blended_tangent = input.tangent.xyz;
    }

    //ワールド座標と画面投影座標の計算
    float4 world_position = mul(blended_position, input.instance_world);
    output.position = mul(output.w_position, view_projection);
    
    //法線と接線の回転と正規化
    output.w_normal = float4(normalize(mul(blended_normal, (float3x3) input.instance_world)), 0.0f); 
    output.w_tangent = float4(normalize(mul(blended_tangent, (float3x3) input.instance_world)), sigma);
    
    //シャドウマップ用の座標計算
    float4 light_space_position = mul(output.w_position, light_view_projection); 
    output.shadow_texcoord = light_space_position.xyz / light_space_position.w;
    
    return output;
}