#include "Character.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModel.h"

#include <imgui.h>
#include <cmath>

//コンストラクタ
Character::Character()
	: health(100.0f)
	, attack_power(10.0f)
	, invincible_timer(0.0f)
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

	//TransformComponent の取得・生成
	transform_component = GetComponent<TransformComponent>();
	if (!transform_component) transform_component = AddComponent<TransformComponent>();

	//ModelComponent の取得・生成
	model_component = GetComponent<ModelComponent>();
	if (!model_component) model_component = AddComponent<ModelComponent>();
	model_component->SetTransformComponent(transform_component);

	//MovementComponent の取得・生成とセットアップ
	movement_component = GetComponent<MovementComponent>();
	if (!movement_component) movement_component = AddComponent<MovementComponent>();
	movement_component->SetTransformComponent(transform_component);

	//全コンポーネントの初期化実行
	GameObject::Initialize();

	SetupBlackboard();

	if (model_component->GetModel())
	{
		sequencer_component = std::make_unique<AnimationSequencerComponent>(model_component);
		sequencer_component->Initialize(model_component->GetModelPath());

		if (root_motion_component && model_component->GetModelData())
		{
			root_motion_component->Initialize(model_component->GetModelData());
		}
	}

	SetupSerialization();
	SetupInspector();
}

//更新処理
void Character::Update(float elapsed_time)
{
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
			if (sequencer_component)
			{
				constexpr float default_blend_time = 0.2f;
				sequencer_component->ChangeAnimation(target_anim_name, default_blend_time);
			}
			previous_animation_name = target_anim_name;
		}
	}

	if (sequencer_component)
	{
		sequencer_component->Update(elapsed_time);
		current_animation_name = sequencer_component->GetCurrentAnimationName();
		current_animation_time = sequencer_component->GetCurrentSequenceTime();
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
}

//描画処理
void Character::Render(ID3D11DeviceContext* context)
{
	GameObject::Render(context);
}

//デバッグ描画
void Character::RenderDebug(ShapeRenderer* renderer)
{

}

//をシリアライザに登録
void Character::SetupSerialization()
{
	GameObject::SetupSerialization();

	if (serializer)
	{
		serializer->RegisterVariable(u8"体力", &health);
		serializer->RegisterVariable(u8"攻撃力", &attack_power);
	}
}

void Character::SetupInspector()
{
	GameObject::SetupInspector();

	if (inspector)
	{
		inspector->RegisterVariable(u8"体力", &health, u8"基本ステータス");
		inspector->RegisterVariable(u8"攻撃力", &attack_power, u8"基本ステータス");
		inspector->RegisterText(u8"無敵時間タイマー", &invincible_timer, u8"ステータスモニター");
		inspector->RegisterText(u8"現在のアニメーション", &current_animation_name, u8"アニメーションモニター");
		inspector->RegisterText(u8"アニメーション再生時間", &current_animation_time, u8"アニメーションモニター");
	}
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

//ブラックボードに登録・初期化
void Character::SetupBlackboard()
{
	blackboard->RegisterVariable(u8"体力");
	blackboard->RegisterVariable("move_speed");
	blackboard->RegisterVariable(u8"接地フラグ");

	blackboard->SetValue(u8"体力", health);
	blackboard->SetValue(u8"接地フラグ", movement_component ? movement_component->IsGround() : false);
	blackboard->SetValue("move_speed", movement_component ? movement_component->GetMoveSpeed() : 0.0f);
}

//移動方向の設定
void Character::Move(float elapsed_time, float vx, float vz, float speed)
{
	if (movement_component) movement_component->Move(vx, vz, speed);
}

//旋回処理
void Character::Tuen(float elapsed_time, float vx, float vz, float speed)
{
	if (movement_component) movement_component->Turn(elapsed_time, vx, vz, speed);
}

//ジャンプ処理
void Character::Jump(float speed)
{
	if (movement_component) movement_component->Jump(speed);
}

//無敵時間更新処理
void Character::UpdateInvincibleTimer(float elapsed_time)
{
	if (invincible_timer > 0.0f)
	{
		invincible_timer -= elapsed_time;
	}
}

//ルートモーション更新
void Character::UpdateRootMotion()
{
	if (!model_component || !model_component->GetModel() || !transform_component)
	{
		OutputDebugStringA("[Character 警告] UpdateRootMotion: 必要なコンポーネントまたは Model が nullptr です。\n");
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