#pragma once

#include "Gameplay\GameObjects\Character\Character.h"
#include "Engine/Collision/Collider.h"

#include <memory>

class CollisionSphere;
class CapsuleColliderComponent;

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

private:
	//コライダー更新処理
	void UpdateCollider();

private:
	std::shared_ptr<CapsuleColliderComponent> collider_component;	//カプセルコライダーコンポーネント
	float collider_radius = 1.0f;	//コライダーの半径
	DirectX::XMFLOAT3 collider_offset = { 0.0f,1.0f,0.0f };	//コライダーのオフセット
};

