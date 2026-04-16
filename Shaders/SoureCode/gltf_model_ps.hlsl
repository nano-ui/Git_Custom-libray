#include "glth_model.hlsli"

float4 main(VS_OUT pin) : SV_TARGET
{
    //--------------------------
    //ライティング計算の準備
    //--------------------------
    float3 N = normalize(pin.w_normal.xyz);     //ワールド空間の法線を正規化し単位ベクトルにする
    float3 L = normalize(-light_direction.xyz); //平行光源の逆方向を計算し、表面からライトへ向かう単位ベクトルを求める
    
    //-----------------
    //拡散反射の計算
    //-----------------
    float3 color = max(0, dot(N, L));      //法線とライト方向から、面の明るさを算出
	
    return float4(color, 1);
}