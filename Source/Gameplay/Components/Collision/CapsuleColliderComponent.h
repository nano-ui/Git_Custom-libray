#pragma once

#include "Gameplay\Components\Collision\ColliderComponent.h"
#include "Engine\Collision\Collider.h"

class CapsuleColliderComponent :public ColliderComponent
{
public:
	//コンストラクタ
	CapsuleColliderComponent();

	//デストラクタ
	virtual ~CapsuleColliderComponent() = default;

	//シリアライズ登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//インスペクター登録
	void SetupInspector(GuiInspector* inspector)override;

	//コライダーの生ポインタ取得
	Collider* GetRawCollider()override { return &capsule_collider; }

	//半径の取得
	float GetRadius()const { return height; }

	//半径の設定
	void SetRadius(float r);

	//高さの取得
	float GetHeight()const { return height; }

	//高さの設定
	void SetHeight(float h) { height = h; }

	//カプセル固有コライダーの取得
	CapsuleCollider* GetCapsuleCollider() { return &capsule_collider; }

protected:
	//座標の同期処理
	void UpdateColliderTransform(const DirectX::XMFLOAT3& world_pos, const DirectX::XMFLOAT4& world_rot) override;

protected:
	CapsuleCollider capsule_collider;	//コライダーの実体
	float radius;						//カプセルの半径
	float height;						//カプセルの高さ
};

