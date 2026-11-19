#include "TransitionConditionEditor.h"

#include "Gameplay\StateMachine\TransitionCondition.h"
#include "Gameplay\Common\Blackboard.h"
#include "Editor\StateMachineEditor\StateGraphDataManager.h"
#include "Engine\Core\Input.h"
#include "Engine\Core\Log\Log.h"

#include <imgui.h>
#include <cstdio>
#include <Windows.h>
#include <winuser.h>

//コンストラクタ
TransitionConditionEditor::TransitionConditionEditor() = default;

//デストラクタ
TransitionConditionEditor::~TransitionConditionEditor() = default;

//リンクの遷移条件設定を描画
bool TransitionConditionEditor::DrawConditonSettings(
	StateGraphDataManager* data_manager,
	Blackboard* blackboard,
	uint32_t grap_id,
	GraphLink* target_link)
{
	//ポインタの有効性チェックとブレンド時間の編集UI描画
	if (!data_manager || !target_link)
	{
		printf("Error: TransitionConditionEditor::DrawConditionSettings - 必要なポインタが nullptr です。\n");
		return false;
	}

	constexpr float drag_spped = 0.1f;
	constexpr float drag_max = 10.0f;
	constexpr float drag_min = 0.0f;
	ImGui::DragFloat(u8"ブレンド時間", &target_link->blend_time, drag_spped, drag_min, drag_max);

	bool is_changed = false;

	//遷移条件の新規追加ボタン処理
	if (ImGui::Button(u8"遷移条件を追加"))
	{
		data_manager->AddConditionToLink(grap_id, target_link->id);
		printf("TransitionConditionEditor: リンク ID:%d に新しい遷移条件枠を追加しました。\n", target_link->id);
		is_changed = true;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	//登録済みの全遷移条件のループ描画と削除・タイプ別UIの呼び出し
	for (size_t i = 0; i < target_link->conditions.size();)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Text(u8"条件[%zu]", i);
		ImGui::SameLine();

		const ImVec4 red_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, red_color);

		if (ImGui::Button(u8"削除"))
		{
			data_manager->DeleteConditionFromLink(grap_id, target_link->id, i);
			printf("TransitionConditionEditor: リンク ID:%d から条件 [%zu] を削除しました。\n", target_link->id, i);

			ImGui::PopStyleColor();
			ImGui::PopID();
			is_changed = true;
			continue;
		}
		ImGui::PopStyleColor();
		TransitionCondition& condition = target_link->conditions[i];

		const char* condition_types[] = {
			u8"通常比較",
			u8"確率抽選 (Random)",
			u8"距離判定 (Distance)",
			u8"割合判定 (Ratio)",
			u8"入力チェック (InputCheck)",
			u8"アニメーション終了 (AnimationEnd)"
		};

		int current_type = static_cast<int>(condition.type);
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::Combo(u8"条件判定のタイプ", &current_type, condition_types, 6))
		{
			condition.type = static_cast<ConditionNodeType>(current_type);
			is_changed = true;
		}
		ImGui::Spacing();

		switch (condition.type)
		{
		case ConditionNodeType::NormalCompare: is_changed |= DrawNormalCompareUI(blackboard, condition); break;
		case ConditionNodeType::Random:        DrawRandomUI(condition);						break;
		case ConditionNodeType::Distance:      DrawDistanceUI(blackboard, condition);		break;
		case ConditionNodeType::Ratio:         DrawRatioUI(blackboard, condition);			break;
		case ConditionNodeType::InputCheck:	   DrawInputCheckUI(condition);					break;
		}

		ImGui::Separator();
		ImGui::PopID();
		i++;
	}
	return is_changed;
}

