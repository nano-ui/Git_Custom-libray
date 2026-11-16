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

public:
	//中心座標の設定
	void SetHomePosition(DirectX::XMFLOAT3 position) { home_position = position; }

	//中心座標の取得
	DirectX::XMFLOAT3 GetHomePosition()const { return home_position; }

	//巡回半径の設定
	void SetWanderRadius(float radius) { wander_radius = radius; }

	//巡回半径の取得
	float GetWanderRadois()const { return wander_radius; }

	//目標座標の設定
	void SetTargetPos(DirectX::XMFLOAT3 position) { target_pos = position; }

	//目標座標の取得
	DirectX::XMFLOAT3 GetTargetPos()const { return target_pos; }

	//待機時間の設定
	void SetWaitDuration(float duration) { wait_duration = duration; }

	//待機時間の取得
	float GetWanderRadius()const { return wait_duration; }

private:
	std::unique_ptr<LinearMove> liner_move;	//直線移動
	DirectX::XMFLOAT3 home_position;		//中心座標
	float wander_radius;					//巡回半径
	DirectX::XMFLOAT3 target_pos;			//目的地
	float wait_duration;					//待機時間
	float wait_timer;						//タイマー
	bool is_wanting;						//待機フラグ
};

