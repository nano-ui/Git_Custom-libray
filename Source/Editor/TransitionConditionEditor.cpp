#include "TransitionConditionEditor.h"

#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"

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

		//判定タイプを選択するコンボボックス
		const char* type_ui_names[] = { u8"通常比較", u8"確率(Random)", u8"距離(Distance)", u8"割合(Ratio)" };
		const int total_type_count = 4;
		int selected_type_index = static_cast<int>(condition.type);

		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::Combo(u8"判定タイプ", &selected_type_index, type_ui_names, total_type_count))
		{
			condition.type = static_cast<ConditionNodeType>(selected_type_index);
		}
		ImGui::Spacing();

		//選択されたタイプに応じた関数の呼び出し
		switch (condition.type)
		{
		case ConditionNodeType::NormalCompare: DrawNormalCompareUI(blackboard, condition);	break;
		case ConditionNodeType::Random:        DrawRandomUI(condition);						break;
		case ConditionNodeType::Distance:      DrawDistanceUI(blackboard, condition);		break;
		case ConditionNodeType::Ratio:         DrawRatioUI(blackboard, condition);			break;
		}

		ImGui::Separator();
		ImGui::PopID();
		i++;
	}
}

//通常比較用のImGui入力UI描画
void TransitionConditionEditor::DrawNormalCompareUI(StateBlackboard* blackboard, GraphTransitionCondition& condition)
{
	const char* all_operators[] = { "==","!=",">","<",">=","<=" };	//全ての演算子
	const char* bool_operators[] = { "==","!=" };					//bool型専用の演算子
	const int op_total_count = 6;	//全演算子の総数
	const int op_bool_count = 2;	//bool用演算子の総数

	//ブラックボードが有効か判定
	if (blackboard)
	{
		std::string current_var_name = blackboard->GetVariableNameFromHash(condition.hash_key);	//現在の変数名
		ImGui::SetNextItemWidth(150.0f);

		//コンボボックス描画
		if (ImGui::BeginCombo(u8"対象変数", current_var_name.c_str()))
		{
			std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();	//登録された変数名リスト

			//全ての登録変数をループ
			for (size_t n = 0; n < var_names.size(); n++)
			{
				if (ImGui::Selectable(var_names[n].c_str(), current_var_name == var_names[n]))
				{
					condition.hash_key = blackboard->GetVariableHash(var_names[n]);
				}
			}
			ImGui::EndCombo();
		}

		if (condition.hash_key != 0)
		{
			const BlackboardData& raw_data = blackboard->GetAttributeValue(condition.hash_key);//読み込み情報
			float val_speed = blackboard->GetChangeSpeed(condition.hash_key);	//感度
			float val_min = blackboard->GetMinLimit(condition.hash_key);		//最小値
			float val_max = blackboard->GetMaxLimit(condition.hash_key);		//最大値

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
		}
	}
}

//確率判定用のImGui入力UI描画
void TransitionConditionEditor::DrawRandomUI(GraphTransitionCondition& condition)
{
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"メルセンヌ・ツイスタによる確率中世んを行います");
	int percent_value = static_cast<int>(condition.reference_value);
	const int min_percent = 0;
	const int max_percent = 100;

	ImGui::SetNextItemWidth(150.0f);
	if (ImGui::DragInt(u8"成功確率(0～100)", &percent_value, 1.0f, min_percent, max_percent))
	{
		condition.reference_value = static_cast<float>(percent_value);
	}
}

//距離判定用のImGui入力UI描画
void TransitionConditionEditor::DrawDistanceUI(StateBlackboard* blackboard, GraphTransitionCondition& condition)
{
	if (blackboard)
	{
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"2つの3D座標ベクトル間の直線距離を測定");

		std::string my_pos_name = blackboard->GetVariableNameFromHash(condition.hash_key);
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::BeginCombo(u8"自身の座標", my_pos_name.c_str()))
		{
			std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();
			for (size_t n = 0; n < var_names.size(); n++)
			{
				if (ImGui::Selectable(var_names[n].c_str(), my_pos_name == var_names[n]))
				{
					condition.hash_key = blackboard->GetVariableHash(var_names[n]);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();

		std::string target_pos_name = blackboard->GetVariableNameFromHash(condition.secondary_hash);
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::BeginCombo(u8"対象の座標", target_pos_name.c_str()))
		{
			std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();
			for (size_t n = 0; n < var_names.size(); n++)
			{
				if (ImGui::Selectable(var_names[n].c_str(), target_pos_name == var_names[n]))
				{
					condition.secondary_hash = blackboard->GetVariableHash(var_names[n]);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat(u8"最小距離制限", &condition.reference_value, 0.1f, 0.0f, 1000.0f, "%.2f");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat(u8"最大距離制限", &condition.param_second, 0.1f, 0.0f, 1000.0f, "%.2f");
	}
}

//割合判定用のImGui入力UI描画
void TransitionConditionEditor::DrawRatioUI(StateBlackboard* blackboard, GraphTransitionCondition& condition)
{
	const char* all_operators[] = { "==","!=",">","<",">=","<=" };
	const int op_total_count = 6;

	if (blackboard)
	{
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"現在地/最大値から現在の割合(0.0f～1.0f)を算出して判定");

		std::string cur_name = blackboard->GetVariableNameFromHash(condition.hash_key);
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::BeginCombo(u8"現在値", cur_name.c_str()))
		{
			std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();
			for (size_t n = 0; n < var_names.size(); n++)
			{
				if (ImGui::Selectable(var_names[n].c_str(), cur_name == var_names[n]))
				{
					condition.hash_key = blackboard->GetVariableHash(var_names[n]);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();

		std::string max_name = blackboard->GetVariableNameFromHash(condition.secondary_hash);
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::BeginCombo(u8"最大値", max_name.c_str()))
		{
			std::vector<std::string> var_names = blackboard->GetRegisteredVariableNames();
			for (size_t n = 0; n < var_names.size(); n++)
			{
				if (ImGui::Selectable(var_names[n].c_str(), max_name == var_names[n]))
				{
					condition.secondary_hash = blackboard->GetVariableHash(var_names[n]);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(80.0f);
		ImGui::Combo(u8"演算子", &condition.compare_operator, all_operators, op_total_count);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f);
		ImGui::DragFloat(u8"基準割合", &condition.reference_value, 0.1f);
	}
}


