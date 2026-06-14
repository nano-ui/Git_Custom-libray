#pragma once
#include "Character.h"
#include "../Collision/Collider.h"

#include <memory>
#include <string>
#include <unordered_map>

class StateMachine;

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

	//変数をシリアライザに登録
	void SetupSerialization() override;

	//当たり判定情報の取得
	CapsuleCollider* GetCapsuleCollider() { return &capsule_collider; }

	//衝突処理
	void OnCollisionHit(const CollisionResult& result)override;

private:
	//入力更新処理
	void UpdateInput(float elapsed_time);

private:
	CapsuleCollider capsule_collider;	//カプセルの当たり判定
};

