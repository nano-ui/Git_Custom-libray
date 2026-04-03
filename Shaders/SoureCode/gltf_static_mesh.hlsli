//ピクセルシェーダーに渡すデータ
struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float4 world_position : TEXCOORD1;
    float4 world_normal : NORMAL;
    float4 world_tangent : TANGENT;
};

//マテリアル定数バッファ
cbuffer MATERIAL_CONSTANT_BUFFER : register(b0)
{
    float4 color;               // ベースカラー (RGBA)
    float3 emissive;            // 放射色
    float metallic;             // メタリック値 (0.0-1.0)
    float roughness;            // ラフネス値 (0.0-1.0)
    float normal_scale;         // 法線スケール
    float occlusion_strength;   // オクルージョン強度
    int padding[2];             // パディング
};

//オブジェクト定数バッファ
cbuffer OBJECT_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 world;
    float4 material_color;
};

//シーン定数バッファ
cbuffer SCENE_CONSTANT_BUFFER : register(b2)
{
    row_major float4x4 view_projection;
    float4 light_direction;
    float4 camera_position;
};