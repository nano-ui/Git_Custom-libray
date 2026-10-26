#include "Character.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay\Components\Editor\AttachmentColliderComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModel.h"

#include "Engine\Core\Log\Log.h"

#include <imgui.h>
#include <cmath>
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
Character::Character()
	: health(100.0f)
	, attack_power(10.0f)
	, invincible_timer(1.0f)
{
	blackboard = std::make_unique<StateBlackboard>();
	state_machine_component = std::make_unique<StateMachineComponent>();
	root_motion_component = std::make_unique<RootMotionComponent>();
}

//デストラクタ
Character::~Character() = default;

//初期化処理
void Character::Initialize()
{
	is_active = true;

	SetupComponent();

	//モデルパスの確認
	if (model_component->GetModelPath().empty())
	{
		const std::string default_path = GetDefaultModelPath();
		if (!default_path.empty())
		{
			model_component->LoadModel(default_path);
		}
		else
		{
			LOG_ERROR({ "Character", "Model" }, u8"Initialize: デフォルトモデルパスが設定されていません。");
		}
	}

	if (!model_component->GetModel())
	{
		LOG_ERROR({ "Character", "Model" }, u8"Initialize: モデルデータのロードに失敗したため、後続の初期化を中止します。");
		return;
	}

	//全コンポーネントの初期化実行
	GameObject::Initialize();

	SetupBlackboard();

	if (model_component->GetModel())
	{
		std::filesystem::path path_obj(model_component->GetModelPath());
		std::string model_name = path_obj.stem().string();

		model_component->SetAutoAnimationUpdate(false);

		sequencer_component = std::make_unique<AnimationSequencerComponent>(model_component);
		sequencer_component->Initialize(model_component->GetModelPath());

		if (root_motion_component && model_component->GetModelData())
		{
			root_motion_component->Initialize(model_component->GetModelData());
		}

		//アタッチメントコライダーコンポーネントの生成と初期化
		if (attachment_collider_component)
		{
			attachment_collider_component->Initializa(
				model_component->GetModelPath(),
				model_component,
				transform_component
			);

			//生成されたコライダーをGameObjectのコライダー配列に登録
			for (const auto& capsule : attachment_collider_component->GetRuntimeCollider())
			{
				if (capsule)
				{
					capsule->listener = this;
					capsule->owner_id = object_id;
					AddCollider(capsule.get());
				}
			}
		}

		if (state_machine_component)
		{
			state_machine_component->SetModelName(model_name);
		}
	}

	if (state_machine_component && blackboard)
	{
		state_machine_component->Initialize(blackboard.get());
	}

	SetupSerialization();
	SetupInspector();

	DirectX::XMFLOAT3 pos = transform_component->GetPosition();
}

//更新処理
void Character::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();
	UpdateInvincibleTimer(elapsed_time);

	// GameObject::Update で MovementComponent などの全コンポーネントが更新される
	GameObject::Update(elapsed_time);

	float current_move_speed = movement_component ? movement_component->GetMoveSpeed() : 0.0f;
	bool current_is_ground = movement_component ? movement_component->IsGround() : false;

	// ブラックボードへ最新情報を同期
	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"接地フラグ", current_is_ground);
	blackboard->SetValue("move_speed", current_move_speed);

	bool is_anim_finished = sequencer_component ? sequencer_component->IsAnimationFinished() : false;

	if (state_machine_component)
	{
		state_machine_component->Update(elapsed_time, blackboard.get(), is_anim_finished);

		std::string target_anim_name = state_machine_component->GetCurrentAnimationName();
		if (!target_anim_name.empty() && previous_animation_name != target_anim_name)
		{
			belnd_time = state_machine_component->GetCurrentBlendTime();
			constexpr float min_blend_time = 0.0f;
			if (belnd_time < min_blend_time)
			{
				LOG_WARNING({ "Character", "Animation" }, u8"Update: 取得したブレンド時間が負の値(%.2f)のため、0.0fに補正しました。", belnd_time);
				belnd_time = min_blend_time;
			}

			if (sequencer_component)
			{
				sequencer_component->ChangeAnimation(target_anim_name, belnd_time);
			}
			previous_animation_name = target_anim_name;
		}
	}

	// ルートモーション判定
	bool is_rm_enabled = state_machine_component ? state_machine_component->IsCurrentRootMotionEnbled() : false;
	debug_root_motion_enabled = is_rm_enabled;

	if (root_motion_component)
	{
		root_motion_component->SetEnable(is_rm_enabled);
		if (is_rm_enabled)
		{
			UpdateRootMotion();
		}
		else
		{
			debug_root_motion_delta = { 0.0f, 0.0f, 0.0f };
		}
	}

	// アニメーションシーケンサの更新と再生時間の同期
	if (sequencer_component)
	{
		constexpr bool is_loop = true;
		sequencer_component->Update(elapsed_time, is_loop);
		current_animation_time = sequencer_component->GetCurrentSequenceTime();
	}
	else
	{
		OutputDebugStringA("[Character 警告] Update: sequencer_component が nullptr です。\n");
	}

	// アニメーション再生時刻に応じた当たり判定の有効状態を更新
	UpdateColliderSequencer();
}

