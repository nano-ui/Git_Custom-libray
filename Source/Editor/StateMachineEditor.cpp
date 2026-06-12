#include "StateMachineEditor.h"

#include "../Serialization/JsonSerializer.h"

#include <imgui.h>
#include <cassert>
#include <fstream>

//コンストラクタ
StateMachineEditor::StateMachineEditor()
	:selected_state_index(-1)
	,initial_state_name("")
{
	json_serializer = std::make_unique<JsonSerializer>();
}

//デストラクタ
StateMachineEditor::~StateMachineEditor()
{

}

//初期化
void StateMachineEditor::Initialize()
{
	editor_states.clear();
	selected_state_index = -1;
	initial_state_name = "";
}

//描画
void StateMachineEditor::RenderGui()
{
	//メインのエディタウィンドウを構築
	ImGui::Begin("State Machine Editor");

	//上部メニューバーの描画
	DrawMenuBar();

	//画面を左右に分割するためのテーブルレイアウト
	constexpr int split_column_count = 2;
	ImGui::Columns(split_column_count, "EditorLayoutColumns", true);

	//ステート一覧
	DrawStateListPane();

	ImGui::NextColumn();

	//インスペクター（詳細設定）
	DrawInspectorPane();

	//画面分割モードを終了して通常の１列レイアウトに戻す
	ImGui::Columns(1);

	ImGui::End();
}

