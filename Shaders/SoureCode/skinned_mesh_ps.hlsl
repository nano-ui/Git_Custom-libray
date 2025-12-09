#include "skinned_mesh.hlsli"

/*最近傍補間（ピクセルが粗くなるが高速）*/
#define POINT 0
//線形補間（ピクセルが滑らかになるがやや遅い）
#define LINEAR 1
//異方性フィルタリング（角度のある面でもテクスチャがぼやけず高品質）
#define ANISOTROPIC 2 

/*テクスチャをサンプリングする際の補間方法や繰り返し方などを定義するオブジェクト*/
SamplerState sampler_states[3] : register(s0);
/*2Dテクスチャ（画像） にアクセスするためのオブジェクト*/
Texture2D texture_maps[4] : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
    /*テクスチャの対応するピクセルの色がRGBA（赤、緑、青、アルファ）の
    4次元ベクトルで格納*/
    float4 color = texture_maps[0].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
    
    float alpha = color.a;

#if 1
    //Inverse gamma process
    const float GAMMA = 2.2f;
    color.rgb = pow(color.rgb, GAMMA);
#endif


    //ピクセルが属する表面の「真の」向きを表す
    float3 N = normalize(pin.world_normal.xyz);
    
    float3 T = normalize(pin.world_tangent.xyz); //接ベクトル
    float sigma = pin.world_tangent.w;
    T = normalize(T - N * dot(N, T)); //TをNと直交に
    float3 B = normalize(cross(N, T) * sigma); //接ベクトルから従ベクトル（ビットンジェント）を作成
    
    //法線マップのピクセルを使って「そのピクセルの正しい法線方向」をワールド空間に変換
    float4 normal = texture_maps[1].Sample(sampler_states[LINEAR], pin.texcoord);
    normal = (normal * 2.0f) - 1.0f;
    N = normalize((normal.x * T) + (normal.y * B) + (normal.z * N)); //接空間 → ワールド空間へ
    
    //ピクセルから光源へ向かう方向を表す
    float3 L = normalize(-light_direction.xyz);
    //テクスチャの持つ色や模様が、適切に陰影の計算（拡散光）の影響を受ける
    float3 diffuse = color.rgb * max(0, dot(N, L));
    
    float3 V = normalize(camera_position.xyz - pin.world_position.xyz);//視線ベクトル
    float3 specular = pow(max(0, dot(N, normalize(V + L))), 64);//鏡面反射(ブリンフォンモデル)
    
    /*最終的なピクセル色が、テクスチャの色、拡散光の強度、
    および pin.color の全ての影響を受ける*/
    return float4(diffuse + specular, alpha) * pin.color;
    //return float4(N.xyz, 1);
}