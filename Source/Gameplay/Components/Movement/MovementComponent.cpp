#include "MovementComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

#include <Windows.h>
#include <cmath>
#include <algorithm>

static constexpr float SLOPE_THRESHOLD = 0.5f;				//床・坂道と判定する法線Yの閾値
static constexpr float CEILING_THRESHOLD = -0.5f;			//天井と判定する法線Yの閾値
static constexpr float MIN_NORMAL_LENGTH_SQ = 0.0001f;		//法線ベクトルの最小二乗長
static constexpr float PENETRATION_LIMIT = 5.0f;			//1フレームあたりの最大押し出し制限値

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

//ステージ（静的メッシュ）との衝突押し出し処理
void MovementComponent::ResolveStageCollision(const CollisionResult& result, Collider* my_collider)
{
	auto transform = target_transform.lock();
	if (!transform)return;

	DirectX::XMFLOAT3 current_pos = transform->GetPosition();
	DirectX::XMFLOAT3 prev_pos = current_pos;

	//貫入ベクトルの取得
	DirectX::XMFLOAT3 push = result.penetration_vector;

	//safe_positionが有効な時はそちらの差分を優先
	if (my_collider)
	{
		float diff_x = result.safe_position.x - current_pos.x;
		float diff_y = result.safe_position.y - current_pos.y;
		float diff_z = result.safe_position.z - current_pos.z;
		if (std::abs(diff_x) > 0.0001f || std::abs(diff_y) > 0.0001f || std::abs(diff_z) > 0.0001f)
		{
			push = { diff_x,diff_y,diff_z };
		}
	}

	//法線に基づいた床・天井・壁の判定と座標補正
	if (result.hit_normal.y > SLOPE_THRESHOLD)
	{
		//床・坂道への接地
		is_graund = true;
		if (velocity.y < 0.0f)velocity.y = 0.0f;

		//坂道での法線補正
		float penetration_depth = std::sqrtf((push.x * push.x) + (push.y * push.y) + (push.z * push.z));
		float upward_push = penetration_depth / result.hit_normal.y;
		current_pos.y += (upward_push > PENETRATION_LIMIT) ? PENETRATION_LIMIT : upward_push;
	}
	else if (result.hit_normal.y < CEILING_THRESHOLD)
	{
		//天井への接触
		if (velocity.y > 0.0f)velocity.y = 0.0f;
		current_pos.y += push.y;
	}
	else
	{
		//壁との接触
		current_pos.x += push.x;
		current_pos.z += push.z;

		float nx = result.hit_normal.x;
		float nz = result.hit_normal.z;
		float normal_len_sq = nx * nx + nz * nz;

		if (normal_len_sq > MIN_NORMAL_LENGTH_SQ)
		{
			float inv_len = 1.0f / std::sqrtf(normal_len_sq);
			nx *= inv_len;
			nz *= inv_len;

			//壁方向へ向かう速度成分の打ち消し
			float dot = velocity.x * nx + velocity.z * nz;
			if (dot < 0.0f)
			{
				velocity.x -= dot * nx;
				velocity.z -= dot * nz;
			}
		}
	}
	//補正座標の適用
	transform->SetPosition(current_pos);

	//コライダーの中心座標の同期
	if (my_collider)
	{
		DirectX::XMFLOAT3 move_delta = {
			current_pos.x - prev_pos.x,
			current_pos.y - prev_pos.y,
			current_pos.z - prev_pos.z
		};
		SyncColliderPosition(my_collider, current_pos, move_delta);
	}
}

//動的オブジェクトとの衝突押し出し
void MovementComponent::ResolveDynamicCollision(const CollisionResult& result, Collider* my_collider)
{
	if (!my_collider || !result.hit_collider)return;

	auto transform = target_transform.lock();
	if (!transform)return;

	//双方の重さから押し出し比率を算出
	float my_weight = my_collider->weight;
	float other_weight = result.hit_collider->weight;
	float push_ratio = 1.0f;

	if (my_weight > 0.0f && other_weight > 0.0f)push_ratio = other_weight / (my_weight + other_weight);
	else if (my_weight <= 0.0f && other_weight > 0.0f)push_ratio = 0.0f;
	else if (my_weight > 0.0f && other_weight <= 0.0f)push_ratio = 1.0f;
	else push_ratio = 0.5f;

	//貫入ベクトルに重量比率を乗算して位置を補正
	DirectX::XMFLOAT3 current_pos = transform->GetPosition();
	DirectX::XMFLOAT3 prev_pos = current_pos;

	DirectX::XMFLOAT3 push_delta = {
		result.penetration_vector.x * push_ratio,
		result.penetration_vector.y * push_ratio,
		result.penetration_vector.z * push_ratio
	};

	current_pos.x += push_delta.x;
	current_pos.y += push_delta.y;
	current_pos.z += push_delta.z;

	transform->SetPosition(current_pos);

	//補正後のコライダー座標の同期
	SyncColliderPosition(my_collider, current_pos, push_delta);
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

//コライダーの形状ごとに現在座標を同期
void MovementComponent::SyncColliderPosition(Collider* collider, const DirectX::XMFLOAT3& new_pos, const DirectX::XMFLOAT3& move_delta)
{
	if (!collider)return;

	switch (collider->type)
	{
	case ColliderType::Sphere:
	{
		auto* sphere = static_cast<SphereCollider*>(collider);
		sphere->center.x += move_delta.x;
		sphere->center.y += move_delta.y;
		sphere->center.z += move_delta.z;
		break;
	}
	case ColliderType::Capsule:
	{
		auto* capsule = static_cast<CapsuleCollider*>(collider);
		capsule->start_center.x += move_delta.x;
		capsule->start_center.y += move_delta.y;
		capsule->start_center.z += move_delta.z;
		capsule->end_center.x += move_delta.x;
		capsule->end_center.y += move_delta.y;
		capsule->end_center.z += move_delta.z;
		break;
	}
	case ColliderType::Box:
	{
		auto* box = static_cast<BoxCollider*>(collider);
		box->box_min.x += move_delta.x;
		box->box_min.y += move_delta.y;
		box->box_min.z += move_delta.z;
		box->box_max.x += move_delta.x;
		box->box_max.y += move_delta.y;
		box->box_max.z += move_delta.z;
		break;
	}
	case ColliderType::Cylinder:
	{
		auto* cylinder = static_cast<CylinderCollider*>(collider);
		cylinder->center.x += move_delta.x;
		cylinder->center.y += move_delta.y;
		cylinder->center.z += move_delta.z;
		break;
	}
	case ColliderType::OBB:
	{
		auto* obb = static_cast<OBB*>(collider);
		obb->center.x += move_delta.x;
		obb->center.y += move_delta.y;
		obb->center.z += move_delta.z;
		break;
	}
	default:
		break;
	}
}
