#pragma once

#include "Gameplay\GameObjects\Character\Character.h"
#include "Engine/Collision/Collider.h"

#include <memory>

class CollisionSphere;
class CapsuleColliderComponent;
class BoneCapsuleColliderComponent;
class AttachmentColliderComponent;
class LinearMove;
class WanderMove;
class HateComponent;

class Enemy :public Character
{
public:
	//コンストラクタ
	Enemy();

	//デストラクタ
	~Enemy()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//シリアライズ登録
	void SetupSerialization()override;

	//インスペクター登録
	void SetupInspector()override;

	// アニメーション終了イベント
	void OnAnimationEnd(uint32_t state_key) override;

	//対象を設定
	void SetTarget(const std::shared_ptr<TransformComponent>& target);

protected:
	//デフォルトパスの取得
	std::string GetDefaultModelPath() const override;

	//被弾時イベント
	void OnDamage(uint32_t attacker_id, float hate)override;

private:
	//コンポーネント群のセットアップ
	void SetupComponent();

	//座標計算・移動入力の更新処理
	void UpdateChase(float elapsed_time);

	//注目対象更新処理
	void UpdateTargetFromHate();

private:
	bool is_play = true;	//playフラグ
	std::weak_ptr<TransformComponent> target_transform;	//対象の座標
	std::shared_ptr<HateComponent> hate_component;		//ヘイト管理
	std::unique_ptr<LinearMove> linear_move;			//直線移動
	std::unique_ptr<WanderMove> wander_move;			//徘徊移動
	float chase_speed;									//移動速度
	float stop_distance;								//停止距離
};
