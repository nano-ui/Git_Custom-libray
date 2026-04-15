
//--------------------------------------------------------------------------------------
//定数バッファ・テクスチャ定義
//--------------------------------------------------------------------------------------

//オブジェクト用定数バッファ
cbuffer ObjectConstantBuffer : register(b0)
{
    row_major float4x4 world_matrix;          //ワールド変換行列
    float4 material_color;          //マテリアルカラー
    row_major float4x4 bone_transformes[256]; //各ボーンの変形行列配列
}

//シーン定数バッファ
cbuffer SceneConstantBuffer : register(b1)
{
    row_major float4x4 view_projection;        //ビュー・プロジェクション行列
    float4 light_direction;          //ライトの方向
    float3 camera_position;          //カメラの位置
}

//--------------------------------------------------------------------------------
//構造体定義
//--------------------------------------------------------------------------------

//頂点シェーダー入力構造体
struct VS_IN
{
    float3 position : POSITION;     // 頂点座標
    float3 normal : NORMAL;         // 法線ベクトル
    float4 tangent : TANGENT;       // 接線ベクトル
    float2 texcoord : TEXCOORD;     // テクスチャ座標
    float4 bone_weights : WEIGHTS;  // スキニング重み
    uint4 bone_indices : JOINTS;    // ボーン影響番号
};

//頂点シェーダー出力構造体
struct VS_OUT
{
    float4 position : SV_POSITION;  // システム用座標
    float3 world_pos : POSITION;    // ワールド空間座標
    float3 normal : NORMAL;         // ワールド空間法線
    float3 tangent : TANGENT;       // ワールド空間接線
    float2 texcoord : TEXCOORD;     // テクスチャ座標
};