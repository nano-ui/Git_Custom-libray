#include "Character.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "../Gameplay/Components/StateMachineComponent.h"
#include "Gameplay/Components/Movement/CharacterMovementComponent.h"
#include "Serialization/JsonSerializer.h"

#include <imgui.h>
#include <cmath>

// コンストラクタ
Character::Character()
{
	angle = { 0.0f,0.0f,0.0f };
	is_ground = false;
	invincible_timer = 0.0f;
	radius = 2.0f;
	height = 1.0f;
	offset_y = 0.0f;
	weight = 10.0f;
	health = 100.0f;

	blackboard = std::make_unique<StateBlackboard>();
	state_machine_component = std::make_unique<StateMachineComponent>();
	animation_component = nullptr;
	root_motion_component = std::make_unique<RootMotionComponent>();
	movement_component = std::make_unique<CharacterMovementComponent>();
}

// デストラクタ
Character::~Character()
{
}

// 初期化処理
void Character::Initialize()
{
	is_active = true;
	SetupSerialization();
	SetupBlackboard();

	if (root_motion_component && character)
	{
		root_motion_component->Initialize(character->GetGltfModelData());
	}
}

// 更新処理
void Character::Update(float elapsed_time)
{
	UpdateInvincibleTimer(elapsed_time);

	if (movement_component)
	{
		movement_component->SetGround(is_ground);
	}

	if (root_motion_component && movement_component)
	{
		if (!root_motion_component->IsEnable())
		{
			movement_component->Update(elapsed_time, position, angle);
		}
	}

	if (movement_component)
	{
		is_ground = movement_component->IsGround();
	}

	DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMStoreFloat4(&rotation, q);

	blackboard->SetValue(u8"体力", health);
	if (movement_component)
	{
		blackboard->SetValue(u8"速度", movement_component->GetMoveSpeed());
	}
	blackboard->SetValue(u8"接地フラグ", is_ground);

	if (state_machine_component)
	{
		state_machine_component->Update(elapsed_time, blackboard.get());
	}

	if (state_machine_component && animation_component)
	{
		std::string target_anim_name = state_machine_component->GetCurrentAnimationName();
		uint32_t current_state_id = state_machine_component->GetCurrentNodeId();
		bool target_anim_loop = state_machine_component->GetAnimationLoop();

		animation_component->PlayAnimationByName(target_anim_name, current_state_id, target_anim_loop);
	}

	if (animation_component)
	{
		animation_component->Update(elapsed_time);
		UpdateRootMotion();
		root_motion_component->TraceRootMotionDebug(position);
	}
}

// 描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	DirectX::XMMATRIX world_matrix = GetWorldMatrix();
	DirectX::XMFLOAT4X4 transform_matrix;
	DirectX::XMStoreFloat4x4(&transform_matrix, world_matrix);
	character->Render(context, transform_matrix);
}

// デバッグ描画
void Character::RenderDebug(ShapeRenderer* renderer)
{
}

// をシリアライザに登録
void Character::SetupSerialization()
{
	GameObject::SetupSerialization();

	if (serializer)
	{
		serializer->RegisterVariable(u8"体力", &health, u8"基本ステータス");
		serializer->RegisterVariable(u8"攻撃力", &attack_power, u8"基本ステータス");

		serializer->RegisterVariable(u8"重量", &weight, u8"移動・物理パラメータ");

		serializer->RegisterVariable(u8"コライダー半径", &radius, u8"当たり判定設定");
		serializer->RegisterVariable(u8"コライダー高さ", &height, u8"当たり判定設定");
		serializer->RegisterVariable(u8"コライダーY軸オフセット", &offset_y, u8"当たり判定設定");

		if (movement_component)
		{
			movement_component->SetupSerialization(serializer.get());
		}
	}
}

