#pragma once
#include "Character.h"
#include "Engine/Collision/Collider.h"

class CapsuleColliderComponent;

class Player : public Character, public ICollisionListener
{
public:
	//コンストラクタ
	Player();

	//デストラクタ
	~Player();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//をシリアライザに登録
	void SetupSerialization() override;

	// コライダーコンポーネント取得
	CapsuleColliderComponent* GetCapsuleColliderComponent() const { return collider_component.get(); }

	//衝突処理
	void OnCollisionHit(const CollisionResult& result)override;

	//アニメーション終了イベント
	void OnAnimationEnd(uint32_t state_key)override;

private:
	//入力更新処理
	void UpdateInput(float elapsed_time);

private:
	std::shared_ptr<CapsuleColliderComponent> collider_component;	//カプセルコライダーコンポーネント
};

