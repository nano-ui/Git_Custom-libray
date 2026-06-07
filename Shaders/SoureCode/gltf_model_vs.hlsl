#include "gltf_model.hlsli"

VS_OUT main( VS_IN vin)
{
    float sigma = vin.tangent.w; //従法線の向きを保存
    
    //スキニング計算処理
    if(skin > -1)
    {
        row_major float4x4 skin_matrix =
        vin.weights.x * joint_matrices[vin.joints.x] +
        vin.weights.y * joint_matrices[vin.joints.y] +
        vin.weights.z * joint_matrices[vin.joints.z] +
        vin.weights.w * joint_matrices[vin.joints.w];
        vin.position = mul(float4(vin.position.xyz, 1), skin_matrix);
        vin.normal = normalize(mul(float4(vin.normal.xyz, 0), skin_matrix));
        vin.tangent = normalize(mul(float4(vin.tangent.xyz, 0), skin_matrix));
    }
    
    VS_OUT vout;
    
    //座標変換の計算
    vin.position.w = 1; //位置情報として扱うためにw成分を1に設定
    vout.position = mul(vin.position, mul(world, view_projection));  //ローカル座標からワールド座標に変換
    vout.w_position = mul(vin.position, world); //ワールド座標から射影空間(画面座標)に変換
    
    //法線・接線の計算
    vin.normal.w = 0;   //w成分を0にして変換の影響をなくす
    vout.w_normal = normalize(mul(vin.normal, world));  //法線をワールド空間に変換し正規化
    
    vin.tangent.w = 0; //w成分を0にして変換の影響をなくす
    vout.w_tangent = normalize(mul(vin.tangent, world));    //接線をワールド空間へ変換し、正規化
    vout.w_tangent.w = sigma; //保存した従法線の向きを適用
    
    vout.texcoord = vin.texcoord;   //テクスチャ座標をピクセルシェーダーに渡す
    
    //シャドウマップ用座標変換
    float4 light_space_position = mul(vout.w_position, light_view_projection);
    vout.shadow_texcoord = light_space_position.xyz / light_space_position.w;
    
    return vout;
}