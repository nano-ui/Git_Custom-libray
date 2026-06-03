#include "CircularMove.h"

//======================
//円移動の更新処理
//======================
DirectX::XMFLOAT3 CircularMove::UpdateCircular
(
	const DirectX::XMFLOAT3& current_pos,	//自身の座標
	const DirectX::XMFLOAT3& target_pos,	//対象の座標
	float speed,							//移動速度
	float direction,						//回転方向
	float base_distance,					//維持したい距離
	float correction_weight					//補正の強さ
)
{
	//------------------------
	//相対ベクトルを求める
	//------------------------
	DirectX::XMFLOAT3 diff;	//相対ベクトル
	diff.x = target_pos.x - current_pos.x;	//X軸の相対ベクトルを算出	
	diff.y = target_pos.y - current_pos.y;	//Y軸の相対ベクトルを算出
	diff.z = target_pos.z - current_pos.z;	//Z軸の相対ベクトルを算出

	//------------------
	//距離を求める
	//------------------
	float distance;		//対象との距離
	distance = sqrtf((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));	//対象との距離を算出

	if (distance <= 0.0f)return { 0.0f,0.0f,0.0f };	//0除算していないか確認

	//--------------------------------
	//正規化して速度ベクトルを算出
	//--------------------------------
	diff.x = (diff.x / distance) * speed;	//X軸の速度ベクトルを算出
	diff.y = (diff.y / distance) * speed;	//Y軸の速度ベクトルを算出
	diff.z = (diff.z / distance) * speed;	//Z軸の速度ベクトルを算出

	//-------------------------
	//接線ベクトルを算出
	//-------------------------
	float tangent_x;	//X軸の接線ベクトル
	float tangent_z;	//Z軸の接線ベクトル
	tangent_x = -diff.z * direction;	//X軸の接線ベクトルを算出
	tangent_z = diff.x * direction;		//Z軸の接線ベクトルを算出

	//-------------------------
	//補正ベクトルを作成
	//-------------------------
	float dist_error = distance - base_distance;	//距離の差分
	float correction_x = (diff.x / distance) * dist_error * correction_weight;	//Xベクトルの補正
	float correction_z = (diff.z / distance) * dist_error * correction_weight;	//Zベクトルの補正

	//-------------------------
	//速度ベクトルを合成
	//-------------------------
	DirectX::XMFLOAT3 velocity;	//速度ベクトル
	velocity.x = tangent_x + correction_x;	//X軸の合成
	velocity.y = 0.0f;						//高さ移動無し
	velocity.z = tangent_z + correction_z;	//Z軸の合成

	return velocity;
}

//==============================================
//対象を中心に円を描きながら目標地点に移動
//==============================================
DirectX::XMFLOAT3 CircularMove::UpdateCirclarWithPoint
(
	const DirectX::XMFLOAT3& position,				//自身の座標
	const DirectX::XMFLOAT3& target_pos,			//対象の座標(回転の中心)
	float speed,									//移動速度
	float direction,								//回転方向
	const DirectX::XMFLOAT3& current_target_pos,	//目標地点
	float correction_weight							//補正の強さ
)
{
	//------------------------
	//相対ベクトルを求める
	//------------------------
	DirectX::XMFLOAT3 diff;	//相対ベクトル
	diff.x = target_pos.x - position.x;	//X軸の相対ベクトルを算出	
	diff.y = target_pos.y - position.y;	//Y軸の相対ベクトルを算出
	diff.z = target_pos.z - position.z;	//Z軸の相対ベクトルを算出

	//------------------
	//距離を求める
	//------------------
	float distance;		//対象との距離
	distance = sqrtf((diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z));	//対象との距離を算出

	if (distance <= 0.0f)return { 0.0f,0.0f,0.0f };	//0除算していないか確認

	//-------------------------
	//接線ベクトルを算出
	//-------------------------
	float tangent_x;	//X軸の接線ベクトル
	float tangent_z;	//Z軸の接線ベクトル
	tangent_x = (-diff.z / distance) * speed * direction;	//X軸の接線ベクトルを算出
	tangent_z = (diff.x / distance) * speed * direction;	//Z軸の接線ベクトルを算出


	//---------------------------------------
	//自分から目標地点へのベクトルを計算
	//---------------------------------------
	DirectX::XMFLOAT3 to_point;	//目標地点
	to_point.x = current_target_pos.x - position.x;	//X軸の目標地点を算出
	to_point.z = current_target_pos.z - position.z;	//Z軸の目標地点を算出

	float dist_to_point = sqrtf((to_point.x * to_point.x) + (to_point.z * to_point.z));	

	//-------------------------
	//補正ベクトルを作成
	//-------------------------
	if (dist_to_point <= 0.0f)return { 0.0f,0.0f,0.0f };	//0除算していないか確認

	float correction_x = (to_point.x / dist_to_point) * correction_weight;	//Xベクトルの補正
	float correction_z = (to_point.z / dist_to_point) * correction_weight;	//Zベクトルの補正

	//-------------------------
	//速度ベクトルを合成
	//-------------------------
	DirectX::XMFLOAT3 velocity;	//速度ベクトル
	velocity.x = tangent_x + correction_x;	//X軸の合成
	velocity.y = 0.0f;						//高さ移動無し
	velocity.z = tangent_z + correction_z;	//Z軸の合成

	//--------------
	//速度制限
	//--------------
	float val_sq = (velocity.x * velocity.x) + (velocity.z * velocity.z);
	if (val_sq > (speed * speed))
	{
		float ratio = speed / sqrtf(val_sq);
		velocity.x *= ratio;
		velocity.z *= ratio;
	}

	return velocity;
}
