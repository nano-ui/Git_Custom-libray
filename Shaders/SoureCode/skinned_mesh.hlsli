
//情報を入れる箱
struct VS_IN
{
    float4 position : POSITION;//頂点の3D空間上の位置
    float4 normal : NORMAL;    //頂点の法線ベクトル
    float4 tangent : TANGENT;//接線ベクトル
    float2 texcoord : TEXCOORD;//テクスチャ座標
    float4 bone_wights : WEIGHTS; //頂点に影響を与えるボーンの重み（影響度）
    uint4 bone_indices : BONES;//頂点に影響を与えるボーンのインデックス（識別番号）
};

//頂点シェーダーの処理を終えた後に、情報をピクセルシェーダーに渡すデータ
struct VS_OUT
{
    //画面上での最終的な頂点位置
    float4 position : SV_POSITION;
    
    //ワールド空間（3D空間全体）での頂点の位置
    float4 world_position : POSITION;
    
    //ワールド空間での法線ベクトル
    float4 world_normal : NORMAL;
    
    //ワールド空間での接線ベクトル
    float4 world_tangent : TANGENT;
    
    //テクスチャ座標
    float2 texcoord : TEXCOORD;
    
    //頂点の色
    float4 color : COLOR;
};

//1つのスキンメッシュで使用可能なボーン（骨）の最大数
static const int MAX_BONES = 256;

//オブジェクト（3Dモデル）固有の設定値
cbuffer OBJECT_CONSTANT_BUFFER : register(b0)
{
    /*オブジェクトの「ワールド変換行列」。
    モデルを3D空間のどこに配置し、回転させ、拡大縮小するかを決定する行列*/
    row_major float4x4 world;
    
    //オブジェクトのマテリアル（素材）の色
    float4 material_color;
    
    //ボーンごとの変換行列
    row_major float4x4 bone_transforms[MAX_BONES];
};

//シーン全体の設定値
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    //3D空間のものをカメラを通して2D画面にどう映すかを決定
    row_major float4x4 view_projection;
    
    //シーンの光源の方向
    float4 light_direction;
    
    //カメラの位置
    float4 camera_position;
};