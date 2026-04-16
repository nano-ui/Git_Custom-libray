#include "gltf_model.hlsli"

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
    pbr_metallic_roughness pbr_matellic_roughness; //PBRパラメータ
    normal_texture_info normal_texture; //法線マップ設定
    occlusion_texture_info occlusion_texture; //オクルージョン設定
    texture_info emissive_texture; //エミッシブテクスチャ設定
};

StructuredBuffer<material_constants> materials : register(t0);


float4 main(VS_OUT pin) : SV_TARGET
{
    material_constants m = materials[material];
    
    //--------------------------
    //ライティング計算の準備
    //--------------------------
    float3 N = normalize(pin.w_normal.xyz);     //ワールド空間の法線を正規化し単位ベクトルにする
    float3 L = normalize(-light_direction.xyz); //平行光源の逆方向を計算し、表面からライトへ向かう単位ベクトルを求める
    
    //-----------------
    //拡散反射の計算
    //-----------------
    float3 color = max(0, dot(N, L) * m.pbr_matellic_roughness.basecolor_factor.rgb); //面の明るさを算出
	
    return float4(color, 1);
}