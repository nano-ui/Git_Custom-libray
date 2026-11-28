#include "SituationEvaluator.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Status\HealthComponent.h"
#include "Engine\Core\Log\Log.h"
#include "Utility\ConvertUtility.h"

//コンストラクタ
SituationEvaluator::SituationEvaluator()
{
	//コンポーネント名の設定と基底クラスの初期化
	SetComponentName(u8"戦況評価");
	Component::Initialize();

	//各スコアの初期値設定
	Initialize();
}

//デストラクタ
SituationEvaluator::~SituationEvaluator() = default;

//初期化
void SituationEvaluator::Initialize()
{
	average_hp_score = 0.0f;
	dying_score = 0.0f;
	activ_score = 0.0f;
	target_hp_score = 0.0f;
}

//GameObjectを利用したスコア計算
void SituationEvaluator::ObjectsScoreCalculation(const std::vector<GameObject*>& game_objects)
{
	if (game_objects.empty())
	{
		Initialize();
		return;
	}
	AverageHp(game_objects);
	MortalityRate(game_objects);
}

//平均HPの割合を算出
void SituationEvaluator::AverageHp(const std::vector<GameObject*>& game_objects)
{
	float total_score = 0.0f;
	int valid_count = 0;
	for (int i = 0; i < game_objects.size(); ++i)
	{
		if (!game_objects[i] || !game_objects[i]->IsActive())continue;
		const std::shared_ptr<HealthComponent> health = game_objects[i]->GetComponent<HealthComponent>();
		if (!health)
		{
			LOG_WARNING({ u8"取得" }, u8"ゲームオブジェクトからHealthComponentが取得できませんでした");
			continue;
		}
		if (health->GetHealth() <= 0.0f)continue;
		valid_count++;
		float score = ConvertUtility::ConvertNormalized(health->GetHealth(), 0.0f, health->GetMaxHealth());
		total_score += score;
	}
	average_hp_score = (valid_count > 0) ? total_score / valid_count : 0.0f;
}

//部隊の瀕死人数・生存人数を算出
void SituationEvaluator::MortalityRate(const std::vector<GameObject*>& game_objects)
{
	int valid_count = 0;
	int dying_count = 0;
	for (int i = 0; i < game_objects.size(); ++i)
	{
		if (!game_objects[i] || !game_objects[i]->IsActive())continue;
		std::shared_ptr<HealthComponent> health = game_objects[i]->GetComponent<HealthComponent>();
		if (!health)
		{
			LOG_WARNING({ u8"取得" }, u8"ゲームオブジェクトからHealthComponentが取得できませんでした");
			continue;
		}
		if (health->GetHealth() <= 0.0f || health->GetMaxHealth() <= 0.0f)continue;
		valid_count++;
		float ratio = health->GetHealth() / health->GetMaxHealth();
		constexpr float dying_rate = 0.3f;
		if (ratio <= dying_rate) dying_count++;
	}
	dying_score = ConvertUtility::ConvertNormalized(static_cast<float>(dying_count), 0.0f, static_cast<float>(valid_count));
	const size_t member_count = SquadManager::Instance().GetMaxMemberCount();
	activ_score = (member_count > 0) ? ConvertUtility::ConvertNormalized(static_cast<float>(valid_count), 0.0f, static_cast<float>(member_count)) : 0.0f;
}