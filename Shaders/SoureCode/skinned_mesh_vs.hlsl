#include "skinned_mesh.hlsli"

VS_OUT main(VS_IN vin)
{
    VS_OUT vont;
    
    //行列変換時に並行移動の影響をなくす
    vin.normal.w = 0;
   
    //接線の向き（左右反転情報）を保持
    float sigma = vin.tangent.w;
    vin.tangent.w = 0;
    
    //最終的なスキンされた位置
    float4 blended_position = { 0, 0, 0, 1 };
    
    //スキンされた法線ベクトル
    float4 blended_normal = { 0, 0, 0, 0 };
    
    //合成される接線
    float4 blended_tangent = { 0, 0, 0, 0 };
    
    //頂点が影響を受ける 4つのボーンインデックスに対して繰り返し
    for (int bone_index = 0; bone_index < 4; bone_index++)
    {
        //全ボーンの結果を加重平均（ウェイト付き合成）する
        blended_position += vin.bone_wights[bone_index]
        * mul(vin.position, bone_transforms[vin.bone_indices[bone_index]]);
        
        //各ボーンで変換された方向ベクトルを加重平均
        blended_normal += vin.bone_wights[bone_index]
        * mul(vin.normal, bone_transforms[vin.bone_indices[bone_index]]);
        
        //各ボーンで変換された接線ベクトルを加重平均
        blended_tangent += vin.bone_wights[bone_index]
        * mul(vin.tangent, bone_transforms[vin.bone_indices[bone_index]]);
    }
    
    //合成結果を再代入
    vin.position = float4(blended_position.xyz, 1.0f);
    vin.normal = float4(blended_normal.xyz, 0.0f);
    vin.tangent = float4(blended_tangent.xyz, 0.0f);
    
    //頂点位置の変換
    vont.position = mul(vin.position, mul(world, view_projection));
    
    //ワールド空間での位置
    vont.world_position = mul(vin.position, world);
    
    //スクリーン座標に変換
    vont.position = mul(vont.world_position, view_projection);
        
    //ワールド空間での法線
    vont.world_normal = vin.normal;//normalize(mul(vin.normal, world));
    
    //tangent の補助情報（sigma）を出力
    vont.world_tangent = normalize(mul(vin.tangent, world));
    vont.world_tangent.w = sigma;
    
    //テクスチャ座標をそのまま出力
    vont.texcoord = vin.texcoord;
    
#if 1
    //オブジェクトのマテリアル色を頂点の色として出力
    vont.color = material_color;  
#else
    //色を初期化
    vont.color = 0;
    
    //赤、緑、青、白の4つの色を定義
    const float4 bone_color[4] =
    {
        { 1, 0, 0, 1 },
        { 0, 1, 0, 1 },
        { 0, 0, 1, 1 },
        { 1, 1, 1, 1 },
    };
    
    //スキニング情報の視覚化
    for (int bone_index = 0; bone_index < 4;bone_index++)
    {
        //頂点は、影響を受けるボーンの種類に応じて色が混ざり合った状態で表示
        vont.color += bone_color[vin.bone_indices[bone_index] % 4]
        * vin.bone_wights[bone_index];
    }
    
#endif
    
    //計算された頂点情報を返す
    return vont;
}