//通常比較用のImGui入力UI描画
bool TransitionConditionEditor::DrawNormalCompareUI(Blackboard* blackboard, TransitionCondition& condition)
{
	bool is_ui_changed = false;

	const char* all_operators[] = { "==","!=",">","<",">=","<=" };
	const char* bool_operators[] = { "==","!=" };
	constexpr int op_total_count = 6;
	constexpr int op_bool_count = 2;
	constexpr float combo_var_width = 150.0f;
	constexpr float combo_op_width = 80.0f;
	constexpr float drag_input_width = 120.0f;
	constexpr float drag_vec_width = 200.0f;
	constexpr float int_drag_speed = 1.0f;
	constexpr float float_drag_speed = 0.1f;

	//ブラックボードの有効性確認と対象変数コンボボックスの描画
	if (!blackboard)
	{
		return false;
	}

	ImGui::SetNextItemWidth(combo_var_width);
	if (ImGui::BeginCombo(u8"対象", condition.variable_name.c_str()))
	{
		std::vector<std::string> var_names = blackboard->GetRegisteredNames();
		for (size_t n = 0; n < var_names.size(); n++)
		{
			if (ImGui::Selectable(var_names[n].c_str(), condition.variable_name == var_names[n]))
			{
				condition.variable_name = var_names[n];
				is_ui_changed = true;
			}
		}
		ImGui::EndCombo();
	}

	if (condition.variable_name.empty())
	{
		return is_ui_changed;
	}

	//選択された変数の型を判別し、型に応じた演算子・基準値入力UIを描画
	int current_op = static_cast<int>(condition.compart_op);

	bool bool_val = false;
	int int_val = 0;
	float float_val = 0.0f;
	DirectX::XMFLOAT3 vec_val = { 0.0f, 0.0f, 0.0f };

	if (blackboard->TryGetValue(condition.variable_name, bool_val))
	{
		if (current_op >= op_bool_count)
		{
			current_op = 0;
			condition.compart_op = CompareOperator::Equal;
		}

		ImGui::SetNextItemWidth(combo_op_width);
		if (ImGui::Combo(u8"演算子", &current_op, bool_operators, op_bool_count))
		{
			condition.compart_op = static_cast<CompareOperator>(current_op);
			is_ui_changed = true;
		}
		ImGui::SameLine();

		bool bool_check_value = (condition.reference_value != 0.0f);
		if (ImGui::Checkbox(u8"基準値(True/False)", &bool_check_value))
		{
			condition.reference_value = bool_check_value ? 1.0f : 0.0f;
			is_ui_changed = true;
		}
	}
	else if (blackboard->TryGetValue(condition.variable_name, int_val))
	{
		ImGui::SetNextItemWidth(combo_op_width);
		if (ImGui::Combo(u8"演算子", &current_op, all_operators, op_total_count))
		{
			condition.compart_op = static_cast<CompareOperator>(current_op);
			is_ui_changed = true;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(drag_input_width);

		int int_drag_value = static_cast<int>(condition.reference_value);
		if (ImGui::DragInt(u8"基準値", &int_drag_value, int_drag_speed))
		{
			condition.reference_value = static_cast<float>(int_drag_value);
			is_ui_changed = true;
		}
	}
	else if (blackboard->TryGetValue(condition.variable_name, float_val))
	{
		ImGui::SetNextItemWidth(combo_op_width);
		if (ImGui::Combo(u8"演算子", &current_op, all_operators, op_total_count))
		{
			condition.compart_op = static_cast<CompareOperator>(current_op);
			is_ui_changed = true;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(drag_input_width);
		if (ImGui::DragFloat(u8"基準値", &condition.reference_value, float_drag_speed, 0.0f, 0.0f, "%.3f"))
		{
			is_ui_changed = true;
		}
	}
	else if (blackboard->TryGetValue(condition.variable_name, vec_val))
	{
		ImGui::SetNextItemWidth(combo_op_width);
		if (ImGui::Combo(u8"比較演算子", &current_op, all_operators, op_total_count))
		{
			condition.compart_op = static_cast<CompareOperator>(current_op);
			is_ui_changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(drag_vec_width);
		if (ImGui::DragFloat3(u8"基準値(XYZ)", &condition.vector_reference_value.x, float_drag_speed, 0.0f, 0.0f, "%.3f"))
		{
			is_ui_changed = true;
		}
	}
	else
	{
		LOG_WARNING({ u8"エディタ", u8"遷移条件" }, u8"変数 [%s] はブラックボードに未登録、またはUI非対応の型です", condition.variable_name.c_str());
	}

	return is_ui_changed;
}

//確率判定用のImGui入力UI描画
void TransitionConditionEditor::DrawRandomUI(TransitionCondition& condition)
{
	//成功確率パーセンテージのスライダー入力描画
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"メルセンヌ・ツイスタによる確率抽選を行います");
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
void TransitionConditionEditor::DrawDistanceUI(Blackboard* blackboard, TransitionCondition& condition)
{
	if (!blackboard)
	{
		return;
	}

	constexpr float combo_width = 150.0f;
	constexpr float drag_width = 120.0f;
	constexpr float drag_speed = 0.1f;
	constexpr float min_dist = 0.0f;
	constexpr float max_dist = 1000.0f;

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"2つの3D座標ベクトル間の直線距離を測定");

	//自身と対象の座標変数を選択するコンボボックス描画
	ImGui::SetNextItemWidth(combo_width);
	if (ImGui::BeginCombo(u8"自身の座標", condition.variable_name.c_str()))
	{
		std::vector<std::string> var_names = blackboard->GetRegisteredNames();
		for (size_t n = 0; n < var_names.size(); n++)
		{
			if (ImGui::Selectable(var_names[n].c_str(), condition.variable_name == var_names[n]))
			{
				condition.variable_name = var_names[n];
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();

	ImGui::SetNextItemWidth(combo_width);
	if (ImGui::BeginCombo(u8"対象の座標", condition.secondary_variable_name.c_str()))
	{
		std::vector<std::string> var_names = blackboard->GetRegisteredNames();
		for (size_t n = 0; n < var_names.size(); n++)
		{
			if (ImGui::Selectable(var_names[n].c_str(), condition.secondary_variable_name == var_names[n]))
			{
				condition.secondary_variable_name = var_names[n];
			}
		}
		ImGui::EndCombo();
	}

	//最小距離・最大距離の閾値入力UI描画
	ImGui::SetNextItemWidth(drag_width);
	ImGui::DragFloat(u8"最小距離制限", &condition.reference_value, drag_speed, min_dist, max_dist, "%.2f");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(drag_width);
	ImGui::DragFloat(u8"最大距離制限", &condition.param_second, drag_speed, min_dist, max_dist, "%.2f");
}

//割合判定用のImGui入力UI描画
void TransitionConditionEditor::DrawRatioUI(Blackboard* blackboard, TransitionCondition& condition)
{
	const char* all_operators[] = { "==","!=",">","<",">=","<=" };
	constexpr int op_total_count = 6;
	constexpr float combo_var_width = 150.0f;
	constexpr float combo_op_width = 80.0f;
	constexpr float drag_ratio_width = 150.0f;
	constexpr float drag_speed = 0.1f;

	if (!blackboard)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"現在値/最大値から現在の割合(0.0f～1.0f)を算出して判定");

	//現在値と最大値の変数を選択するコンボボックス描画
	ImGui::SetNextItemWidth(combo_var_width);
	if (ImGui::BeginCombo(u8"現在値", condition.variable_name.c_str()))
	{
		std::vector<std::string> var_names = blackboard->GetRegisteredNames();
		for (size_t n = 0; n < var_names.size(); n++)
		{
			if (ImGui::Selectable(var_names[n].c_str(), condition.variable_name == var_names[n]))
			{
				condition.variable_name = var_names[n];
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();

	ImGui::SetNextItemWidth(combo_var_width);
	if (ImGui::BeginCombo(u8"最大値", condition.secondary_variable_name.c_str()))
	{
		std::vector<std::string> var_names = blackboard->GetRegisteredNames();
		for (size_t n = 0; n < var_names.size(); n++)
		{
			if (ImGui::Selectable(var_names[n].c_str(), condition.secondary_variable_name == var_names[n]))
			{
				condition.secondary_variable_name = var_names[n];
			}
		}
		ImGui::EndCombo();
	}

	//比較演算子と基準割合の入力UI描画
	int current_op = static_cast<int>(condition.compart_op);
	ImGui::SetNextItemWidth(combo_op_width);
	if (ImGui::Combo(u8"演算子", &current_op, all_operators, op_total_count))
	{
		condition.compart_op = static_cast<CompareOperator>(current_op);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(drag_ratio_width);
	ImGui::DragFloat(u8"基準割合", &condition.reference_value, drag_speed);
}

//キーボード・マウスの入力を設定するUI描画
void TransitionConditionEditor::DrawInputCheckUI(TransitionCondition& condition)
{
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.9f, 1.0f), u8"入力判定設定");

	//キー入力待機中なら押下されたキー・マウスボタンを検出して文字列保存、通常時は現在の設定名を表示
	if (waiting_for_key_condition == &condition)
	{
		ImGui::Button(u8"入力待機中...(キーまたはマウス右/中をクリック)", ImVec2(-1.0f, 30.0f));

		if (Input::Instance().IsMouseTrigger(Input::mouse_right))
		{
			condition.variable_name = std::to_string(VK_RBUTTON);
			waiting_for_key_condition = nullptr;
		}
		else if (Input::Instance().IsMouseTrigger(Input::mouse_middle))
		{
			condition.variable_name = std::to_string(VK_MBUTTON);
			waiting_for_key_condition = nullptr;
		}
		else
		{
			for (int key_idx = 1; key_idx < 256; key_idx++)
			{
				if (key_idx == VK_LBUTTON || key_idx == VK_RBUTTON || key_idx == VK_MBUTTON ||
					key_idx == VK_CAPITAL || key_idx == VK_KANA || key_idx == VK_KANJI ||
					key_idx == VK_CONVERT || key_idx == VK_NONCONVERT || key_idx == VK_NUMLOCK ||
					key_idx == VK_SCROLL || key_idx == 0xF0 || key_idx == 0xF2 || key_idx == 0xF3 || key_idx == 0xF4)
				{
					continue;
				}

				if (Input::Instance().IsKeyTrigger(key_idx))
				{
					condition.variable_name = std::to_string(key_idx);
					waiting_for_key_condition = nullptr;
					break;
				}
			}
		}
	}
	else
	{
		constexpr size_t key_name_buffer_size = 64;
		char key_name_buffer[key_name_buffer_size] = {};
		int saved_v_key = 0;

		if (!condition.variable_name.empty())
		{
			try
			{
				saved_v_key = std::stoi(condition.variable_name);
			}
			catch (...)
			{
				LOG_WARNING({ u8"エディタ", u8"入力判定" }, u8"保存されているキーコード文字列が不正です: %s", condition.variable_name.c_str());
				saved_v_key = 0;
			}
		}

		if (saved_v_key == VK_LBUTTON)
		{
			sprintf_s(key_name_buffer, sizeof(key_name_buffer), u8"マウス左クリック");
		}
		else if (saved_v_key == VK_RBUTTON)
		{
			sprintf_s(key_name_buffer, sizeof(key_name_buffer), u8"マウス右クリック");
		}
		else if (saved_v_key == VK_MBUTTON)
		{
			sprintf_s(key_name_buffer, sizeof(key_name_buffer), u8"マウスホイールクリック");
		}
		else if (saved_v_key > 0)
		{
			unsigned int scan_code = MapVirtualKeyA(saved_v_key, MAPVK_VK_TO_VSC);
			LONG long_scan_code = static_cast<LONG>(scan_code << 16);
			if (GetKeyNameTextA(long_scan_code, key_name_buffer, sizeof(key_name_buffer)) == 0)
			{
				sprintf_s(key_name_buffer, sizeof(key_name_buffer), "KeyCord: %d", saved_v_key);
			}
		}
		else
		{
			sprintf_s(key_name_buffer, sizeof(key_name_buffer), u8"キー未設定");
		}

		ImGui::Text(u8"キー設定 [");
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		if (ImGui::Button(key_name_buffer, ImVec2(-1.0f, 25.0f)))
		{
			Input::Instance().ClearKeyTriggers();
			waiting_for_key_condition = &condition;
		}
		ImGui::PopStyleColor();

		ImGui::Spacing();
		ImGui::Text(u8"マウス直接設定:");
		ImGui::SameLine();
		if (ImGui::Button(u8"左クリック"))
		{
			condition.variable_name = std::to_string(VK_LBUTTON);
			waiting_for_key_condition = nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"右クリック"))
		{
			condition.variable_name = std::to_string(VK_RBUTTON);
			waiting_for_key_condition = nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"中クリック"))
		{
			condition.variable_name = std::to_string(VK_MBUTTON);
			waiting_for_key_condition = nullptr;
		}
	}

	ImGui::Spacing();

	//入力動作モードの選択コンボボックス描画
	int current_behavior = static_cast<int>(condition.param_second);
	const char* behavior_items[] = {
		u8"押下中 (Press)",
		u8"押下瞬間 (Trigger)",
		u8"未押下 (Not Press)",
		u8"離した瞬間 (Release)"
	};
	constexpr int total_behavior_count = 4;

	ImGui::Text(u8"入力動作モード:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::Combo(u8"##InputBehaviorCombo", &current_behavior, behavior_items, total_behavior_count))
	{
		condition.param_second = static_cast<float>(current_behavior);
	}
}