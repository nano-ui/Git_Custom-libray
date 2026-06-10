#include "Character.h"

#include <imgui.h>
#include <cmath>

//コンストラクタ
Character::Character()
{
	angle = { 0.0f,0.0f,0.0f };
	velocity = { 0.0f,0.0f,0.0f };
	is_ground = false;
	invincible_timer = 0.0f;
	radius = 2.0f;
	height = 4.0f;
	gravity = -10.0f;
	height = 1.0f;
	acceleration = 50.0f;
	move_vecX = 0.0f;
	move_vecZ = 0.0f;
	friction = 15.0f;
	max_speed = 5.0f;
	air_control = 0.3f;
	offset_y = 0.0f;
	weight = 10.0f;
	health = 100.0f;
	move_speed = 0.0f;
}

//デストラクタ
Character::~Character()
{
}

//初期化処理
void Character::Initialize()
{
	is_active = true;
	SetupSerialization();
}

//更新処理
void Character::Update(float elapsed_time)
{
	UpdateInvincibleTimer(elapsed_time);
	UpdateVelocity(elapsed_time);

	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMStoreFloat4(&rotation, q);
}

//描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	DirectX::XMMATRIX world_matrix = GetWorldMatrix();
	DirectX::XMFLOAT4X4 transform_matrix;
	DirectX::XMStoreFloat4x4(&transform_matrix, world_matrix);
	character->Render(context, transform_matrix);
}

//デバッグ描画
void Character::RenderDebug(ShapeRenderer* renderer)
{

}

//変数をシリアライザに登録
void Character::SetupSerialization()
{
	GameObject::SetupSerialization();
	serializer->RegisterVariable("Heilth", &height);
	serializer->RegisterVariable("Radius", &radius);
	serializer->RegisterVariable("MaxSpeed", &max_speed);
	serializer->RegisterVariable("MoveSpeed", &move_speed);
	serializer->RegisterVariable("Health", &health);
	serializer->RegisterVariable("AttackPower", &attack_power);
	serializer->RegisterVariable("Offset_Y", &offset_y);
	serializer->RegisterVariable("Weight", &weight);
	serializer->RegisterVariable("Gravity", &gravity);
	serializer->RegisterVariable("Friction", &friction);
	serializer->RegisterVariable("Acceleration", &acceleration);
}

//ダメージ処理
bool Character::ApplyDamage(float damage, float invincible_time)
{
	//ダメージの適用条件判定
	if (damage <= 0)return false;
	if (health <= 0)return false;
	if (invincible_timer > 0.0f)return false;

	//ダメージ適用とイベント実行
	invincible_timer = invincible_time;
	health -= damage;
	if (health <= 0)
	{
		OnDead();
	}
	else
	{
		OnDamage();
	}

	return true;
}

//移動方向の設定
void Character::Move(float elapsed_time, float vx, float vz, float speed)
{
	move_vecX = vx;
	move_vecZ = vz;
	max_speed = speed;
}

//旋回処理
void Character::Tuen(float elapsed_time, float vx, float vz, float speed)
{
	//回転の計算
	float rot_speed = speed * elapsed_time;
	float length = sqrtf(vx * vx + vz * vz);
	if (length < 0.001f)return;
	vx /= length;
	vz /= length;
	float frontX = sinf(angle.y);
	float frontZ = cosf(angle.y);

	//回転量の決定と適用
	float dot = (frontX * vx) + (frontZ * vz);
	float rot = 1.0f - dot;
	if (rot > rot_speed)rot = rot_speed;
}

//ジャンプ処理
void Character::Jump(float speed)
{
	velocity.y = speed;
}

//速度と座標の更新処理
void Character::UpdateVelocity(float elapsed_time)
{
	UpdateVerticalVelocity(elapsed_time);
	UpdateHorizontalVelocity(elapsed_time);
	UpdateVerticalMove(elapsed_time);
	UpdateHorizontalMove(elapsed_time);
}

//無敵時間更新処理
void Character::UpdateInvincibleTimer(float elapsed_time)
{
	if (invincible_timer > 0.0f)
	{
		invincible_timer -= elapsed_time;
	}
}

