#include "RepulsiveMove.h"

//=================
//斥力更新処理
//=================
DirectX::XMFLOAT3 RepulsiveMove::UpdateRepulsion
(
    const DirectX::XMFLOAT3& current_pos,   //自身の座標
    const DirectX::XMFLOAT3& target_pos,    //対象の座標
    const float repulsion_power,            //最大の反発スピード
    const float effect_radius               //斥力が届く影響範囲
)
{
    //----------------------------------
    //ベクトル計算の準備と方向の算出
    //----------------------------------
    DirectX::XMVECTOR start_vector = DirectX::XMLoadFloat3(&current_pos);   //XMVECTOR型に変換した自身の座標
    DirectX::XMVECTOR end_vector = DirectX::XMLoadFloat3(&target_pos);      //XMVECTOR型に変換した対象の座標
    DirectX::XMVECTOR diff_vector = DirectX::XMVectorSubtract(start_vector, end_vector);    //遠ざかる方向ベクトル
    DirectX::XMVECTOR length_vector = DirectX::XMVector3Length(diff_vector);                //方向ベクトルの長さ

    //-------------------------------
    //距離の判定と斥力の減算計算
    //-------------------------------
    float distance = 0.0f;  //距離
    DirectX::XMStoreFloat(&distance, length_vector);    //float型に変換
    if (distance < 0.001f || distance > effect_radius) return current_pos;  //0除算または影響範囲外の場合
    float current_speed = repulsion_power * (1.0f - (distance / effect_radius));    //斥力の速度

    //-------------------------
    //正規化と移動量の適用
    //-------------------------
    DirectX::XMVECTOR dir_vector = DirectX::XMVector3Normalize(diff_vector);        //遠ざかる方向ベクトルを正規化
    DirectX::XMVECTOR velocity_vector = DirectX::XMVectorScale(dir_vector, current_speed); //斥力での移動量
    DirectX::XMVECTOR result_vector = DirectX::XMVectorAdd(start_vector, velocity_vector);  //実際の移動量

    //------------
    //出力
    //------------
    DirectX::XMFLOAT3 next_pos; //結果格納
    DirectX::XMStoreFloat3(&next_pos, result_vector);   //XMFLOAT3型に変換
    return next_pos;
}
