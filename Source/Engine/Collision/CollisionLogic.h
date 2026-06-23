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

	//カプセル同士の当たり判定
	bool IsCapsuleCapsuleCollision(const CapsuleCollider* capsule_a, const CapsuleCollider* capsule_b, CollisionResult& out_result);

private:
	//2つの線分間の最近傍点を計算
	void CalculaterClosestPointsBetweenSegments(
		const DirectX::XMVECTOR& start_a, const DirectX::XMVECTOR& end_a,
		const DirectX::XMVECTOR& start_b, const DirectX::XMVECTOR& end_b,
		DirectX::XMVECTOR& out_closest_a, DirectX::XMVECTOR& out_closest_b
	)const;
};

