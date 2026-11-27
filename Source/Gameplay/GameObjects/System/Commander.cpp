#include "Commander.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Tactics\TacticalCommandController.h"
#include "Gameplay\Components\AI\TargetSelectorComponent.h"
#include "Gameplay\Components\AI\Utility\UtilityAIComponent.h"
#include "Gameplay\Common\Blackboard.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Core\Log\Log.h"

//コンストラクタ
Commander::Commander()
{
	SetClassName("Commander");
	SetupComponent();
}

//デストラクタ
Commander::~Commander() = default;

//初期化処理
void Commander::Initialize()
{
	GameObject::Initialize();
	target_selector->SetHateComponent(SquadManager::Instance().GetAlliesHate());
	tactical_command_controller->SetTargetSelector(target_selector);
	tactical_command_controller->SetCollisionManager(ObjectManager::Instance().GetCollisionManager());
	SetupTactics();
}

//更新処理
void Commander::Update(float elapsed_time)
{
	GameObject::Update(elapsed_time);
}

//描画処理
void Commander::Render(ID3D11DeviceContext* context)
{

}

//デバッグ描画処理
void Commander::RenderDebug(ShapeRenderer* renderer)
{
	tactical_command_controller->RenderDebug(renderer);
}

//ImGuiデバッグ描画
void Commander::RenderGui()
{
	ImGui::Begin(u8"指揮官");
	GameObject::RenderGui();
	ImGui::End();
}

//コンポーネントの設定
void Commander::SetupComponent()
{
	if (!tactical_command_controller)tactical_command_controller = AddComponent<TacticalCommandController>();
	else tactical_command_controller = GetComponent<TacticalCommandController>();
	if (!target_selector)target_selector = AddComponent<TargetSelectorComponent>();
	else target_selector = GetComponent<TargetSelectorComponent>();
	if (!utility_ai)utility_ai = AddComponent<UtilityAIComponent>();
	else utility_ai = GetComponent<UtilityAIComponent>();
}

//シリアライズ登録
void Commander::SetupSerialization()
{
	GameObject::SetupSerialization();
}

//戦術の登録
size_t Commander::RegisterTactic(const std::string& tactical_name, TacticalType tactical)
{
	size_t action_id = utility_ai->RegisterAction(tactical_name);
	tactical_map[action_id] = tactical;
	return action_id;
}

//戦術の設定
void Commander::SetupTactics()
{
	size_t retreat_id = RegisterTactic(u8"全体退避", TacticalType::Retreat);
	size_t autonomous_id = RegisterTactic(u8"個別行動", TacticalType::Autonomous);
	size_t circle_id = RegisterTactic(u8"円陣", TacticalType::Circle);
	size_t fish_scale_id = RegisterTactic(u8"魚鱗の陣", TacticalType::FishScale);
	size_t crane_wing_id = RegisterTactic(u8"鶴翼の陣", TacticalType::CraneWing);
	size_t v_shaped_id = RegisterTactic(u8"V字の陣", TacticalType::VShaped);
	size_t rectangle_id = RegisterTactic(u8"長方形の陣", TacticalType::Rectangle);

	utility_ai->RegisterCallback
	([this](size_t id)
		{
			auto found = tactical_map.find(id);
			if (found == tactical_map.end())
			{
				LOG_WARNING({ u8"検索" }, u8"登録されていないIDが呼び出されました(ID:%zu)", id);
				return;
			}
			auto blackboard = SquadManager::Instance().GetSquadBlackboard();
			if (!blackboard)return;
			blackboard->SetValue(current_tactical_type, static_cast<int>(found->second));
		}
	);
}