//衝突処理
void Character::OnCollisionHit(const CollisionResult& result)
{
	BS_PROFILE_FUNCTION();

	if (!movement_component)
	{
		LOG_WARNING({ "Character", "Collision" }, u8"OnCollisionHit: movement_component が存在しないため衝突解決をスキップします。");
		return;
	}

	//衝突した自身のコライダー実体を取得
	Collider* self_collider = result.self_collider;
	if (!self_collider)
	{
		LOG_WARNING({ "Character", "Collision" }, u8"OnCollisionHit: self_collider が nullptr のため衝突処理を中断します。");
		return;
	}

	//地形との当たり判定
	if ((result.hit_attribute & ColliderAttribute::Stage) != ColliderAttribute::None)
	{
		movement_component->ResolveStageCollision(result, self_collider);
	}
	// 動的オブジェクト衝突（他キャラなど）
	else if ((result.hit_attribute & ColliderAttribute::Collision) != ColliderAttribute::None)
	{
		movement_component->ResolveDynamicCollision(result, self_collider);
	}
	// 攻撃判定の被弾処理
	else if ((result.hit_attribute & ColliderAttribute::Attack) != ColliderAttribute::None)
	{
		if (result.hit_collider)
		{
			ApplyDamage(attack_power, invincible_timer, result.hit_collider->owner_id);
			LOG_NORMAL({ "Character", "Combat" }, u8"攻撃判定がヒットしました。攻撃者ID: %u", result.hit_collider->owner_id);
		}
		else
		{
			LOG_WARNING({ "Character", "Collision" }, u8"OnCollisionHit: hit_collider が nullptr のため被弾処理を中断しました。");
		}
	}
}

//描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	BS_PROFILE_FUNCTION();

	GameObject::Render(context);
}

//デバッグ描画
void Character::RenderDebug(ShapeRenderer* renderer)
{
	BS_PROFILE_FUNCTION();
	if (attachment_collider_component)
	{
		attachment_collider_component->RenderDebug(renderer);
	}
}

//シリアライザに登録
void Character::SetupSerialization()
{
	BS_PROFILE_FUNCTION();

	GameObject::SetupSerialization();

	if (serializer)
	{
		serializer->RegisterVariable(u8"体力", &health, SerializeTarget::Detail);
		serializer->RegisterVariable(u8"攻撃力", &attack_power, SerializeTarget::Detail);
	}
	else
	{
		LOG_WARNING({ "Character", "Serialization" }, u8"SetupSerialization: serializer が nullptr です。");
	}
}

void Character::SetupInspector()
{
	BS_PROFILE_FUNCTION();

	GameObject::SetupInspector();

	if (inspector)
	{
		inspector->RegisterVariable(u8"体力", &health, u8"基本ステータス");
		inspector->RegisterVariable(u8"攻撃力", &attack_power, u8"基本ステータス");
		inspector->RegisterText(u8"無敵時間タイマー", &invincible_timer, u8"ステータスモニター");
		inspector->RegisterText(u8"現在のアニメーション", &current_animation_name, u8"アニメーションモニター");
		inspector->RegisterText(u8"アニメーション再生時間", &current_animation_time, u8"アニメーションモニター");
		inspector->RegisterText(u8"ルートモーション有効", &debug_root_motion_enabled, u8"ルートモーション");
		inspector->RegisterText(u8"ルートモーション差分", &debug_root_motion_delta, u8"ルートモーション");
	}
	else
	{
		LOG_WARNING({ "Character", "Inspector" }, u8"SetupInspector: inspector が nullptr です。");
	}
}
//コンポーネントの設定
void Character::SetupComponent()
{
	//TransformComponent の取得・生成
	transform_component = GetComponent<TransformComponent>();
	if (!transform_component) transform_component = AddComponent<TransformComponent>();

	//ModelComponent の取得・生成
	model_component = GetComponent<ModelComponent>();
	if (!model_component) model_component = AddComponent<ModelComponent>();
	model_component->SetTransformComponent(transform_component);

	//MovementComponent の取得・生成
	movement_component = GetComponent<MovementComponent>();
	if (!movement_component) movement_component = AddComponent<MovementComponent>();
	movement_component->SetTransformComponent(transform_component);

	//AttachmentColliderComponentt の取得・生成
	attachment_collider_component = GetComponent<AttachmentColliderComponent>();
	if (!attachment_collider_component) attachment_collider_component = AddComponent<AttachmentColliderComponent>();
}

