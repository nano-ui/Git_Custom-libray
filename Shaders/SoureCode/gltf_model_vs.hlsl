#include "glth_model.hlsli"

VS_OUT main( VS_IN vin)
{
    VS_OUT vout;
    
    //-------------------
    //座標変換の計算
    //-------------------
    vin.position.w = 1; //位置情報として扱うためにw成分を1に設定
    vout.position = mul(vin.position, mul(world, view_projection));  //ローカル座標からワールド座標に変換
    vout.w_position = mul(vin.position, world); //ワールド座標から射影空間(画面座標)に変換
    
    //------------------
    //法線・接線の計算
    //------------------
    vin.normal.w = 0;   //w成分を0にして変換の影響をなくす
    vout.w_normal = normalize(mul(vin.normal, world));  //法線をワールド空間に変換し正規化
    
    float sigma = vin.tangent.w;    //従法線の向きを保存
    vin.tangent.w = 0; //w成分を0にして変換の影響をなくす
    vout.w_tangent = normalize(mul(vin.tangent, world));    //接線をワールド空間へ変換し、正規化
    vout.w_tangent = sigma; //保存した従法線の向きを適用
    
    vout.texcoord = vin.texcoord;   //テクスチャ座標をピクセルシェーダーに渡す
    
    return vout;
}