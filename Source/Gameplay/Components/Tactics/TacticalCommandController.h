#pragma once
#include "Gameplay\Components\Base\Component.h"

#include <DirectXMath.h>

class Camera;
class CollisionManager;
class ShapeRenderer;

class TacticalCommandController : public Component
{
public:
	//コンストラクタ
	TacticalCommandController();

	//デストラクタ
	~TacticalCommandController()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time) override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

public:
	//座標の設定
	void SetTargetPosition(DirectX::XMFLOAT3 pos) { target_position = pos; }

	//座標を取得
	DirectX::XMFLOAT3 GetTargetPosition()const { return target_position; }

	//座標指定フラグの設定
	void SetHasTarget(bool flag) { has_target = flag; }

	//座標指定フラグの取得
	bool GetHasTarget()const { return has_target; }

	//レイの最大距離の設定
	void SetRayDistance(float distance) { max_ray_distance = distance; }

	//レイの最大距離の取得
	float GetRayDistance()const { return max_ray_distance; }

	//当たり判定マネージャーの設定
	void SetCollisionManager(CollisionManager* manager);

private:
	DirectX::XMFLOAT3 target_position;	//対象の座標
	bool has_target;					//座標指定フラグ
	float max_ray_distance;				//レイの最大距離
	CollisionManager* collision_manager;//参照先ポインタ
};