//ステージとの衝突処理
void Character::ResolveStageCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	//押し出しベクトルの逆算
	float push_x = result.safe_position.x - collider.start_center.x;
	float push_y = result.safe_position.y - collider.start_center.y;
	float push_z = result.safe_position.z - collider.start_center.z;

	//スロープと壁・天井の自動識別
	constexpr float SLOPE_THRESHOLD = 0.5f;
	
	if (result.hit_normal.y > SLOPE_THRESHOLD)
	{
		is_ground = true;
		if (velocity.y < 0.0f)velocity.y = 0.0f;
		float penetration_depth = std::sqrtf(push_x * push_x + push_y * push_y + push_z * push_z);
		float upward_push = penetration_depth / result.hit_normal.y;
		position.y += upward_push;
	}
	else if (result.hit_normal.y < -SLOPE_THRESHOLD)
	{
		if (velocity.y > 0.0f)velocity.y = 0.0f;
		position.y += push_y;
	}
	else
	{
		position.x += push_x;
		position.z += push_z;
		float nx = result.hit_normal.x;
		float nz = result.hit_normal.z;
		float len = std::sqrtf(nx * nx + nz * nz);
		if (len > 0.001f)
		{
			nx /= len;
			nz /= len;
			float dot = velocity.x * nx + velocity.z * nz;
			if (dot < 0.0f)
			{
				velocity.x -= dot * nx;
				velocity.z -= dot * nz;
			}
		}
	}

	//コライダー一の即時上書き
	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}

//動的オブジェクトとの衝突処理
void Character::ResolveDynamicCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	//相手のコライダー情報が存在するかチェック
	if (!result.hit_collider)return;

	//互いの重さを取得
	float my_weight = collider.weight;
	float other_weight = result.hit_collider->weight;

	//押し出される割合の計算
	float push_ratio = 1.0f;
	if (my_weight > 0.0f && other_weight > 0.0f)
	{
		push_ratio = other_weight / (my_weight + other_weight);
	}
	else if (my_weight <= 0.0f && other_weight > 0.0f)
	{
		push_ratio = 0.0f;
	}
	else if (my_weight > 0.0f && other_weight <= 0.0f)
	{
		push_ratio = 1.0f;
	}
	else
	{
		push_ratio = 0.0f;
	}

	//貫通量に割合を掛けて座標を補正
	position.x += result.penetration_vector.x * push_ratio;
	position.y += result.penetration_vector.y * push_ratio;
	position.z += result.penetration_vector.z * push_ratio;

	//コライダー位置の即時上書き
	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}

//垂直方向の移動速度更新
void Character::UpdateVerticalVelocity(float elapsed_time)
{
	velocity.y += gravity * elapsed_time;
}

//垂直方向の座標更新処理
void Character::UpdateVerticalMove(float elapsed_time)
{
	position.y += velocity.y * elapsed_time;

	//接地判定処理
	if (position.y < 0.0f)
	{
		position.y = 0.0f;
		if (!is_ground)OnLanding();
		is_ground = true;
		velocity.y = 0.0f;
	}
	else
	{
		is_ground = false;
	}

}

//水平方向の速度更新処理
void Character::UpdateHorizontalVelocity(float elapsed_time)
{
	//摩擦の計算
	float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (length > 0.0f)
	{
		float current_friction = friction * elapsed_time;
		if (!is_ground)current_friction *= air_control;
		if (length > current_friction)
		{
			float vx = velocity.x / length;
			float vz = velocity.z / length;
			velocity.x -= vx * current_friction;
			velocity.z -= vz * current_friction;
		}
		else
		{
			velocity.x = 0.0f;
			velocity.z = 0.0f;
		}
	}

	//加速の計算
	if (length <= max_speed)
	{
		float move_vec_length = sqrtf(move_vecX * move_vecX + move_vecZ * move_vecZ);
		if (move_vec_length > 0.0f)
		{
			float current_acceleration = acceleration * elapsed_time;
			if (!is_ground)current_acceleration *= air_control;
			velocity.x += move_vecX * current_acceleration;
			velocity.z += move_vecZ * current_acceleration;
			float new_length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
			if (new_length > max_speed)
			{
				float vx = velocity.x / new_length;
				float vz = velocity.z / new_length;
				velocity.x = vx * max_speed;
				velocity.z = vz * max_speed;
			}
		}
	}
	//入力リセット
	move_vecX = 0.0f;
	move_vecZ = 0.0f;
}

//水平方向の座標更新処理
void Character::UpdateHorizontalMove(float elapsed_time)
{
	position.x += velocity.x * elapsed_time;
	position.z += velocity.z * elapsed_time;
}
