#pragma once

#include "Gameplay\GameObjects\Character\Character.h"
#include "Engine/Collision/Collider.h"

#include <memory>

class CollisionSphere;
class CapsuleColliderComponent;
class BoneCapsuleColliderComponent;

class Enemy :public Character, public ICollisionListener
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

	//衝突コールバック処理
	void OnCollisionHit(const CollisionResult& result)override;

private:
	//コンポーネント群のセットアップ
	void SetupComponent();

	//部位別ボーン追従コライダーのセットアップ
	void SetupColliders();

	//コライダー更新処理
	void UpdateCollider();

private:
	std::shared_ptr<BoneCapsuleColliderComponent> head_collider_component;	//頭部用ボーン追従カプセルコライダー
	std::shared_ptr<BoneCapsuleColliderComponent> body_collider_component;	//胴体用ボーン追従カプセルコライダー

	// 初期設定用定数
	static constexpr float head_collider_radius = 0.35f; // 頭部コライダー半径
	static constexpr float head_collider_height = 0.2f;  // 頭部コライダー高さ
	static constexpr float body_collider_radius = 0.5f;  // 胴体コライダー半径
};

