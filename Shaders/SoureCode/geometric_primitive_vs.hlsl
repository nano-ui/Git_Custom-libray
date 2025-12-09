#include "geometric_primitive.hlsli"


VS_OUT main(float4 position:POSITION,float4 normal:NORMAL)
{
    VS_OUT vont;
    
    //最終的なスクリーン上の位置に変換
    vont.position = mul(position, mul(world, view_projection));
    
    //平行移動の影響を受けないようにする
    normal.w = 0;
    
    //法線ベクトルを正規化してローカル空間からワールド空間に変換
    float4 N = normalize(mul(normal, world));
    
    //ライティング計算
    float4 L = normalize(-light_direction);
    vont.color.rgb = material_color.rgb * max(0, dot(L, N));
    
    //アルファ値(透明度)の設定
    vont.color.a = material_color.a;
    
    return vont;
}