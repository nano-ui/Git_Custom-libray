#include "Commander.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Tactics\TacticalCommandController.h"
#include "Gameplay\Components\AI\TargetSelectorComponent.h"
#include "Gameplay\Components\AI\Utility\UtilityAIComponent.h"
#include "Gameplay\Components\Tactics\SituationEvaluator.h"
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
	situaltion_evaluator->MemberScoreCalculation(SquadManager::Instance().GetMenbers());
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
	if (!situaltion_evaluator)situaltion_evaluator = AddComponent<SituationEvaluator>();
	else situaltion_evaluator = GetComponent<SituationEvaluator>();
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
	//コンポーネントの有効性確認
	if (!utility_ai || !situaltion_evaluator)
	{
		LOG_WARNING({ u8"初期化" }, u8"戦術設定に必要なコンポーネントが不足しています");
		return;
	}

	//戦術アクションの登録
	size_t retreat_id = RegisterTactic(u8"全体退避", TacticalType::Retreat);
	size_t autonomous_id = RegisterTactic(u8"個別行動", TacticalType::Autonomous);
	size_t circle_id = RegisterTactic(u8"円陣", TacticalType::Circle);
	size_t fish_scale_id = RegisterTactic(u8"魚鱗の陣", TacticalType::FishScale);
	size_t crane_wing_id = RegisterTactic(u8"鶴翼の陣", TacticalType::CraneWing);
	size_t v_shaped_id = RegisterTactic(u8"V字の陣", TacticalType::VShaped);
	size_t rectangle_id = RegisterTactic(u8"長方形の陣", TacticalType::Rectangle);

	//評価計算用の定数定義
	constexpr float max_score = 1.0f;
	constexpr float crane_wing_weight = 0.85f;
	constexpr float v_shaped_weight = 0.8f;
	constexpr float base_rectangle_weight = 0.6f;

	//全体退避の条件設定（瀕死者がいる場合に最優先）
	utility_ai->AddRequirements(retreat_id, [this]() { return situaltion_evaluator->GetDyingScore(); });
	utility_ai->AddBonus(retreat_id, [this, max_score]() { return max_score - situaltion_evaluator->GetAverageHpScore(); });

	//円陣の条件設定（HPが減っている際の防御）
	utility_ai->AddRequirements(circle_id, [this, max_score]() { return max_score - situaltion_evaluator->GetAverageHpScore(); });
	utility_ai->AddBonus(circle_id, [this]() { return situaltion_evaluator->GetActiveScore(); });

	//個別行動の条件設定（生存人数が減った際の自律戦闘）
	utility_ai->AddRequirements(autonomous_id, [this, max_score]() { return max_score - situaltion_evaluator->GetActiveScore(); });
	utility_ai->AddBonus(autonomous_id, [this]() { return situaltion_evaluator->GetAverageHpScore(); });

	//魚鱗の陣の条件設定（HP・生存数ともに万全な時の一斉攻撃）
	utility_ai->AddRequirements(fish_scale_id, [this]() { return situaltion_evaluator->GetAverageHpScore(); });
	utility_ai->AddRequirements(fish_scale_id, [this]() { return situaltion_evaluator->GetActiveScore(); });

	//鶴翼の陣の条件設定（生存人数が多い時の包囲攻撃）
	utility_ai->AddRequirements(crane_wing_id, [this]() { return situaltion_evaluator->GetActiveScore(); });
	utility_ai->AddBonus(crane_wing_id, [this]() { return situaltion_evaluator->GetAverageHpScore(); });
	utility_ai->SetWeight(crane_wing_id, crane_wing_weight);

	//V字の陣の条件設定（生存人数が多い時の挟撃）
	utility_ai->AddRequirements(v_shaped_id, [this]() { return situaltion_evaluator->GetActiveScore(); });
	utility_ai->AddBonus(v_shaped_id, [this]() { return situaltion_evaluator->GetAverageHpScore(); });
	utility_ai->SetWeight(v_shaped_id, v_shaped_weight);

	//長方形の陣の条件設定（どの特殊条件にも当てはまらない時の基本陣形）
	utility_ai->SetWeight(rectangle_id, base_rectangle_weight);

	//戦術決定時のコールバック登録
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
			if (!blackboard)
			{
				LOG_WARNING({ u8"更新" }, u8"部隊のBlackboardが取得できませんでした");
				return;
			}
			blackboard->SetValue(current_tactical_type, static_cast<int>(found->second));
		}
	);
}
