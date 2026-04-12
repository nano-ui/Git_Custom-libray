#include "gltf_dynamic_model.hlsli"

VS_OUT main(VS_IN input)
{
    //-----------------
    //初期化
    //-----------------
    
    //出力用構造体
    VS_OUT output;
    //接線の向きを保持
    float sigma = input.tangent.w;   
    //スキニング計算用のベクトルを初期化
    float4 blended_position = { 0, 0, 0, 1 };
    float3 blended_normal = { 0, 0, 0 };
    float3 blended_tangent = { 0, 0, 0 };
    
    //------------------
    //スキニング計算
    //------------------
    
    //頂点が影響を受ける4つのボーンインデックスに対して繰り返し処理
    for (int i = 0; i < 4;i++)
    {
        //ボーンのインデックスと重みを取得
        uint bone_index = input.bone_indices[i];    //ボーン番号
        float weight = input.bone_weights[i];       //ボーンの影響度
        
        //各ボーンの変換行列を適用し、重みを掛けて加算
        //座標の合成
        blended_position.xyz += weight * mul(float4(input.position, 1.0f), bone_transformes[bone_index]).xyz;
        //法線の合成
        blended_normal += weight * mul(input.normal, (float3x3) bone_transformes[bone_index]);
        //接線の合成
        blended_tangent += weight * mul(input.tangent.xyz, (float3x3) bone_transformes[bone_index]);
    }
    
    //--------------------
    //座標変換
    //--------------------
    
    //ワールド空間での位置を計算
    float4 world_pos = mul(blended_position, world_matrix);
    //ピクセルシェーダー用に変換後のワールド座標を保存
    output.world_pos = world_pos.xyz;
    //スクリーン座標に変換
    output.position = mul(world_pos, view_projection);
    
    //-------------------
    //法線・接線変換
    //-------------------
    
    //ワールド行列の回転成分のみを抽出し、法線と接線をワールド空間へ変換
    float3x3 world_rotate = (float3x3) world_matrix;
    //ワールド行列の法線を計算し、正規化
    output.normal = normalize(mul(blended_normal, world_rotate));
    //ワールド行列の接線を計算し、正規化
    output.tangent = normalize(mul(blended_tangent, world_rotate));
    
    //-------------------
    //データのコピー
    //-------------------
    
    //テクスチャ座標を出力へコピー
    output.texcoord = input.texcoord;
    
    return output;
}