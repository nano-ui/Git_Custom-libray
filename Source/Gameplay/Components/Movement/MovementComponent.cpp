#include "MovementComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

#include <Windows.h>
#include <cmath>
#include <algorithm>

//コンストラクタ
MovementComponent::MovementComponent()
	:velocity({ 0.0f,0.0f,0.0f })
	, move_input({ 0.0f,0.0f })
	, move_speed(0.0f)
	, max_speed(5.0f)
	, acceleration(50.0f)
	, friction(15.0f)
	, gravity(-10.0f)
	, air_control(0.3f)
	, is_graund(false)
{
	SetComponentName(u8"移動コンポーネント");
}

//デストラクタ
MovementComponent::~MovementComponent() = default;

//初期化処理
void MovementComponent::Initialize()
{
	Component::Initialize();
	if (target_transform.expired())
	{
		OutputDebugStringA("[MovementComponent 警告] target_transform が未設定です。SetTransformComponent を実行してください。\n");
	}
}

//更新処理
void MovementComponent::Update(float elapsed_time)
{
	if (!is_active)return;
	UpdateVelocity(elapsed_time);
	move_speed = std::sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
}

//ImGuiデバッグ描画処理
void MovementComponent::RenderGui()
{
	if (!is_active)return;
}

//JSON保存変数登録
void MovementComponent::SetupSerialization(JsonSerializer* serializer)
{
	if (!serializer)return;
	serializer->RegisterVariable(u8"最高速度", &max_speed);
	serializer->RegisterVariable(u8"加速度", &acceleration);
	serializer->RegisterVariable(u8"摩擦力", &friction);
	serializer->RegisterVariable(u8"重力", &gravity);
	serializer->RegisterVariable(u8"空中制御力", &air_control);
}

//inspector登録
void MovementComponent::SetupInspector(GuiInspector* inspector)
{
	if (!inspector)return;
	inspector->RegisterVariable(u8"最高速度", &max_speed, GetComponentName());
	inspector->RegisterVariable(u8"加速度", &acceleration, GetComponentName());
	inspector->RegisterVariable(u8"摩擦力", &friction, GetComponentName());
	inspector->RegisterVariable(u8"重力", &gravity, GetComponentName());
	inspector->RegisterVariable(u8"空中制御力", &air_control, GetComponentName());

	inspector->RegisterText(u8"移動速度ベクトル", &velocity.x, GetComponentName());
	inspector->RegisterText(u8"現在の移動速度", &move_speed, GetComponentName());
	inspector->RegisterText(u8"接地フラグ", &is_graund, GetComponentName());
}

//対象のTeansformComponentを指定
void MovementComponent::SetTransformComponent(const std::shared_ptr<TransformComponent>& transform)
{
	target_transform = transform;
}

//移動方向と速度の入力設定
void MovementComponent::Move(float vx, float vz, float speed)
{
	move_input.x = vx;
	move_input.y = vz;
	max_speed = speed;
}

//移動方向への旋回処理
void MovementComponent::Turn(float elapsed_time, float vx, float vz, float speed)
{
	auto transform = target_transform.lock();
	if (!transform)return;

	constexpr float min_input_length = 0.001f;
	float length = std::sqrtf(vx * vx + vz * vz);
	if (length < min_input_length)return;

	float target_angle = std::atan2(vx, vz);
	DirectX::XMFLOAT3 current_rot = transform->GetRotation();
	float angle_diff = target_angle - current_rot.y;

	while (angle_diff > DirectX::XM_PI)angle_diff -= DirectX::XM_2PI;
	while (angle_diff > -DirectX::XM_PI)angle_diff += DirectX::XM_2PI;

	float rot_speed = speed * elapsed_time;

	if (std::abs(angle_diff) <= rot_speed)current_rot.y = target_angle;
	else current_rot.y += (angle_diff > 0.0f ? rot_speed : -rot_speed);

	while (current_rot.y > DirectX::XM_PI)current_rot.y -= DirectX::XM_2PI;
	while (current_rot.y > -DirectX::XM_PI)current_rot.y += DirectX::XM_2PI;

	if (std::isnan(current_rot.y))
	{
		OutputDebugStringA("[MovementComponent エラー] Turn: current_rot.y に NaN が検出されたため 0.0 に補正しました。\n");
		current_rot.y = 0.0f;
	}

	transform->SetRotation(current_rot);
}

//ジャンプ処理
void MovementComponent::Jump(float speed)
{
	velocity.y = speed;
}

//速度・位置更新
void MovementComponent::UpdateVelocity(float elapsed_time)
{
	UpdateVerticalVelocity(elapsed_time);
	UpdateHorizontalVelocity(elapsed_time);
	UpdateVericalMove(elapsed_time);
	UpdateHorizontalMove(elapsed_time);
}

void MovementComponent::UpdateVerticalVelocity(float elapsed_time)
{
	velocity.y += gravity * elapsed_time;
}

void MovementComponent::UpdateVericalMove(float elapsed_time)
{
	auto transform = target_transform.lock();
	if (!transform)return;

	DirectX::XMFLOAT3 pos = transform->GetPosition();
	pos.y += velocity.y * elapsed_time;

	if (pos.y < 0.0f)
	{
		pos.y = 0.0f;
		is_graund = true;
		velocity.y = 0.0f;
	}
	else is_graund = false;

	transform->SetPosition(pos);
}

void MovementComponent::UpdateHorizontalVelocity(float elapsed_time)
{
	//摩擦の計算
	float length = std::sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (length > 0.0f)
	{
		float current_friction = friction * elapsed_time;
		if (!is_graund)current_friction *= air_control;

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
		float move_vec_length = std::sqrtf((move_input.x * move_input.x) + (move_input.y * move_input.y));
		if (move_vec_length > 0.0f)
		{
			float current_acceleration = acceleration * elapsed_time;
			if (!is_graund)current_acceleration *= air_control;
			velocity.x += move_input.x * current_acceleration;
			velocity.z += move_input.y * current_acceleration;
			float new_length = std::sqrtf((velocity.x * velocity.x) + (velocity.z * velocity.z));
			if (new_length > max_speed)
			{
				float vx = velocity.x / new_length;
				float vz = velocity.z / new_length;
				velocity.x = vx * max_speed;
				velocity.z = vz * max_speed;
			}
		}
	}
	//入力のリセット
	move_input.x = 0.0f;
	move_input.y = 0.0f;
}

void MovementComponent::UpdateHorizontalMove(float elapsed_time)
{
	auto transform = target_transform.lock();
	if (!transform)return;
	DirectX::XMFLOAT3 pos = transform->GetPosition();
	pos.x += velocity.x * elapsed_time;
	pos.z += velocity.z * elapsed_time;
	transform->SetPosition(pos);
}
