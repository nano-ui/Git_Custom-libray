
struct VS_IN
{
    float4 position : POSITION; //頂点座標
    float4 normal : NORMAL;     //頂点の法線ベクトル
    float4 tangent : TANGENT;   //頂点の接線ベクトル
    float2 texcoord : TEXCOORD; //テクスチャのUV座標
    uint4 joints : JOINTS;      //スキニングに使用するジョイントのインデックス
    float4 weights : WEIGHTS;   //スキニングに使用するジョイントの重み
};

struct VS_OUT
{
    float4 position : SV_POSITION;      //射影空間の座標
    float4 w_position : POSITION;       //ワールド空間での頂点座標
    float4 w_normal : NORMAL;           //ワールド空間での法線ベクトル
    float4 w_tangent : TANGENT;         //ワールド空間での接線ベクトル
    float2 texcoord : TEXCOORD;         //ピクセルシェーダーへ渡すUV座標
    float3 shadow_texcoord : TEXCOORD1; //シャドウマップ用のパラメータ
};

cbuffer PRIMITIVE_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;   //ワールド変換行列
    int material;               //マテリアルのID
    bool has_tangent;           //頂点データに接線が含まれているかの判定フラグ
    int skin;                   //スキニング用データの参照用インデックス
    int pad;                    //定数バッファのアライメント調整用パディング
};

cbuffer SCENE_CONSTNANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;         //ビュー行列とプロジェクション行列を合成した行列
    float4 camera_position;                     //ワールド空間におけるカメラの現在位置
    float4 light_color;                         //平行光源の色と強さ
    float4 ambient_color;                       //環境光の基本色
    row_major float4x4 light_view_projection;   //ライトから見たビュー・プロジェクション合成行列
    float4 light_direction;                     //平行香華の方向ベクトル
}

static const uint PRIMITIVE_MAX_JOINTS = 512;
cbuffer PRIMITIVE_JOINT_CONSTANTS : register(b2)
{
    row_major float4x4 joint_matrices[PRIMITIVE_MAX_JOINTS];
}