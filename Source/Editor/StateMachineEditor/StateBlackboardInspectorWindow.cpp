#include "StateBlackboardInspectorWindow.h"
#include "Editor\StateMachineEditor\StateGraphDataManager.h"
#include "Gameplay\Common\Blackboard.h"
#include "Engine\Core\Log\Log.h"

#include <DirectXMath.h>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

//グラフ内の遷移条件変数がブラックボードに登録済みか検証
void StateBlackboardInspectorWindow::SyncBlackboardVariablesFromGraph(const GraphData* current_graph, Blackboard* blackboard)
{
	//ポインタの有効性チェックと登録済み変数リストの取得
	if (!current_graph || !blackboard)
	{
		LOG_WARNING({ u8"エディタ", u8"インスペクター" }, u8"グラフデータまたはブラックボードが無効なため変数検証をスキップします");
		return;
	}

	const std::vector<std::string> registered_names = blackboard->GetRegisteredNames();

	//全リンクと遷移条件を走査し、未登録の変数名が使われていれば警告を出力
	for (size_t l = 0; l < current_graph->links.size(); l++)
	{
		const GraphLink& link = current_graph->links[l];

		for (size_t c = 0; c < link.conditions.size(); c++)
		{
			const TransitionCondition& cond = link.conditions[c];

			if (cond.type == ConditionNodeType::InputCheck ||
				cond.type == ConditionNodeType::Random ||
				cond.type == ConditionNodeType::AnimationEnd)
			{
				continue;
			}

			if (!cond.variable_name.empty())
			{
				const bool is_found = std::find(registered_names.begin(), registered_names.end(), cond.variable_name) != registered_names.end();
				if (!is_found)
				{
					LOG_WARNING({ u8"エディタ", u8"インスペクター" }, u8"グラフ上の遷移条件変数 [%s] はブラックボードに未登録です", cond.variable_name.c_str());
				}
			}

			if (cond.type == ConditionNodeType::Distance || cond.type == ConditionNodeType::Ratio)
			{
				if (!cond.secondary_variable_name.empty())
				{
					const bool is_sec_found = std::find(registered_names.begin(), registered_names.end(), cond.secondary_variable_name) != registered_names.end();
					if (!is_sec_found)
					{
						LOG_WARNING({ u8"エディタ", u8"インスペクター" }, u8"グラフ上の副遷移条件変数 [%s] はブラックボードに未登録です", cond.secondary_variable_name.c_str());
					}
				}
			}
		}
	}
}

//インスペクターウィンドウのImGui描画
void StateBlackboardInspectorWindow::DrawInspector(Blackboard* blackboard)
{
	//ウィンドウサイズの初期設定と描画開始判定
	constexpr float default_window_width = 300.0f;
	constexpr float default_window_height = 400.0f;

	ImGui::SetNextWindowSize(ImVec2(default_window_width, default_window_height), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin(u8"シミュレーションパラメータ"))
	{
		ImGui::End();
		return;
	}

	if (!blackboard)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"Blackboardが割り当てられていません");
		LOG_WARNING({ u8"エディタ", u8"インスペクター" }, u8"blackboard が nullptr のため描画を中断しました");
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"【シミュレーション用パラメータ】");
	ImGui::Separator();
	ImGui::Spacing();

	//登録済みの全変数名を取得してそれぞれの操作UIを描画
	const std::vector<std::string> var_names = blackboard->GetRegisteredNames();

	if (var_names.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"登録されているパラメータはありません");
		ImGui::End();
		return;
	}

	for (size_t i = 0; i < var_names.size(); i++)
	{
		DrawVariableControl(blackboard, var_names[i]);
	}
	ImGui::End();
}

//変数の型に応じたImGui操作UIの描画
void StateBlackboardInspectorWindow::DrawVariableControl(Blackboard* blackboard, const std::string& var_name)
{
	constexpr float int_drag_speed = 1.0f;
	constexpr float float_drag_speed = 0.1f;

	ImGui::PushID(var_name.c_str());

	//変数の型を判別して対応するImGuiウィジェットを描画し、変更時に値を反映
	bool bool_val = false;
	int int_val = 0;
	float float_val = 0.0f;
	DirectX::XMFLOAT3 vec_val = { 0.0f, 0.0f, 0.0f };

	if (blackboard->TryGetValue(var_name, bool_val))
	{
		if (ImGui::Checkbox(var_name.c_str(), &bool_val))
		{
			blackboard->SetValue(var_name, bool_val);
		}
	}
	else if (blackboard->TryGetValue(var_name, int_val))
	{
		if (ImGui::DragInt(var_name.c_str(), &int_val, int_drag_speed))
		{
			blackboard->SetValue(var_name, int_val);
		}
	}
	else if (blackboard->TryGetValue(var_name, float_val))
	{
		if (ImGui::DragFloat(var_name.c_str(), &float_val, float_drag_speed, 0.0f, 0.0f, "%.3f"))
		{
			blackboard->SetValue(var_name, float_val);
		}
	}
	else if (blackboard->TryGetValue(var_name, vec_val))
	{
		if (ImGui::DragFloat3(var_name.c_str(), &vec_val.x, float_drag_speed, 0.0f, 0.0f, "%.3f"))
		{
			blackboard->SetValue(var_name, vec_val);
		}
	}
	else
	{
		ImGui::TextDisabled(u8"%s (UI非対応の型)", var_name.c_str());
	}

	ImGui::PopID();
}