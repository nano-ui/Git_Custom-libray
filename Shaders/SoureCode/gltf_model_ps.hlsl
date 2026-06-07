#include "gltf_model.hlsli"

static const float PI = 3.14159265358979f;
static const float GammaFactor = 2.2f; //ガンマ補正
static const float shadow_bias = 0.0005f;
static const float3 shadow_factor = float3(0.2f, 0.2f, 0.2f);

//------------------------------
//テクスチャスロットの定義
//------------------------------
#define BASECOLOR_TEXTURE 0             //基本色のスロット番号
#define METALLIC_ROUGHNESS_TEXTURE 1    //金属度・粗さのスロット番号
#define NORMAL_TEXTURE 2                //法線マップのスロット番号
#define EMISSIVE_TEXTURE 3              //自己発光マップのスロット番号
#define OCCLUSION_TEXTURE 4             //オクルージョンマップのスロット番号
Texture2D<float4> material_textures[5] : register(t1);  //テクスチャ配列のレジスタ設定

TextureCube diffuse_iem_map : register(t33);    //拡散反射用環境マップ
TextureCube specular_pmrem_map : register(t34); //鏡面反射用環境マップ
Texture2D lut_ggx_map : register(t35);          //BRDFルックアップテーブル
Texture2D shadow_map : register(t10);           //ライト深度が書き込まれたシャドウマップテクスチャ

//-----------------------------
//サンプラーステートの定義
//-----------------------------
#define POINT 0         //ポイントサンプリング
#define LINEAR 1        //線形フィルタリング
#define ANISOTROPIC 2   //異方性フィルタリング
SamplerState sampler_states[3] : register(s0);  //サンプラー配列のレジスタ設定
SamplerState shadow_sampler_state : register(s10); //シャドウマップ用サンプラーステート

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
    int alpha_mode;         //アルファブレンドモード
    float alpha_cutoff;     //アルファテストの閾値
    int double_sided;       //両面描画フラグ
    pbr_metallic_roughness pbr_metallic_roughness;  //PBRパラメータ
    normal_texture_info normal_texture;             //法線マップ設定
    occlusion_texture_info occlusion_texture;       //オクルージョン設定
    texture_info emissive_texture;                  //エミッシブテクスチャ設定
};

StructuredBuffer<material_constants> materials : register(t0);

//ラフネル項
float3 f_schlick(float3 f0,float3 f90,float VoH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0f - VoH, 0.0f, 1.0f), 5.0f);
}

//幾何減衰項
float v_ggx(float NoL, float NoV, float alpha_roughness)
{
    float alpha_roughness_sq = alpha_roughness * alpha_roughness;
    float ggxv = NoL * sqrt(NoV * NoV * (1.0f - alpha_roughness_sq) + alpha_roughness_sq);
    float ggxl = NoV * sqrt(NoL * NoL * (1.0f - alpha_roughness_sq) + alpha_roughness_sq);
    float ggx = ggxv + ggxl;
    return (ggx > 0.0f) ? 0.5f / ggx : 0.0f;
}

//法線分布関数
float d_ggx(float NoH, float alpha_roughness)
{
    float alpha_roughness_sq = alpha_roughness * alpha_roughness;
    float f = (NoH * NoH) * (alpha_roughness_sq - 1.0f) + 1.0f;
    return alpha_roughness_sq / (PI * f * f);
}

