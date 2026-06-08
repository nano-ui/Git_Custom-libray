#include "CollisionSphere.h"
#include "../Collision/CollisionManager.h"
#include "../Graphics/ShapeRenderer.h"
#include <cmath>
#include "CollisionExperiment.h"

//コンストラクタ
CollisionSphere::CollisionSphere(CollisionManager* collision_manager, const DirectX::XMFLOAT3& spawn_position, float radius_size, const DirectX::XMFLOAT3& velocity)
	:manager_ptr(collision_manager)
	,move_velocity(velocity)
	,hit_color_timer(0.0f)
{
	//コライダーの初期化と生成
	sphere_collider = std::make_unique<SphereCollider>();
	sphere_collider->center = spawn_position;
	sphere_collider->old_center = spawn_position;
	sphere_collider->radius = radius_size;
	sphere_collider->attribute = ColliderAttribute::Collision;
	sphere_collider->listener = this;

	move_velocity = { 1.0f,1.0f,1.0f };

	//衝突管理システムへの登録
	if (manager_ptr != nullptr)
	{
		manager_ptr->Register(sphere_collider.get());
	}

}

//デストラクタ
CollisionSphere::~CollisionSphere()
{
	if (!manager_ptr)
	{
		manager_ptr->Remove(sphere_collider.get());
	}
}

//更新
void CollisionSphere::Update(float elapsed_time)
{
	if (hit_color_timer > 0.0f)
	{
		hit_color_timer -= elapsed_time;
		if (hit_color_timer <= 0.0f)
		{
			hit_color_timer = 0.0f;
			color = { 0.0f,0.0f,0.0f,1.0f };
		}
	}

	//前回の位置を保存
	sphere_collider->old_center = sphere_collider->center;

	//座標の移動
	sphere_collider->center.x += move_velocity.x * elapsed_time;
	sphere_collider->center.y += move_velocity.y * elapsed_time;
	sphere_collider->center.z += move_velocity.z * elapsed_time;

	//エリア境界による跳ね返り反転
	constexpr float area_limit = 60.0f;
	if (std::abs(sphere_collider->center.x) > area_limit)
	{
		move_velocity.x *= -1.0f;
	}
	if (std::abs(sphere_collider->center.y) > area_limit)
	{
		move_velocity.y *= -1.0f;
	}
	if (std::abs(sphere_collider->center.z) > area_limit)
	{
		move_velocity.z *= -1.0f;
	}
}

//描画
void CollisionSphere::Render(ShapeRenderer* renderer)
{
	if (!renderer)return;

	//描画カラーとリクエストの発行
	if(hit_color_timer > 0.0f)
	{
		color = { 1.0f,0.0f,0.0f,1.0f };
	}

	renderer->DrawSphere(sphere_collider->center, sphere_collider->radius, color);
}

//衝突イベント通知
void CollisionSphere::OnCollisionHit(const CollisionResult& result)
{
	hit_color_timer = 0.2f;
}
