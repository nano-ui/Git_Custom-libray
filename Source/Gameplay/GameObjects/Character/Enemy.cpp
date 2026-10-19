#include "Enemy.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"
#include "Gameplay\Components\Editor\AttachmentColliderComponent.h"
#include "Gameplay\Components\Movement\Move\LinearMove.h"

#include "Engine\Graphics\Renderers\ShapeRenderer.h"

#include <Windows.h>

static AutoRegister<Enemy> auto_register_enemy("Enemy");

static constexpr uint32_t DUMMY_BLACKBOARD_KEY = 0;
static constexpr float DEFAULT_CHASE_SPEED = 3.0f;
static constexpr float DEFAULT_STOP_DISTANCE = 1.5f;
static constexpr float EPSILON_DISTANCE_SQ = 0.0001f;

//コンストラクタ
Enemy::Enemy()
{
	SetClassName("Enemy");
	chase_speed = DEFAULT_CHASE_SPEED;
	stop_distance = DEFAULT_STOP_DISTANCE;
}

//デストラクタ
Enemy::~Enemy() = default;

//初期化処理
void Enemy::Initialize()
{
	SetupComponent();

	linear_move = std::make_unique<LinearMove>(DUMMY_BLACKBOARD_KEY);

	//基底クラスの初期化
	Character::Initialize();
}

//更新処理
void Enemy::Update(float elapsed_time)
{
	if (is_play)
	{
		UpdateChase(elapsed_time);
		Character::Update(elapsed_time);
	}
}

//デバッグ描画
void Enemy::RenderDebug(ShapeRenderer* renderer)
{
	Character::RenderDebug(renderer);
}

//シリアライズ登録
void Enemy::SetupSerialization()
{
	Character::SetupSerialization();
	serializer->RegisterVariable(u8"開始", &is_play, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"速度", &chase_speed, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"停止距離", &stop_distance, SerializeTarget::Detail);
}

//インスペクター登録
void Enemy::SetupInspector()
{
	Character::SetupInspector();
	inspector->RegisterVariable(u8"開始", &is_play);
	inspector->RegisterVariable(u8"速度", &chase_speed);
	inspector->RegisterVariable(u8"停止距離", &stop_distance);
}

// アニメーション終了イベント
void Enemy::OnAnimationEnd(uint32_t state_key)
{

}

//対象を設定
void Enemy::SetTarget(const std::shared_ptr<TransformComponent>& target)
{
	target_transform = target;
}

//デフォルトパスの取得
std::string Enemy::GetDefaultModelPath() const
{
	return "Data/Model/Character/Enemy/Rampage_Elemental.gltf";
}

//コンポーネント群のセットアップ
void Enemy::SetupComponent()
{
	transform_component = GetComponent<TransformComponent>();
	if (!transform_component)transform_component = AddComponent<TransformComponent>();

	model_component = GetComponent<ModelComponent>();
	if (!model_component)model_component = AddComponent<ModelComponent>();

	if (model_component && transform_component)
	{
		model_component->SetTransformComponent(transform_component);

		//モデルがロードされていない場合
		if (model_component->GetModelPath().empty())
		{
			const std::string default_model_path = GetDefaultModelPath();
			model_component->LoadModel(default_model_path);
		}

	}
	else
	{
		OutputDebugStringA("[Enemy エラー] SetupComponents: 必要な基本コンポーネントの生成に失敗しました。\n");
	}
}

//座標計算・移動入力の更新処理
void Enemy::UpdateChase(float elapsed_time)
{
	auto target = target_transform.lock();
	if (!target)
	{
		//OutputDebugStringA("[Enemy 警告] UpdateChase: 追尾対象の TransformComponent が無効または未設定です。\n");
		return;
	}
	
	//座標の取得
	DirectX::XMFLOAT3 current_pos = transform_component->GetPosition();
	DirectX::XMFLOAT3 target_pos = target->GetPosition();
	target_pos.y = current_pos.y;

	//距離を算出
	DirectX::XMVECTOR current_v = DirectX::XMLoadFloat3(&current_pos);
	DirectX::XMVECTOR target_v = DirectX::XMLoadFloat3(&target_pos);
	DirectX::XMVECTOR diff_v = DirectX::XMVectorSubtract(target_v, current_v);
	DirectX::XMVECTOR dist_sq_v = DirectX::XMVector3LengthSq(diff_v);

	float distance_sq = 0.0f;
	DirectX::XMStoreFloat(&distance_sq, dist_sq_v);

	float stop_distance_sq = stop_distance * stop_distance;
	if (distance_sq <= stop_distance_sq || distance_sq < EPSILON_DISTANCE_SQ)
	{
		Character::Move(elapsed_time, 0.0f, 0.0f, 0.0f);
		return;
	}

	//移動速度を算出
	DirectX::XMFLOAT3 move_velocity = linear_move->UpdateLiner(current_pos, target_pos, chase_speed);

	float length = std::sqrtf((move_velocity.x * move_velocity.x) + (move_velocity.z * move_velocity.z));
	if (length < EPSILON_DISTANCE_SQ)
	{
		Character::Move(elapsed_time, 0.0f, 0.0f, 0.0f);
		return;
	}

	float dir_x = move_velocity.x / length;
	float dir_z = move_velocity.z / length;

	//実際に移動
	Character::Move(elapsed_time, dir_x, dir_z, chase_speed);
	Character::Tuen(elapsed_time, dir_x, dir_z, chase_speed);
}