float4 main(VS_OUT pin, bool is_front_face : SV_IsFrontFace) : SV_TARGET
{
    //マテリアル情報の取得
    material_constants m = materials[material];
    
    //ベースカラーとアルファの決定
    float4 basecolor = m.pbr_metallic_roughness.basecolor_texture.index > -1 ? 
    material_textures[BASECOLOR_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord) : 
    m.pbr_metallic_roughness.basecolor_factor;
    
    if(m.alpha_mode == 1 && basecolor.a < m.alpha_cutoff)
    {
        discard;
    }
    basecolor.rgb = pow(basecolor.rgb, GammaFactor);
    
    //エミッシブ
    float3 emissive = m.emissive_texture.index > -1 ?
    material_textures[EMISSIVE_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord).rgb :
    m.emissive_factor;
    emissive = pow(emissive, GammaFactor);
    
    //法線と接線ベクトルの準備
    float3 N = normalize(pin.w_normal.xyz);
    float3 T = has_tangent ? normalize(pin.w_tangent.xyz) : float3(1, 0, 0);
    float sigma = has_tangent ? pin.w_tangent.w : 1.0;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);
    
    //両面描画かつ裏面を見ている場合、法線・接線・従法線を反転
    if (m.double_sided && !is_front_face)
    {
        N = -N;
        T = -T;
        B = -B;
    }
    
    //法線マッピング
    if(m.normal_texture.index > -1)
    {
        float4 sampled = material_textures[NORMAL_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord);
        float3 normal_factor = sampled.xyz;
        normal_factor = (normal_factor * 2.0f) - 1.0f;
        normal_factor = normalize(normal_factor * float3(m.normal_texture.scale, m.normal_texture.scale, 1.0f));
        N = normalize((normal_factor.x * T) + (normal_factor.y * B) + (normal_factor.z * N));
    }
    
    //メタリックとラフネス
    float metallic = m.pbr_metallic_roughness.metallic_factor;
    float roughness = m.pbr_metallic_roughness.roughness_factor;
    if (m.pbr_metallic_roughness.metallic_roughness_texture.index > -1)
    {
        float4 mr_sampled = material_textures[METALLIC_ROUGHNESS_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord);
        roughness *= mr_sampled.g; // Gチャンネルがラフネス
        metallic *= mr_sampled.b; // Bチャンネルがメタリック
    }
    
    //オクルージョン
    float occlusion = 1.0f;
    if (m.occlusion_texture.index > -1)
    {
        occlusion = material_textures[OCCLUSION_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord).r;
    }
    
    //PBR計算のためのベクトルとパラメータ準備
    float3 V = normalize(camera_position.xyz - pin.w_position.xyz); //視線ベクトル
    float NoV = max(0.001f, dot(N, V));                             //法線と視線の内積
    
    //絶縁体と金属の反射率補間
    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), basecolor.rgb, metallic);
    float3 f90 = float3(1.0f, 1.0f, 1.0f);
    
    //拡散反射色
    float3 diffuse_color = basecolor.rgb * (1.0f - metallic);
    
    //ラフネスの二乗をalpha_roughnessとして使用
    float alpha_roughness = roughness * roughness;
    
    //最終的な出力用の色変数を初期化
    float3 total_diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 total_specular = float3(0.0f, 0.0f, 0.0f);
    
    //ライティング計算
    float3 L = normalize(-light_direction.xyz); //光源へのベクトル
    float3 H = normalize(V + L);                //ハーフベクトル 
    
    float NoL = max(0.001f, dot(N, L));
    float NoH = max(0.001f, dot(N, H));
    float VoH = max(0.001f, dot(V, H));
    
    if (NoL > 0.0f || NoV > 0.0f)
    {
        //BRDF計算
        float3 F = f_schlick(f0, f90, VoH);
        float V_vis = v_ggx(NoL, NoV, alpha_roughness);
        float D = d_ggx(NoH, alpha_roughness);

        //鏡面反射
        float3 specular_brdf = F * V_vis * D;
        
        //拡散反射
        float3 diffuse_brdf = (1.0f - F) * (diffuse_color / PI);
        
        //光源のエネルギー
        float3 radiance = light_color.rgb * light_color.a * NoL;
        
        //シャドウマップによる影の判定
        float2 shadow_uv = pin.shadow_texcoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
        if (shadow_uv.x >= 0.0f && shadow_uv.x <= 1.0f && shadow_uv.y >= 0.0f && shadow_uv.y <= 1.0f)
        {
            float shadow_depth = shadow_map.Sample(shadow_sampler_state, shadow_uv).r;
            if(pin.shadow_texcoord.z - shadow_bias>shadow_depth)
            {
                radiance *= shadow_factor;
            }
        }

        total_diffuse += diffuse_brdf * radiance;
        total_specular += specular_brdf * radiance;
    }
    
    //間接光の計算
    float3 R = reflect(-V, N); //反射ベクトル
    
    //拡散反射IBL
    float3 diffuse_irradiance = diffuse_iem_map.Sample(sampler_states[LINEAR], N).rgb;
    float3 F_ibl = f_schlick(f0, f90, NoV);
    float3 ibl_diffuse = (1.0f - F_ibl) * diffuse_color * diffuse_irradiance;
    
    //鏡面反射IBL
    uint width, height, mip_levels;
    specular_pmrem_map.GetDimensions(0, width, height, mip_levels);
    float lod = roughness * float(mip_levels - 1); //ラフネスに応じてミップマップレベルを決定
    
    float3 specular_radiance = specular_pmrem_map.SampleLevel(sampler_states[LINEAR], R, lod).rgb;
    float2 brdf_sample_point = float2(NoV, roughness);
    float2 env_brdf = lut_ggx_map.Sample(sampler_states[LINEAR], brdf_sample_point).rg;
    
    float3 ibl_specular = specular_radiance * (f0 * env_brdf.x + env_brdf.y);
    
    //IBL結果を合算
    total_diffuse += ibl_diffuse * ambient_color.a;
    total_specular += ibl_specular * ambient_color.a;
    
    //最終出力色の合成
    float3 final_color = total_diffuse + total_specular + emissive;
    if (m.alpha_mode == 2)
    {
        final_color = (total_diffuse + emissive) + (total_specular * basecolor.a);
    }
    final_color *= occlusion;
    final_color = pow(final_color, 1.0f / GammaFactor);
    return float4(final_color, basecolor.a);
}