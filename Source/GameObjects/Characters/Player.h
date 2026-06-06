#pragma once
#include "Character.h"
#include "../Collision/Collider.h"

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

	//影の書き込みパス専用の描画処理
	void RenderCaster(ID3D11DeviceContext* context) override;

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override;

	//ImGuiデバッグ描画
	void RenderGui()override;

	//当たり判定情報の取得
	CapsuleCollider* GetCapsuleCollider() { return &capsule_collider; }

	//衝突処理
	void OnCollisionHit(const CollisionResult& result)override;

private:
	//入力更新処理
	void UpdateInput(float elapsed_time);

private:
	float move_speed;					//移動速度
	CapsuleCollider capsule_collider;	//カプセルの当たり判定
};

