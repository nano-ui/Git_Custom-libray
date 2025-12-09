
//各頂点のスクリーン上の位置と色を出力
struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float4 world_position : POSITOIN;
    float4 world_normal : NORMAL;
};

//オブジェクトのワールド変換行列とマテリアル色にアクセス
cbuffer OBJECT_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;
    float4 material_color;
};

//カメラ設定(ビュー射影行列)やシーンのライトの方向等にアクセス
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 light_direction;
    float4 camera_position;
};