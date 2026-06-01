#pragma once

#include "MoveBase.h"
#include "LinearMove.h"

//=============
//円移動
//=============
class CircularMove : public MoveBase
{
public:
	//円移動の更新処理
	DirectX::XMFLOAT3 UpdateCircular(
		const DirectX::XMFLOAT3& current_pos,	//自身の座標
		const DirectX::XMFLOAT3& target_pos,	//対象の座標
		float speed,							//移動速度
		float direction,						//回転方向
		float base_distance,					//維持したい距離
		float correction_weight					//補正の強さ
	);

	//対象を中心に円を描きながら目標地点に移動
	DirectX::XMFLOAT3 UpdateCirclarWithPoint(
		const DirectX::XMFLOAT3& position,				//自身の座標
		const DirectX::XMFLOAT3& target_pos,			//対象の座標(回転の中心)
		float speed,									//移動速度
		float direction,								//回転方向
		const DirectX::XMFLOAT3& current_target_pos,	//目標地点
		float correction_weight							//補正の強さ
	);




	DirectX::XMFLOAT3 Update(const DirectX::XMFLOAT3& current_pos, const DirectX::XMFLOAT3& target_pos)
	{
		return { 0.0f,0.0f,0.0f };
	}
};

