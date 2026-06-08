#pragma once

#include <DirectXMath.h>
#include <memory>

#include "../Collision/Collider.h"

struct SphereCollider;
class ShapeRenderer;
class CollisionManager;

class CollisionSphere : public ICollisionListener
{
public:
	//コンストラクタ
	CollisionSphere(CollisionManager* collision_manager, const DirectX::XMFLOAT3& spawn_position, float radius_size, const DirectX::XMFLOAT3& velocity);

	//デストラクタ
	~CollisionSphere();

	//更新
	void Update(float elapsed_time);

	//描画
	void Render(ShapeRenderer* renderer);

	//衝突イベント通知
	void OnCollisionHit(const CollisionResult& result) override;

private:
	CollisionManager* manager_ptr;						//衝突管理クラスへの生ポインタ
	std::unique_ptr<SphereCollider> sphere_collider;	//スフィアコライダークラス
	DirectX::XMFLOAT3 move_velocity;					//移動速度ベクトル
	DirectX::XMFLOAT4 color;
	float hit_color_timer = 0.0f;
};

