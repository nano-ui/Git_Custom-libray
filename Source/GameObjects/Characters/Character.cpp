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
	gravity = 0.0f;
	height = 100.0f;
	acceleration = 50.0f;
	move_vecX = 0.0f;
	move_vecZ = 0.0f;
	friction = 15.0f;
	max_speed = 5.0f;
	air_control = 0.3f;
}

//デストラクタ
Character::~Character()
{
}

//初期化処理
void Character::Initialize()
{
	is_active = true;
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

//ImGui表示
void Character::RenderGui()
{
	ImGui::DragFloat3("Position", &position.x, 0.1f);
	ImGui::DragFloat3("Angle", &angle.x, 0.1f);
	ImGui::DragFloat3("Scale", &scale.x, 0.1f);
	ImGui::Separator();
	ImGui::DragFloat("Health", &height, 0.1f);
	ImGui::DragFloat("Gravity", &gravity, 0.1f);
	ImGui::DragFloat("Friction", &friction, 0.1f);
	ImGui::DragFloat("Acceleration", &acceleration, 0.1f);
	ImGui::DragFloat("MaxSpeed", &max_speed, 0.1f);
	ImGui::DragFloat("AttackPower", &attack_power, 0.1f);
}

//ダメージ処理
bool Character::ApplyDamage(float damage, float invincible_time)
{
	//ダメージの適用条件判定
	if (damage <= 0)return false;
	if (height <= 0)return false;
	if (invincible_timer > 0.0f)return false;

	//ダメージ適用とイベント実行
	invincible_timer = invincible_time;
	height -= damage;
	if (height <= 0)
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