//ダメージ処理
bool Character::ApplyDamage(float damage, float invincible_time, uint32_t attacker_id)
{
	BS_PROFILE_FUNCTION();

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
		OnDamage(attacker_id, damage);
	}

	return true;
}

//ブラックボードに同期
void Character::SyncBlackboard()
{
	BS_PROFILE_FUNCTION();

	float current_move_speed = movement_component ? movement_component->GetMoveSpeed() : 0.0f;
	bool current_is_ground = movement_component ? movement_component->IsGround() : false;

	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"接地フラグ", current_is_ground);
	blackboard->SetValue(u8"move_speed", current_move_speed);
}

//ブラックボードに登録・初期化
void Character::SetupBlackboard()
{
	BS_PROFILE_FUNCTION();

	blackboard->RegisterVariable(u8"体力");
	blackboard->RegisterVariable(u8"接地フラグ");
	blackboard->RegisterVariable(u8"move_speed");

	SyncBlackboard();
}

//移動方向の設定
void Character::Move(float elapsed_time, float vx, float vz, float speed)
{
	BS_PROFILE_FUNCTION();

	if (state_machine_component->GetCurrentActionCategory() != ActionCategory::Attack)
	{
		movement_component->Move(vx, vz, speed);
	}
}

//旋回処理
void Character::Tuen(float elapsed_time, float vx, float vz, float speed)
{
	BS_PROFILE_FUNCTION();

	if (state_machine_component->GetCurrentActionCategory() != ActionCategory::Attack)
	{
		movement_component->Turn(elapsed_time, vx, vz, speed);
	}
}

//ジャンプ処理
void Character::Jump(float speed)
{
	BS_PROFILE_FUNCTION();

	if (movement_component) movement_component->Jump(speed);
}

//無敵時間更新処理
void Character::UpdateInvincibleTimer(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	if (invincible_timer > 0.0f)
	{
		invincible_timer -= elapsed_time;
	}
}

//ルートモーション更新
void Character::UpdateRootMotion()
{
	BS_PROFILE_FUNCTION();

	if (!model_component || !model_component->GetModel() || !transform_component)
	{
		LOG_WARNING({ "Character", "RootMotion" }, u8"UpdateRootMotion: 必要なコンポーネントまたは Model が nullptr です。");
		return;
	}

	// Model クラスから最新のルートモーション移動差分を取得
	DirectX::XMFLOAT3 delta_pos = root_motion_component->GetDeltaPosition();
	debug_root_motion_delta = delta_pos;

	DirectX::XMVECTOR local_translation = DirectX::XMLoadFloat3(&delta_pos);
	DirectX::XMMATRIX world_transform = transform_component->GetWorldMatrix();
	DirectX::XMVECTOR world_translation = DirectX::XMVector3TransformNormal(local_translation, world_transform);
	world_translation = DirectX::XMVectorSetY(world_translation, 0.0f); // Y軸補正

	DirectX::XMFLOAT3 final_movement = {};
	DirectX::XMStoreFloat3(&final_movement, world_translation);

	//TransformComponent から現在座標を取得して加算更新
	DirectX::XMFLOAT3 pos = transform_component->GetPosition();
	pos.x += final_movement.x;
	pos.y += final_movement.y;
	pos.z += final_movement.z;
	transform_component->SetPosition(pos);
}

//アニメーションシーケンス時間に合わせて当たり判定の有効状態を更新
void Character::UpdateColliderSequencer()
{
	BS_PROFILE_FUNCTION();

	if (!attachment_collider_component || !sequencer_component)return;

	//攻撃判定コライダーをすべて非アクティブ
	attachment_collider_component->SetAllColluderActive(true);
	attachment_collider_component->SetAttackCollidersActive(false);

	//現在のアニメーションシーケンスデータを取得
	const auto* sequence_data = sequencer_component->GetCurrentSequenceData();
	if (!sequence_data || sequence_data->collider_tracks.empty())
	{
		return;
	}

	//有効時間帯に入っているコライダーのみをアクティブ化
	for (const auto& track : sequence_data->collider_tracks)
	{
		bool is_in_range = (current_animation_time >= track.start_time && current_animation_time <= track.end_time);
		if (is_in_range)
		{
			attachment_collider_component->SetColliderActive(track.collider_name, true);
		}
	}
}