//メニューバー
void StateMachineEditor::DrawMenuBar()
{
	if (ImGui::Button("New Project"))
	{
		Initialize();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load JSON"))
	{
		LoadEditorData("Data/Script/StateMachine/Player.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Save JSON"))
	{
		SaveEditorData("Data/Script/StateMachine/Player.json");
	}
	ImGui::Separator();
}

//登録ステートの一覧・追加・削除
void StateMachineEditor::DrawStateListPane()
{
	ImGui::Text("[States List]");

	//ステートの新規追加ボタン
	if (ImGui::Button("Add New State", ImVec2(-1, 0)))
	{
		StateNodeData new_node;
		new_node.state_name = "NewState_" + std::to_string(editor_states.size());
		new_node.animation_clip_name = "Idle";
		new_node.is_animation_loop = true;

		editor_states.push_back(new_node);
		selected_state_index = static_cast<int>(editor_states.size()) - 1;
	}

	ImGui::Spacing();

	//子ウィンドウを作成してスクロール可能なリストにする
	ImGui::BeginChild("StateListChild", ImVec2(0, 0), true);
	for (size_t i = 0; i < editor_states.size(); i++)
	{
		std::string label = editor_states[i].state_name;

		//初期起動ステートなら分かりやすくマーキング
		if (label == initial_state_name)
		{
			label += " [Initial]";
		}

		//リストアイテムが選択されたらインデックスを更新
		if (ImGui::Selectable(label.c_str(), selected_state_index == static_cast<int>(i)))
		{
			selected_state_index = static_cast<int>(i);
		}
	}
	ImGui::EndChild();
}

//選択中のステート・遷移条件のプロパティ編集
void StateMachineEditor::DrawInspectorPane()
{
	ImGui::Text("[Inspector]");

	//編集ターゲットが選ばれていない場合は警告を表示して処理を抜ける
	if (selected_state_index < 0 || selected_state_index >= static_cast<int>(editor_states.size()))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Select a state from the left list to edit.");
		return;
	}

	//編集中のノードの参照を取得
	StateNodeData& target_node = editor_states[selected_state_index];

	json_serializer = std::make_unique<JsonSerializer>();
	json_serializer->RegisterVariable("State Name", &target_node.state_name);
	json_serializer->RegisterVariable("Loop Animation", &target_node.is_animation_loop);

	//基本情報の一括描画
	json_serializer->RenderGui();

	//自動取得したアニメーション名の一覧をコンボボックス化
	if (available_animations.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Warning: No animations loaded from model.");
	}
	else
	{
		//現在設定されているアニメーション名が、リストの何番目にあるかを検索
		int current_anim_index = 0;
		for (int i = 0; i < static_cast<int>(available_animations.size()); ++i)
		{
			if (available_animations[i] == target_node.animation_clip_name)
			{
				current_anim_index = i;
				break;
			}
		}

		std::vector<const char*> combo_items;
		for (const auto& name : available_animations)
		{
			combo_items.push_back(name.c_str());
		}

		//コンボボックスを表示
		if (ImGui::Combo("Animation Clip", &current_anim_index, combo_items.data(), static_cast<int>(combo_items.size())))
		{
			target_node.animation_clip_name = available_animations[current_anim_index];
		}
	}

	//初期ステートに設定するボタン
	if (ImGui::Button("Set as Initial State"))
	{
		initial_state_name = target_node.state_name;
	}

	ImGui::Separator();
	ImGui::Text("-> Transitions (Arrows)");

	//遷移（矢印）の追加ボタン
	if (ImGui::Button("Add Transition"))
	{
		TransitionNodeData new_trans;
		new_trans.next_state_name = "Idle";
		target_node.transitions.push_back(new_trans);
	}

	//ステートから伸びているすべての遷移条件をループ描画
	for (size_t t = 0; t < target_node.transitions.size(); t++)
	{
		ImGui::PushID(static_cast<int>(t)); //ImGuiのIDバグを防ぐためIDをプッシュ
		ImGui::Separator();

		TransitionNodeData& trans = target_node.transitions[t];

		//遷移先ステートの設定
		char next_state_buf[64];
		strncpy_s(next_state_buf, sizeof(next_state_buf), trans.next_state_name.c_str(), _TRUNCATE);
		if (ImGui::InputText("Next State", next_state_buf, sizeof(next_state_buf)))
		{
			trans.next_state_name = next_state_buf;
		}

		//判定対象タイプ（ConditionType）の簡易コンボ選択欄
		int current_type = static_cast<int>(trans.condition_type);
		const char* type_names[] = { "InputLength", "ButtonCommand", "ActionPressed", "IsGrounded", "TargetMoveSpeed" };
		if (ImGui::Combo("Condition Target", &current_type, type_names, IM_ARRAYSIZE(type_names)))
		{
			trans.condition_type = static_cast<ConditionType>(current_type);
		}

		//演算子（ConditionOp）の簡易コンボ選択欄
		int current_op = static_cast<int>(trans.operator_type);
		const char* op_names[] = { "Equal(==)", "NotEqual(!=)", "GreaterThan(>)", "LessThan(<)", "GreaterEqual(>=)", "LessEqual(<=)" };
		if (ImGui::Combo("Operator", &current_op, op_names, IM_ARRAYSIZE(op_names)))
		{
			trans.operator_type = static_cast<ConditionOp>(current_op);
		}

		//判定対象が「ActionPressed（文字列）」の場合はテキスト欄、それ以外は数値欄を出す
		if (trans.condition_type == ConditionType::ActionPressed)
		{
			char action_name_buf[64];
			strncpy_s(action_name_buf, sizeof(action_name_buf), trans.target_action_name.c_str(), _TRUNCATE);
			if (ImGui::InputText("Target Action Name", action_name_buf, sizeof(action_name_buf)))
			{
				trans.target_action_name = action_name_buf;
			}
		}
		else
		{
			ImGui::DragFloat("Compare Value", &trans.compare_value, 0.01f);
		}

		//削除ボタン
		if (ImGui::Button("Remove Transition"))
		{
			target_node.transitions.erase(target_node.transitions.begin() + t);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
}

//JSONに書き出す
void StateMachineEditor::SaveEditorData(const std::string& file_path)
{
	nlohmann::json root_json;
	root_json["initial_state"] = initial_state_name;

	//すべてのノード情報をnlohmann::jsonの配列オブジェクトへ手動パースして落とし込む
	nlohmann::json states_array = nlohmann::json::array();
	for (const auto& node : editor_states)
	{
		nlohmann::json node_json;
		node_json["name"] = node.state_name;
		node_json["clip_name"] = node.animation_clip_name;
		node_json["is_loop"] = node.is_animation_loop;

		//遷移条件の配列パース
		nlohmann::json trans_array = nlohmann::json::array();
		for (const auto& trans : node.transitions)
		{
			nlohmann::json trans_json;
			trans_json["next_state"] = trans.next_state_name;
			trans_json["condition_type"] = static_cast<int>(trans.condition_type);
			trans_json["operator_type"] = static_cast<int>(trans.operator_type);
			trans_json["compare_value"] = trans.compare_value;
			trans_json["target_action_name"] = trans.target_action_name;
			trans_array.push_back(trans_json);
		}
		node_json["transitions"] = trans_array;
		states_array.push_back(node_json);
	}
	root_json["states"] = states_array;

	//テキストファイルへの書き出し
	std::ofstream output_file(file_path);
	if (output_file.is_open())
	{
		constexpr int json_indent_space = 4;
		output_file << root_json.dump(json_indent_space);
		output_file.close();
	}
	else
	{
		//ファイルが開けなかった場合は意図しない挙動としてデバッグ出力
		assert(false && "StateMachineEditor::SaveEditorData - Failed to open output file path.");
	}
}

//JSONファイルから編集データを読み込む
void StateMachineEditor::LoadEditorData(const std::string& file_path)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		//ファイルがない可能性に対するセーフガード
		return;
	}

	nlohmann::json root_json;
	input_file >> root_json;
	input_file.close();

	//データをクリアして再構築
	Initialize();

	if (root_json.contains("initial_state"))
	{
		initial_state_name = root_json["initial_state"].get<std::string>();
	}

	//JSONノードから配列データを復元
	if (root_json.contains("states") && root_json["states"].is_array())
	{
		for (const auto& node_json : root_json["states"])
		{
			StateNodeData node;
			node.state_name = node_json["name"].get<std::string>();
			node.animation_clip_name = node_json["clip_name"].get<std::string>();
			node.is_animation_loop = node_json["is_loop"].get<bool>();

			if (node_json.contains("transitions") && node_json["transitions"].is_array())
			{
				for (const auto& trans_json : node_json["transitions"])
				{
					TransitionNodeData trans;
					trans.next_state_name = trans_json["next_state"].get<std::string>();
					trans.condition_type = static_cast<ConditionType>(trans_json["condition_type"].get<int>());
					trans.operator_type = static_cast<ConditionOp>(trans_json["operator_type"].get<int>());
					trans.compare_value = trans_json["compare_value"].get<float>();
					trans.target_action_name = trans_json["target_action_name"].get<std::string>();
					node.transitions.push_back(trans);
				}
			}
			editor_states.push_back(node);
		}
	}
}