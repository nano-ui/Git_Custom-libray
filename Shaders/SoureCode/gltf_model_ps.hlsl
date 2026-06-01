#include "gltf_model.hlsli"

//------------------------------
//テクスチャスロットの定義
//------------------------------
#define BASECOLOR_TEXTURE 0             //基本色のスロット番号
#define METALLIC_ROUGHNESS_TEXTURE 1    //金属度・粗さのスロット番号
#define NORMAL_TEXTURE 2                //法線マップのスロット番号
#define EMISSIVE_TEXTURE 3              //自己発光マップのスロット番号
#define OCCLUSION_TEXTURE 4             //オクルージョンマップのスロット番号
Texture2D<float4> material_textures[5] : register(t1);  //テクスチャ配列のレジスタ設定

//-----------------------------
//サンプラーステートの定義
//-----------------------------
#define POINT 0         //ポイントサンプリング
#define LINEAR 1        //線形フィルタリング
#define ANISOTROPIC 2   //異方性フィルタリング
SamplerState sampler_states[3] : register(s0);  //サンプラー配列のレジスタ設定

//テクスチャ参照用情報の基本構造体
struct texture_info
{
    int index;      //テクスチャの番号
    int texcoord;   //UV座標の設定番号
};

//法線マップ用情報の構造体
struct normal_texture_info
{
    int index;      //テクスチャの番号
    int texcoord;   //UV座標の設定
    float scale;    //法線の強さ
};

//オクルージョンマップ用情報の構造体
struct occlusion_texture_info
{
    int index;      //テクスチャの番号
    int texcoord;   //UV座標の設定
    float strength; //オクルージョンの影響度
};

//PBRの金属度・粗さに関する構造体
struct pbr_metallic_roughness
{
    float4 basecolor_factor;        //基本色の係数
    texture_info basecolor_texture; //ベースカラーテクスチャ情報
    float metallic_factor;          //金属度係数
    float roughness_factor;         //粗さ係数
    texture_info metallic_roughness_texture;    //金属度・粗さテクスチャ情報
};


//マテリアルの定数データ構造体
struct material_constants
{
    float3 emissive_factor; //自発光の係数
    int alpha_mode; //アルファブレンドモード
    float alpha_cutoff; //アルファテストの閾値
    int double_sided; //両面描画フラグ
    pbr_metallic_roughness pbr_metallic_roughness; //PBRパラメータ
    normal_texture_info normal_texture; //法線マップ設定
    occlusion_texture_info occlusion_texture; //オクルージョン設定
    texture_info emissive_texture; //エミッシブテクスチャ設定
};

StructuredBuffer<material_constants> materials : register(t0);


float4 main(VS_OUT pin) : SV_TARGET
{
    //-----------------------
    //マテリアル情報の取得
    //-----------------------
    material_constants m = materials[material]; //バッファから現在のマテリアルデータを抽出
    
    //---------------------
    //ベースカラーの決定
    //---------------------
    float4 basecolor = m.pbr_metallic_roughness.basecolor_texture.index > -1 ?  //テクスチャが指定されているかを判定
    material_textures[BASECOLOR_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord) :    //テクスチャをサンプリング
    m.pbr_metallic_roughness.basecolor_factor;  //未指定なら係数を使用
    
    float3 emissive = m.emissive_texture.index > -1 ? //テクスチャが指定されているかを判定
    material_textures[EMISSIVE_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord).rgb : //テクスチャをサンプリング
    m.emissive_factor;  //未指定なら係数を使用
    
    //--------------------------
    //ライティング計算の準備
    //--------------------------
    float3 N = normalize(pin.w_normal.xyz);     //ワールド空間の法線を正規化し単位ベクトルにする
    float3 L = normalize(-light_direction.xyz); //平行光源の逆方向を計算し、表面からライトへ向かう単位ベクトルを求める
    
    //-----------------
    //最終出力色の合成
    //-----------------
    float3 color = max(0, dot(N, L)) * basecolor.rgb + emissive; //面の明るさを算出
	
    return float4(color, basecolor.a);
}