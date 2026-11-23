#include "TacticalCommandController.h"
#include "SquadManager.h"
#include "Editor\Inspector\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Camera\CameraManager.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Gameplay\Scene\SceneManager.h"
#include "Gameplay\Common\Blackboard.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\AI\TargetSelectorComponent.h"
#include "Gameplay\Components\Tactics\TacticalTypes.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Tactics\Formation\FormationBase.h"
#include "Gameplay\Components\Tactics\Formation\CircleFormation.h"
#include "Gameplay\Components\Tactics\Formation\FishScaleFormation.h"
#include "Gameplay\Components\Tactics\Formation\VShapedFormation.h"
#include "Gameplay\Components\Tactics\Formation\CraneWingFormation.h"
#include "Gameplay\Components\Tactics\Formation\RectangleFormation.h"
#include "Utility\RayUtility.h"
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
TacticalCommandController::TacticalCommandController()
{
	Initialize();
	SetComponentName(u8"戦略コンポーネント");
}

//デストラクタ
TacticalCommandController::~TacticalCommandController() = default;

//初期化処理
void TacticalCommandController::Initialize()
{
	target_position = { 0.0f,0.0f,0.0f };
	max_ray_distance = 10.0f;
	has_target = false;
	collision_manager = nullptr;
	formation_map[TacticalType::Circle] = std::make_shared<CircleFormation>();
	formation_map[TacticalType::FishScale] = std::make_shared<FishScaleFormation>();
	formation_map[TacticalType::CraneWing] = std::make_shared<CraneWingFormation>();
	formation_map[TacticalType::VShaped] = std::make_shared<VShapedFormation>();
	formation_map[TacticalType::Rectangle] = std::make_shared<RectangleFormation>();
	current_formation = formation_map[TacticalType::Rectangle];
}

//更新処理
void TacticalCommandController::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	std::shared_ptr<Blackboard> blackboard = SquadManager::Instance().GetSquadBlackboard();
	if (!blackboard)return;
	int tactical = blackboard->GetValue(current_tactical_type, static_cast<int>(TacticalType::None));
	TacticalType tactical_type = static_cast<TacticalType>(tactical);

	switch (tactical_type)
	{
	case TacticalType::None:
		SquadManager::Instance().SetTacticalAction(TacticalAction::None);
		break;
	case TacticalType::Retreat:
		SquadManager::Instance().SetTacticalAction(TacticalAction::TargetPoint);
		break;
	case TacticalType::Autonomous:
		SquadManager::Instance().SetTacticalAction(TacticalAction::Automous);
		break;
	case TacticalType::Circle:
	case TacticalType::FishScale:
	case TacticalType::CraneWing:
	case TacticalType::VShaped:
	case TacticalType::Rectangle:
	{
		SquadManager::Instance().SetTacticalAction(TacticalAction::Formation);
		current_formation = formation_map[tactical_type];
		std::shared_ptr<TargetSelectorComponent> selector = target_selector.lock();
		if (!selector)
		{
			LOG_WARNING({ u8"取得" }, u8"TargetSelectorComponentが取得できませんでした");
			return;
		}
		std::shared_ptr<TransformComponent> transform = selector->GetTransformComponent();
		if (!transform)
		{
			LOG_WARNING({ u8"取得" }, u8"TransformComponentが取得できませんでした");
			return;
		};
		current_formation->SetTargetPos(transform->GetPosition());
		std::vector<DirectX::XMFLOAT3> target_positions = current_formation->CalculatePositions(SquadManager::Instance().GetMenberCount());
		SquadManager::Instance().CommandFormation(target_positions);
		break;
	}
	default:
		break;
	}
}

//デバッグ描画
void TacticalCommandController::RenderDebug(ShapeRenderer* renderer)
{
	if (!has_target || !renderer)return;
	DirectX::XMFLOAT4 color = { 0.8f,0.1f,0.4f,1.0f };
	renderer->DrawSphere(target_position, 1.0f, color, ShapeDrawMode::Wireframe);
}

//JSON保存用変数登録
void TacticalCommandController::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"レイの長さ", &max_ray_distance, SerializeTarget::Detail);
	formation_map[TacticalType::Circle]->SetupSerialization(serializer);
	formation_map[TacticalType::FishScale]->SetupSerialization(serializer);
	formation_map[TacticalType::CraneWing]->SetupSerialization(serializer);
	formation_map[TacticalType::VShaped]->SetupSerialization(serializer);
	formation_map[TacticalType::Rectangle]->SetupSerialization(serializer);
}

//GuiInspector専用のインスペクター項目登録
void TacticalCommandController::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	inspector->RegisterVariable(u8"レイの長さ", &max_ray_distance, GetComponentName());
	inspector->RegisterText(u8"対象の位置", &target_position, GetComponentName());
	inspector->RegisterText(u8"対象の存在フラグ", &has_target, GetComponentName());
	formation_map[TacticalType::Circle]->SetupInspector(inspector);
	formation_map[TacticalType::FishScale]->SetupInspector(inspector);
	formation_map[TacticalType::CraneWing]->SetupInspector(inspector);
	formation_map[TacticalType::VShaped]->SetupInspector(inspector);
	formation_map[TacticalType::Rectangle]->SetupInspector(inspector);
}

//当たり判定マネージャーの設定
void TacticalCommandController::SetCollisionManager(CollisionManager* manager)
{
	if (!manager)
	{
		LOG_WARNING({ u8"設定" }, u8"渡されたポインタがnullでした");
		return;
	}
	collision_manager = manager;
}

//対象検索コンポーネントの設定
void TacticalCommandController::SetTargetSelector(std::shared_ptr<TargetSelectorComponent> selector)
{
	target_selector = selector;
}
