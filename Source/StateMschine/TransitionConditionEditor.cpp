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
	for (size_t i = 0; i < target_link->conditions.size();)
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
						condition.hash_key = blackboard->GetVariableHash(var_names[n]);
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
		const char* all_operators[] = { "==","!=",">","<",">=","<=" };	//全ての演算子
		const char* bool_operators[] = { "==","!=" };					//bool型専用の演算子

		const int op_total_count = 6;	//全演算子の総数
		const int op_bool_count = 2;	//bool用演算子の総数

		//ブラックボードが紐づいており、変数が選択されている場合
		if (blackboard && condition.hash_key != 0)
		{
			const BlackboardData& raw_data = blackboard->GetAttributeValue(condition.hash_key);	//variantデータ参照

			float val_speed = blackboard->GetChangeSpeed(condition.hash_key);	//変化感度
			float val_min = blackboard->GetMinLimit(condition.hash_key);		//最小値制限
			float val_max = blackboard->GetMaxLimit(condition.hash_key);		//最大値制限

			//変数がboolだった場合
			if (std::holds_alternative<bool>(raw_data))
			{
				ImGui::SetNextItemWidth(80.0f);
				ImGui::Combo(u8"演算子", &condition.compare_operator, bool_operators, op_bool_count);

				if (condition.compare_operator >= op_bool_count)
				{
					condition.compare_operator = 0;
				}
				ImGui::SameLine();

				bool bool_check_value = (condition.reference_value != 0.0f);

				if (ImGui::Checkbox(u8"基準値(True/False)", &bool_check_value))
				{
					condition.reference_value = bool_check_value ? 1.0f : 0.0f;
				}
			}
			else if (std::holds_alternative<int>(raw_data))
			{
				ImGui::SetNextItemWidth(80.0f);
				ImGui::Combo(u8"演算子", &condition.compare_operator, all_operators, op_total_count);

				ImGui::SameLine();
				ImGui::SetNextItemWidth(120.0f);

				int int_drag_value = static_cast<int>(condition.reference_value);

				if (ImGui::DragInt(u8"基準値", &int_drag_value, val_speed, static_cast<int>(val_min), static_cast<int>(val_max)))
				{
					condition.reference_value = static_cast<float>(int_drag_value);
				}
			}
			else if (std::holds_alternative<float>(raw_data))
			{
				ImGui::SetNextItemWidth(80.0f);
				ImGui::Combo(u8"演算子", &condition.compare_operator, all_operators, op_total_count);

				ImGui::SameLine();
				ImGui::SetNextItemWidth(120.0f);

				ImGui::DragFloat(u8"基準値", &condition.reference_value, val_speed, val_min, val_max, "%.3f");
			}
			else
			{
				ImGui::SetNextItemWidth(80.0f);
				ImGui::Combo(u8"演算子", &condition.compare_operator, bool_operators, op_bool_count);

				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), u8"(ベクトル型変数は現在プレビューのみ)");
			}
		}
		else
		{
			ImGui::SetNextItemWidth(80.0f);
			ImGui::Combo(u8"演算子", &condition.compare_operator, all_operators, op_total_count);

			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFloat(u8"基準値", &condition.reference_value);
		}


		ImGui::Separator();
		ImGui::PopID();

		i++;
	}
}

