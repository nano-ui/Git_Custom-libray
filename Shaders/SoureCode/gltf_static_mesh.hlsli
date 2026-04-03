
// gltf_static_mesh.hlsli
// Shared header for gltf_static_mesh vertex and pixel shaders

struct VS_OUT
{
    float4 position      : SV_POSITION;
    float4 world_position : POSITION;
    float4 world_normal  : NORMAL;
    float2 texcoord      : TEXCOORD;
};

// b0: オブジェクト定数バッファ (ワールド変換行列)
cbuffer OBJECT_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;
};

// b1: シーン定数バッファ (カメラ / ライト)
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 light_direction;
    float4 camera_position;
};

// b2: マテリアル定数バッファ (PBRパラメータ)
cbuffer MATERIAL_CONSTANT_BUFFER : register(b2)
{
    float4 color_factor;        // ベースカラー係数 (RGBA)
    float3 emissive_factor;     // エミッシブ係数 (RGB)
    float  metallic_factor;     // メタリック係数
    float  roughness_factor;    // ラフネス係数
    float  normal_scale;        // ノーマルマップスケール
    float  occlusion_strength;  // オクルージョン強度
    float  _padding;
};
