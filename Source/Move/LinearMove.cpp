#include "LinearMove.h"

//=====================
//直線移動更新処理
//=====================
DirectX::XMFLOAT3 LinearMove::UpdateLiner
(
	const DirectX::XMFLOAT3& current_pos,	//自身の座標
	const DirectX::XMFLOAT3& target_pos,	//対象の座標
	const float speed						//移動速度
)
{
	//------------------------
	//ベクトル計算の準備
	//------------------------
	DirectX::XMVECTOR start_vector = DirectX::XMLoadFloat3(&current_pos);	//現在の座標
	DirectX::XMVECTOR end_vector = DirectX::XMLoadFloat3(&target_pos);		//対象の座標

	//----------------
	//移動量の算出
	//----------------
	DirectX::XMVECTOR diff_vector = DirectX::XMVectorSubtract(end_vector, start_vector);	//方向ベクトル
	DirectX::XMVECTOR length_vector = DirectX::XMVector3Length(diff_vector);				//ベクトルの長さ

	//---------------------
	//距離判定と正規化
	//---------------------
	float distance = 0.0f;	//距離
	DirectX::XMStoreFloat(&distance, length_vector);	//距離をfloat型に変換
	if (distance < 0.001f) return current_pos;			//0除算防止
	DirectX::XMVECTOR dir_vector;	//正規化されたベクトルの長さ
	dir_vector = DirectX::XMVector3Normalize(diff_vector);	//ベクトルの長さを正規化
	DirectX::XMVECTOR velocity_vector;	//移動量
	velocity_vector = DirectX::XMVectorScale(dir_vector, speed);	//方向ベクトルに速度を掛けて移動量を計算

	//---------------
	//結果の出力
	//---------------
	DirectX::XMFLOAT3 result_vector;	//最終的な移動量
	DirectX::XMStoreFloat3(&result_vector, velocity_vector);
	return result_vector;
}