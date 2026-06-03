#pragma once

#include <DirectXMath.h>

#include "Collider.h"

class CollisionLogic
{
public:
	//コンストラクタ
	CollisionLogic() {};

	//カプセルとスフィアの当たり判定
	bool IsCapsuleSphereCollision(const DirectX::XMFLOAT3& capsule_start, const DirectX::XMFLOAT3& capsule_end, float capsule_radius, const DirectX::XMFLOAT3& sphere_center, float sphere_radius);

	//スフィア同士の当たり判定
	bool IsSphereSphereCollision(const SphereCollider* sphere_a, const SphereCollider* sphere_b, CollisionResult& out_result);
};

