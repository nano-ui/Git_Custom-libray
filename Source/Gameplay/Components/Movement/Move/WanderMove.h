#pragma once

#include "Gameplay\Components\Movement\Move\MoveBase.h"

#include <DirectXMath.h>
#include <memory>

class LinearMove;
class ShapeRenderer;

class WanderMove : MoveBase
{
public:
	//コンストラクタ
	WanderMove();

	//デストラクタ
	~WanderMove()override;

	//更新処理
	DirectX::XMFLOAT3 Update(
		const DirectX::XMFLOAT3& current_pos,
		const DirectX::XMFLOAT3& target_pos,
		const StateBlackboard* blackboard)override;

	//徘徊処理
	DirectX::XMFLOAT3 UpdateWander(
		float elapsed_time,
		const DirectX::XMFLOAT3& current_pos,
		float speed
	);

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer);

private:
	std::unique_ptr<LinearMove> liner_move;	//直線移動
	DirectX::XMFLOAT3 home_position;		//中心座標
	float wander_radius;					//巡回半径
	DirectX::XMFLOAT3 target_pos;			//目的地
	float wait_duration;					//待機時間
	float wait_timer;						//タイマー
	bool is_wanting;						//待機フラグ
};

