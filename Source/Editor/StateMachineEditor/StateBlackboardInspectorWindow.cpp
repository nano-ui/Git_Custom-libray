#include "StateBlackboardInspectorWindow.h"
#include "Gameplay\StateMachine\StateGraphDataManager.h"
#include "Gameplay\StateMachine\StateBlackboard.h"

#include <imgui.h>
#include <cstdio>

//遷移条件変数を登録
void StateBlackboardInspectorWindow::SyncBlackboardVariablesFromGraph(const GraphData* current_graph, StateBlackboard* blackboard)
{
	if (!current_graph || !blackboard)return;

	//グラフ内の全リンクを走査
	for (size_t l = 0; l < current_graph->links.size(); l++)
	{
		const GraphLink& link = current_graph->links[l];

		//リンクが保持する全ての遷移条件を走査
		for (size_t c = 0; c < link.conditions.size(); c++)
		{
			const GraphTransitionCondition& cond = link.conditions[c];

			if (cond.type == ConditionNodeType::InputCheck || cond.type == ConditionNodeType::Random) return;

			//ハッシュキーが設定されており、Blackboardに未登録の場合に自動登録
			if (cond.hash_key != 0)
			{
				std::string var_name = blackboard->GetVariableNameFromHash(cond.hash_key);
				if (var_name.empty())
				{
					std::string default_name = "Param_" + std::to_string(cond.hash_key);
					blackboard->RegisterVariable(default_name);
					printf("StateBlackboardInspectorWindow: グラフ上の条件キー[Hash:%u]を「%s」として自動登録しました。\n",
						cond.hash_key, default_name.c_str());
				}
			}

			//副ハッシュキーが存在する場合も自動登録
			if (cond.secondary_hash != 0)
			{
				std::string sec_var_name = blackboard->GetVariableNameFromHash(cond.secondary_hash);
				if (sec_var_name.empty())
				{
					std::string default_sec_name = "Param_" + std::to_string(cond.secondary_hash);
					blackboard->RegisterVariable(default_sec_name);

					printf("StateBlackboardInspectorWindow: グラフ上の副条件キー[Hash:%u]を「%s」として自動登録しました。\n",
						cond.secondary_hash, default_sec_name.c_str());
				}
			}
		}
	}
}

//ImGui描画
void StateBlackboardInspectorWindow::DrawInspector(StateBlackboard* blackboard)
{
	//初期ウィンドウサイズ
	constexpr float default_window_width = 300.0f;
	constexpr float default_window_height = 400.0f;

	ImGui::SetNextWindowSize(ImVec2(default_window_width, default_window_height), ImGuiCond_FirstUseEver);

	//独立ウィンドウとして描画
	if (!ImGui::Begin(u8"シミュレーションパラメータ"))
	{
		ImGui::End();
		return;
	}

	if (!blackboard)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"Blackboardが割り当てられていません");
		printf("Warning: StateBlackboardInspectorWindow::DrawInspector - blackboard が nullptr のため描画を中断しました。\n");
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"【シミュレーション用パラメータ】");
	ImGui::Separator();
	ImGui::Spacing();

	//Blackboardに登録されている全変数名のリストを取得
	std::vector<std::string> var_name = blackboard->GetRegisteredVariableNames();

	if (var_name.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"登録されているパラメータはありません");
		ImGui::End();
		return;
	}

	//登録変数ごとにImGui入力UIを描画
	for (size_t i = 0; i < var_name.size(); i++)
	{
		uint32_t hash_key = blackboard->GetVariableHash(var_name[i]);
		DrawVariableControl(blackboard, var_name[i], hash_key);
	}
	ImGui::End();
}

//変数の型に応じたImGui描画
void StateBlackboardInspectorWindow::DrawVariableControl(StateBlackboard* blackboard, const std::string& var_name, uint32_t hash_key)
{
	const BlackboardData& raw_data = blackboard->GetAttributeValue(hash_key);
	float val_speed = blackboard->GetChangeSpeed(hash_key);
	float val_min = blackboard->GetMinLimit(hash_key);
	float val_max = blackboard->GetMaxLimit(hash_key);

	ImGui::PushID(static_cast<int>(hash_key));

	//型に応じたImGui描画
	if (std::holds_alternative<bool>(raw_data))
	{
		bool current_val = std::get<bool>(raw_data);
		if (ImGui::Checkbox(var_name.c_str(), &current_val))
		{
			blackboard->SetValue(var_name, current_val);
		}
	}
	else if (std::holds_alternative<int>(raw_data))
	{
		int current_val = std::get<int>(raw_data);
		if (ImGui::DragInt(var_name.c_str(), &current_val, val_speed, static_cast<int>(val_min), static_cast<int>(val_max)))
		{
			blackboard->SetValue(var_name, current_val);
		}
	}
	else if (std::holds_alternative<float>(raw_data))
	{
		float current_val = std::get<float>(raw_data);
		if (ImGui::DragFloat(var_name.c_str(), &current_val, val_speed, val_min, val_max, "%.3f"))
		{
			blackboard->SetValue(var_name, current_val);
		}
	}
	ImGui::PopID();
}