// ダメージ処理
bool Character::ApplyDamage(float damage, float invincible_time)
{
	if (damage <= 0)
	{
		return false;
	}
	if (health <= 0)
	{
		return false;
	}
	if (invincible_timer > 0.0f)
	{
		return false;
	}

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

// ブラックボードに登録・初期化
void Character::SetupBlackboard()
{
	blackboard->RegisterVariable(u8"体力");
	blackboard->RegisterVariable(u8"速度");
	blackboard->RegisterVariable(u8"接地フラグ");

	blackboard->SetValue(u8"体力", health);
	if (movement_component)
	{
		blackboard->SetValue(u8"速度", movement_component->GetMaxSpeed());
	}
	blackboard->SetValue(u8"接地フラグ", is_ground);
}

// アニメーション終了イベント
void Character::OnAnimationEnd(uint32_t state_key)
{
}

// 移動方向の設定
void Character::Move(float elapsed_time, float vx, float vz, float speed)
{
	if (movement_component)
	{
		DirectX::XMFLOAT3 move_dir = { vx, 0.0f, vz };
		movement_component->SetMoveVec(move_dir);
		movement_component->SetMaxSpeed(speed);
	}
}

// ジャンプ処理
void Character::Jump(float speed)
{
	if (movement_component)
	{
		movement_component->Jump(speed);
	}
}

// 無敵時間更新処理
void Character::UpdateInvincibleTimer(float elapsed_time)
{
	if (invincible_timer > 0.0f)
	{
		invincible_timer -= elapsed_time;
	}
}

// ルートモーション更新
void Character::UpdateRootMotion()
{
	if (!root_motion_component || !animation_component || !character)
	{
		return;
	}

	std::string curreent_anim_name = animation_component->GetCurrentAnimationName();
	float current_time = animation_component->GetCurrentAnimationTime();

	if (previous_animation_name != curreent_anim_name)
	{
		int anim_index = character->GetAnimationIndex(curreent_anim_name.c_str());

		if (anim_index >= 0)
		{
			root_motion_component->OnAnimationChaanged(static_cast<size_t>(anim_index));
		}
		else
		{
			OutputDebugStringA("[Character Error] UpdateRootMotion: Animation index not found!\n");
		}
		previous_animation_name = curreent_anim_name;
		previous_animation_time = 0.0f;
	}

	bool is_rm_enabled = state_machine_component ? state_machine_component->IsCurrentRootMotionEnbled() : false;
	root_motion_component->SetEnable(is_rm_enabled);

	if (!root_motion_component->IsEnable())
	{
		return;
	}

	root_motion_component->Update(current_time);

	DirectX::XMFLOAT3 delta_pos = root_motion_component->GetDeltaPosition();
	DirectX::XMFLOAT4 delta_rot = root_motion_component->GetDeltaRotation();

	DirectX::XMVECTOR local_translation = DirectX::XMLoadFloat3(&delta_pos);
	DirectX::XMVECTOR local_rotation = DirectX::XMLoadFloat4(&delta_rot);

	int root_index = root_motion_component->GetTargetNodeIndex();

	DirectX::XMMATRIX root_local_rotation_matrix = DirectX::XMMatrixIdentity();

	if (root_index >= 0)
	{
		const auto& animated_nodes = character->GetAnimatedNodes();
		if (static_cast<size_t>(root_index) < animated_nodes.size())
		{
			DirectX::XMVECTOR root_rot = DirectX::XMLoadFloat4(&animated_nodes.at(root_index).rotation);
			root_local_rotation_matrix = DirectX::XMMatrixRotationQuaternion(root_rot);
		}
	}

	DirectX::XMVECTOR corrected_local_translation = DirectX::XMVector3TransformNormal(local_translation, root_local_rotation_matrix);

	DirectX::XMMATRIX world_transform = GetWorldMatrix();
	DirectX::XMVECTOR world_translation = DirectX::XMVector3TransformNormal(corrected_local_translation, world_transform);

	world_translation = DirectX::XMVectorSetY(world_translation, 0.0f);

	DirectX::XMFLOAT3 final_movement;
	DirectX::XMFLOAT3 final_movement_temp;
	DirectX::XMStoreFloat3(&final_movement_temp, world_translation);

	final_movement.x = final_movement_temp.x;
	final_movement.y = final_movement_temp.y;
	final_movement.z = final_movement_temp.z;

	position.x += final_movement.x;
	position.y += final_movement.y;
	position.z += final_movement.z;

	if (root_index >= 0)
	{
		const std::vector<GltfModelData::node>& animated_nodes = character->GetAnimatedNodes();

		if (static_cast<size_t>(root_index) < animated_nodes.size())
		{
			DirectX::XMFLOAT3 initial_pose_pos = root_motion_component->GetInitialLocalPosition();
			DirectX::XMFLOAT3 current_local_pos = animated_nodes.at(root_index).translation;

			DirectX::XMFLOAT3 new_local_pos = current_local_pos;

			new_local_pos.x = initial_pose_pos.x;
			new_local_pos.y = initial_pose_pos.y;
			new_local_pos.z = initial_pose_pos.z;

			character->SetNodeTranslation(root_index, new_local_pos);
			character->RecalculateTransforms();
		}
		else
		{
			OutputDebugStringA("[Character Error] UpdateRootMotion: root_index is out of range of animated_nodes!\n");
		}
	}
	previous_animation_time = current_time;
}

// ステージとの衝突処理
void Character::ResolveStageCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	float push_x = result.safe_position.x - collider.start_center.x;
	float push_y = result.safe_position.y - collider.start_center.y;
	float push_z = result.safe_position.z - collider.start_center.z;

	constexpr float SLOPE_THRESHOLD = 0.5f;

	if (result.hit_normal.y > SLOPE_THRESHOLD)
	{
		is_ground = true;
		if (movement_component)
		{
			DirectX::XMFLOAT3 v = movement_component->GetVelocity();
			if (v.y < 0.0f)
			{
				v.y = 0.0f;
				movement_component->SetVelocity(v);
			}
		}
		float penetration_depth = std::sqrtf(push_x * push_x + push_y * push_y + push_z * push_z);
		float upward_push = penetration_depth / result.hit_normal.y;
		position.y += upward_push;
	}
	else if (result.hit_normal.y < -SLOPE_THRESHOLD)
	{
		if (movement_component)
		{
			DirectX::XMFLOAT3 v = movement_component->GetVelocity();
			if (v.y > 0.0f)
			{
				v.y = 0.0f;
				movement_component->SetVelocity(v);
			}
		}
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
			if (movement_component)
			{
				DirectX::XMFLOAT3 v = movement_component->GetVelocity();
				float dot = v.x * nx + v.z * nz;
				if (dot < 0.0f)
				{
					v.x -= dot * nx;
					v.z -= dot * nz;
					movement_component->SetVelocity(v);
				}
			}
		}
	}

	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}

// 動的オブジェクトとの衝突処理
void Character::ResolveDynamicCollision(
	const CollisionResult& result,
	CapsuleCollider& collider,
	float cap_height,
	float offset_y)
{
	if (!result.hit_collider)
	{
		return;
	}

	float my_weight = collider.weight;
	float other_weight = result.hit_collider->weight;

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

	position.x += result.penetration_vector.x * push_ratio;
	position.y += result.penetration_vector.y * push_ratio;
	position.z += result.penetration_vector.z * push_ratio;

	collider.start_center = position;
	collider.start_center.y += offset_y;
	collider.end_center = position;
	collider.end_center.y += cap_height + offset_y;
}