#include "UtilityAIComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\Inspector\GuiInspector.h"
#include "Engine\Core\Log\Log.h"

#include <imgui.h>

//コンストラクタ
UtilityAIComponent::UtilityAIComponent()
{
	SetComponentName(u8"ユーティリティAI");
	Initialize();
}

//デストラクタ
UtilityAIComponent::~UtilityAIComponent() = default;

//初期化処理
void UtilityAIComponent::Initialize()
{
	Component::Initialize();

	time_spent_thinking = 5.0f;
	current_action_id = 0;
	hash_id_list = {};
	select_action_function = {};
	utility_settings = {};
	utility_action_map = {};
}

//更新処理
void UtilityAIComponent::Update(float elapsed_time)
{
	//更新時間の確認
	if (time_spent_thinking > 0.0f)
	{
		time_spent_thinking -= elapsed_time;
		return;
	}
	else time_spent_thinking = utility_settings.evaluate_interval;

	//状況の数値化
	float highest_score = -1.0f;
	size_t highest_action_id = 0;
	std::string highest_action_name = "";
	for (auto& [id, action] : utility_action_map)
	{
		float required_score = 1.0f;
		for (int i = 0; i < action.required_conditions.size(); ++i)
		{
			float score = action.required_conditions[i]();
			score = std::clamp(score, 0.0f, 1.0f);
			required_score *= score;
		}

		float bonus_score = 0.0f;
		for (int i = 0; i < action.bonus_conditions.size(); i++)
		{
			float score = action.bonus_conditions[i]();
			score = std::clamp(score, 0.0f, 1.0f);
			bonus_score += score;
		}
		if (id == current_action_id) bonus_score += utility_settings.inertia_bonus;
		action.latest_score = required_score * (1.0f + bonus_score) * action.weight;
		if (highest_score < action.latest_score)
		{
			highest_score = action.latest_score;
			highest_action_id = action.action_id;
			highest_action_name = action.action_name;
		}
	}

	//条件判定
	if (highest_action_id == 0)return;
	bool is_changed = (current_action_id != highest_action_id);
	current_action_id = highest_action_id;
	if (select_action_function && (!utility_settings.notify_on_change_only || is_changed))select_action_function(current_action_id);
}

//Gui処理
void UtilityAIComponent::RenderGui()
{
	if (ImGui::CollapsingHeader(GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();

		ImGui::Text(u8"現在の行動:%s (ID:%zu)",GetCurrentActionName().c_str(), current_action_id);
		ImGui::Text(u8"思考タイマー:");
		ImGui::SameLine();
		float ratio = (utility_settings.evaluate_interval > 0.0f) ? 1.0f - (time_spent_thinking / utility_settings.evaluate_interval) : 1.0f;
		ImGui::ProgressBar(ratio, ImVec2(150.0f, 0.0f));
		ImGui::SameLine();
		if (ImGui::Button(u8"今すぐ思考"))time_spent_thinking = 0.0f;
		ImGui::DragFloat(u8"更新間隔", &utility_settings.evaluate_interval);
		ImGui::DragFloat(u8"維持ボーナス", &utility_settings.inertia_bonus);
		ImGui::Checkbox(u8"変化時のみ通知", &utility_settings.notify_on_change_only);

		for (int i = 0; i < hash_id_list.size(); ++i)
		{
			auto found = utility_action_map.find(hash_id_list[i]);
			if (found == utility_action_map.end())
			{
				ImGui::Text(u8"辞書に存在しないハッシュ値です(ハッシュ値:%zu)", hash_id_list[i]);
				continue;
			}
			ImGui::PushID(i);
			ImGui::Text(u8"行動:%s", found->second.action_name.c_str());
			ImGui::SameLine();
			ImGui::DragFloat(u8"重み", &found->second.weight);
			ImGui::Text(u8"スコア");
			ImGui::SameLine();
			ImGui::ProgressBar((std::max)(0.0f, found->second.latest_score), ImVec2(150.0f, 0.0f));
			ImGui::SameLine();
			ImGui::Text("%.2f", found->second.latest_score);
			ImGui::PopID();
		}
		ImGui::Unindent();
	}
}

//JSON保存用変数登録
void UtilityAIComponent::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"更新時間", &utility_settings.evaluate_interval, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"維持ボーナス", &utility_settings.inertia_bonus, SerializeTarget::Detail);
	serializer->RegisterVariable(u8"変化フラグ", &utility_settings.notify_on_change_only, SerializeTarget::Detail);
}

//インスペクター項目登録
void UtilityAIComponent::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
}

//行動の登録
size_t UtilityAIComponent::RegisterAction(const std::string& action_name)
{
	//ハッシュ化
	size_t action_hash = std::hash<std::string>{}(action_name);

	//重複チェック
	auto found = utility_action_map.find(action_hash);
	if (found == utility_action_map.end())
	{
		//コンテナへの登録
		hash_id_list.push_back(action_hash);
		UtilityAction new_action = {};
		new_action.action_id = action_hash;
		new_action.action_name = action_name;
		utility_action_map[action_hash] = new_action;
	}
	else
	{
		LOG_WARNING({ u8"検索" }, u8"登録しようとしたハッシュ値が重複しています(ハッシュ値:%zu,行動名:%s)", action_hash, action_name.c_str());
	}
	return action_hash;
}

//必須条件の追加
void UtilityAIComponent::AddRequirements(size_t action_id, const std::function<float()>& evaluation_function)
{
	//重複チェック
	auto found = utility_action_map.find(action_id);
	if (found == utility_action_map.end())
	{
		LOG_WARNING({ u8"検索" }, u8"存在しないハッシュ値を引数に入れています(ハッシュ値:%zu)", action_id);
		return;
	}
	found->second.required_conditions.push_back(evaluation_function);
}

//ボーナス条件の追加
void UtilityAIComponent::AddBonus(size_t action_id, const std::function<float()>& evaluation_function)
{
	//重複チェック
	auto found = utility_action_map.find(action_id);
	if (found == utility_action_map.end())
	{
		LOG_WARNING({ u8"検索" }, u8"存在しないハッシュ値を引数に入れています(ハッシュ値:%zu)", action_id);
		return;
	}
	found->second.bonus_conditions.push_back(evaluation_function);
}

//候補の全削除
void UtilityAIComponent::Clear()
{
	utility_action_map.clear();
	hash_id_list.clear();
	current_action_id = 0;
}

//コールバック関数の登録
void UtilityAIComponent::RegisterCallback(const std::function<void(size_t)>& callback_function)
{
	select_action_function = callback_function;
}

//設計情報の設定
void UtilityAIComponent::SetUtilitySettings(const UtilitySettings& settings)
{
	utility_settings = settings;
}

//設定情報の取得
UtilitySettings UtilityAIComponent::GetUtilitySettings() const
{
	return utility_settings;
}

//現在の行動名の取得
std::string UtilityAIComponent::GetCurrentActionName() const
{
	auto found = utility_action_map.find(current_action_id);
	if (found == utility_action_map.end())
	{
		LOG_WARNING({ u8"検索" }, u8"現在の行動名が取得できませんでした(ハッシュ値:%zu)", current_action_id);
		return "";
	}
	return found->second.action_name;
}

//現在の行動IDの取得
size_t UtilityAIComponent::GetCurrentActionId() const
{
	return current_action_id;
}

