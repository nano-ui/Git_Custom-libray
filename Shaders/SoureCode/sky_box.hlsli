
//行列を渡すための定数バッファ
cbuffer OBJECT_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world; //スカイボックスのワールド行列
};

//シーン全体で共有する定数バッファ
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection; //ビュー・プロジェクション行列
    float4 light_direction;             //平行光源の方向
    float4 camera_position;             //カメラの位置座標
    float4 light_color;                 //光源の色
    float4 ambient_color;               //環境光の色と強さ
};

//頂点シェーダーへの入力構造体
struct VS_IN
{
    float3 position : POSITION; //頂点のローカル座標
};

//ピクセルシェーダーへの入力構造体
struct VS_OUT
{
    float4 position : SV_POSITION;      //画面上の描画座標
    float3 local_position : POSITION;   //キューブマップをサンプリングするための方向ベクトル
};