#include "TransitionConditionEditor.h"

#include "../StateMschine/StateBlackboard.h"
#include "../StateMschine/StateGraphDataManager.h"

#include <imgui.h>
#include <cstdio>

//コンストラクタ
TransitionConditionEditor::TransitionConditionEditor()
{

}

//デストラクタ
TransitionConditionEditor::~TransitionConditionEditor() = default;

//リンクの遷移条件設定を描画
void TransitionConditionEditor::DrawConditonSettings(StateGraphDataManager* data_manager, StateBlackboard* blackboard, uint32_t grap_id, GraphLink* target_link)
{
	//ポインタの安全チェック
	if (!data_manager || !target_link)
	{
		printf("Error: TransitionConditionEditor::DrawConditionSettings - 必要なポインタが nullptr です。\n");
		return;
	}

	//リンクに新しい条件を追加
	if (ImGui::Button(u8"遷移条件を追加"))
	{
		data_manager->AddConditionToLink(grap_id, target_link->id);
		printf("TransitionConditionEditor: リンク ID:%d に新しい遷移条件枠を追加しました。\n", target_link->id);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	//登録されている遷移条件のループUI描画と削除管理
	for (size_t i = 0; i < target_link->conditions.size(); i++)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Text(u8"条件[%zu]", i);
		ImGui::SameLine();

		const ImVec4 red_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);	//削除ボタンの色
		ImGui::PushStyleColor(ImGuiCol_Button, red_color);

		//削除ボタンが押されたか判定
		if (ImGui::Button(u8"削除"))
		{
			data_manager->DeleteConditionFromLink(grap_id, target_link->id, i);
			printf("TransitionConditionEditor: リンク ID:%d から条件 [%zu] を削除しました。\n", target_link->id, i);

			ImGui::PopStyleColor();
			ImGui::PopID();
			continue;
		}
		ImGui::PopStyleColor();
		GraphTransitionCondition& condition = target_link->conditions[i];	//編集対象の条件情報

		//ブラックボード変数選択コンボボックス描画
		if (blackboard)
		{
			std::string current_var_name = blackboard->GetVariableNameFromHash(condition.hash_key);

			//変数選択用のコンボボックスを開始
			if (ImGui::BeginCombo(u8"対象変数", current_var_name.c_str()))
			{
				std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();

				//取得した変数名の数だけ選択肢をループ生成
				for (size_t n = 0; n < var_names.size(); n++)
				{
					bool is_selected = (current_var_name == var_names[n]);	//現在の項目が選択中かどうか判定

					//選択しアイテムがクリックされたか判定
					if (ImGui::Selectable(var_names[n].c_str(), is_selected))
					{
						printf("TransitionConditionEditor: 変数「%s」が選択されました。\n", var_names[n].c_str());
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), u8"ブラックボードがリンクされていません");
		}
		//比較演算子と基準値の設定UI描画
		const char* operator_list[] = { "==","!=",">","<",">=","<=" };
		const int operator_count = 6;

		ImGui::SetNextItemWidth(80.0f);

		ImGui::Combo(u8"演算子", &condition.compare_operator, operator_list, operator_count);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);

		ImGui::InputFloat(u8"基準値", &condition.reference_value);

		ImGui::Separator();
		ImGui::PopID();

		i++;
	}
}

