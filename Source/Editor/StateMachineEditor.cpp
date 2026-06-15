#include "StateMachineEditor.h"

#include "../Serialization/JsonSerializer.h"
#include "../ObjectsRender/Model.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/GameObject.h"
#include "../Graphics/Graphics.h"

#include <imgui.h> 
#include <imgui_internal.h> 
#include <cassert> 
#include <fstream> 
#include <commdlg.h> 

#define _U8(str) reinterpret_cast<const char*>(u8##str) 

//コンストラクタ
StateMachineEditor::StateMachineEditor()
	:selected_state_index(-1) 
	, initial_state_name("") 
	, loaded_model_path("") 
	, selected_target_class_index(0) 
{
	json_serializer = std::make_unique<JsonSerializer>();
	available_animations.clear();
	scannable_class_names.clear();
}

//デストラクタ
StateMachineEditor::~StateMachineEditor() {}

//初期化
void StateMachineEditor::Initialize()
{
	editor_states.clear();
	selected_state_index = -1;
	initial_state_name = "";
	current_project_file_path = ""; 
	selected_transition_src_index = -1; 
	selected_transition_idx = -1; 
	zoom_factor = 1.0f;
}

//描画メインの窓口
void StateMachineEditor::RenderGui(ObjectManager* object_manager) 
{
	ImGui::Begin(_U8("ステートマシンエディタ")); 
	DrawMenuBar(object_manager); 

	ImGui::Text(_U8("読み込み中の3Dモデル : %s"), loaded_model_path.empty() ? _U8("[未ロード]") : loaded_model_path.c_str()); 
	if (!current_project_file_path.empty()) 
	{
		ImGui::Text(_U8("現在の設定ファイルパス : %s"), current_project_file_path.c_str()); 
	}
	ImGui::Separator(); 

	constexpr int split_column_count = 2; 
	ImGui::Columns(split_column_count, "EditorLayoutColumns", true); 

	DrawNodeGraphCanves(); 

	ImGui::NextColumn(); 

	DrawInspectorPane(); 

	ImGui::Columns(1); 
	ImGui::End(); 
}

//上部メニューバーの描画
void StateMachineEditor::DrawMenuBar(ObjectManager* object_manager) 
{
	if (ImGui::Button(_U8("新規プロジェクト"))) 
	{
		Initialize(); 
	}
	ImGui::SameLine(); 

	if (ImGui::Button(_U8("プレビューモデル読み込み"))) 
	{
		std::string model_path = OpenFileDialog(false, _U8("3Dモデルアセット (*.glb;*.gltf)\0*.glb;*.gltf\0"));
		if (!model_path.empty()) 
		{
			LoadPreviewModel(model_path); 
		}
	}
	ImGui::SameLine(); 

	if (ImGui::Button(_U8("JSONを開く"))) 
	{
		std::string selected_path = OpenFileDialog(false, _U8("ステートマシン設定ファイル (*.json)\0*.json\0")); 
		if (!selected_path.empty()) 
		{
			LoadEditorData(selected_path); 
		}
	}
	ImGui::SameLine(); 

	if (ImGui::Button(_U8("保存"))) 
	{
		if (current_project_file_path.empty()) 
		{
			std::string new_save_path = OpenFileDialog(true, _U8("ステートマシン設定ファイル (*.json)\0*.json\0")); 
			if (!new_save_path.empty()) 
			{
				SaveEditorData(new_save_path); 
			}
		}
		else 
		{
			SaveEditorData(current_project_file_path); 
		}
	}
	ImGui::SameLine(); 

	if (ImGui::Button(_U8("名前を付けて保存"))) 
	{
		std::string save_path = OpenFileDialog(true, _U8("ステートマシン設定ファイル (*.json)\0*.json\0")); 
		if (!save_path.empty()) 
		{
			SaveEditorData(save_path); 
		}
	}
	ImGui::Separator(); 

	if (object_manager) 
	{
		ImGui::Text(_U8("[シーンオブジェクトへの適用・ホットリロード]"));

		if (ImGui::Button(_U8("シーン上の有効なクラスを検索")))
		{
			scannable_class_names.clear();
			for (const std::unique_ptr<GameObject>& obj_ptr : object_manager->GetGameObjects())
			{
				if (!obj_ptr) continue;
				const std::string& name = obj_ptr->GetClassName();

				if (!name.empty() && std::find(scannable_class_names.begin(), scannable_class_names.end(), name) == scannable_class_names.end())
				{
					scannable_class_names.push_back(name);
				}
			}
		}
		ImGui::SameLine();

		if (!scannable_class_names.empty()) 
		{
			std::vector<const char*> items; 
			for (const auto& c_name : scannable_class_names) 
			{
				items.push_back(c_name.c_str()); 
			}
			ImGui::SetNextItemWidth(200.0f); 
			ImGui::Combo(_U8("対象クラス"), &selected_target_class_index, items.data(), static_cast<int>(items.size())); 
			ImGui::SameLine(); 

			if (ImGui::Button(_U8("適用 & ホットリロード実行"))) 
			{
				if (!current_project_file_path.empty()) 
				{
					SaveEditorData(current_project_file_path); 
					ApplyStateMachineToClass(object_manager, scannable_class_names[selected_target_class_index]); 
				}
				else 
				{
					assert(false && "Please save your project configuration (.json) before injecting into a class."); 
				}
			}
		}
	}
}

//キャンバス空間（グリッド・入力検出）の描画
void StateMachineEditor::DrawNodeGraphCanves() 
{
	ImGui::Text(_U8("【ビジュアル・ノードグラフ】 (マウス中ボタンドラッグ: 画面スクロール / 右クリック: メニュー展開)")); 

	ImGui::BeginChild("CanvasChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar); 

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos(); 
	ImVec2 canvas_size = ImGui::GetContentRegionAvail(); 
	ImDrawList* draw_list = ImGui::GetWindowDrawList(); 

	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255)); 

	//マウスホイールによる拡大縮小倍率の更新処理
	if (ImGui::IsWindowHovered())
	{
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f)
		{
			ImVec2 mouse_graph_pos;

			//ズームする前のマウスのグラフ内相対座標を計算
			mouse_graph_pos.x = (ImGui::GetMousePos().x - canvas_pos.x - scrolling_offset.x) / zoom_factor;
			mouse_graph_pos.y = (ImGui::GetMousePos().y - canvas_pos.y - scrolling_offset.y) / zoom_factor;

			//ズーム倍率を変更
			constexpr float ZOOM_SPEED = 0.05f;
			constexpr float ZOOM_MIN = 0.3f;
			constexpr float ZOOM_MAX = 2.0f;
			zoom_factor += wheel * ZOOM_SPEED;
			if (zoom_factor < ZOOM_MIN) zoom_factor = ZOOM_MIN;
			if (zoom_factor > ZOOM_MAX) zoom_factor = ZOOM_MAX;

			//マウスカーソルの位置を中心に拡大縮小するよう、スクロールオフセットを補正
			scrolling_offset.x = ImGui::GetMousePos().x - canvas_pos.x - mouse_graph_pos.x * zoom_factor;
			scrolling_offset.y = ImGui::GetMousePos().y - canvas_pos.y - mouse_graph_pos.y * zoom_factor;
		}
	}

	constexpr float grid_space = 64.0f; 
	float scroll_x = fmodf(scrolling_offset.x, grid_space); 
	float scroll_y = fmodf(scrolling_offset.y, grid_space); 

	for (float x = scroll_x; x < canvas_size.x; x += grid_space) 
	{
		draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y), ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y), IM_COL32(60, 60, 60, 255)); 
	}
	for (float y = scroll_y; y < canvas_size.y; y += grid_space) 
	{
		draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y), IM_COL32(60, 60, 60, 255)); 
	}

	ImGui::PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true); 

	DrawGraphTransitions(draw_list, canvas_pos); 
	DrawGraphNodes(draw_list, canvas_pos); 

	if (link_source_node_index != -1) 
	{
		ImVec2 src_pos = ImVec2(canvas_pos.x + editor_states[link_source_node_index].graph_position.x + scrolling_offset.x + 140.0f, 
			canvas_pos.y + editor_states[link_source_node_index].graph_position.y + scrolling_offset.y + 25.0f); 
		draw_list->AddLine(src_pos, ImGui::GetMousePos(), IM_COL32(255, 255, 0, 255), 3.0f); 

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
		{
			link_source_node_index = -1; 
		}
	}

	ImGui::PopClipRect(); 

	if (ImGui::IsWindowHovered()) 
	{
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) 
		{
			scrolling_offset.x += ImGui::GetIO().MouseDelta.x; 
			scrolling_offset.y += ImGui::GetIO().MouseDelta.y; 
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) 
		{
			ImGui::OpenPopup("CanvasContextMenu"); 
		}
	}

	if (ImGui::BeginPopup("CanvasContextMenu")) 
	{
		if (ImGui::MenuItem(_U8("新規ステートノードを追加(配置)"))) 
		{
			StateNodeData new_node; 
			new_node.state_name = "NewState_" + std::to_string(editor_states.size()); 
			new_node.animation_clip_name = "Idle"; 
			new_node.is_animation_loop = true; 

			new_node.graph_position.x = ImGui::GetIO().MouseClickedPos[1].x - canvas_pos.x - scrolling_offset.x; 
			new_node.graph_position.y = ImGui::GetIO().MouseClickedPos[1].y - canvas_pos.y - scrolling_offset.y; 

			editor_states.push_back(new_node); 
			selected_state_index = static_cast<int>(editor_states.size()) - 1; 
		}
		ImGui::EndPopup(); 
	}
	ImGui::EndChild(); 
}

//四角形ノード本体の描画とマウスドラッグ移動・削除処理
void StateMachineEditor::DrawGraphNodes(ImDrawList* draw_list, ImVec2 canvas_pos) 
{
	// 基準サイズ（等倍時）
	constexpr float BASE_NODE_WIDTH = 200.0f;
	constexpr float BASE_PIN_RADIUS = 5.0f;
	constexpr float BASE_LINE_HEIGHT = 20.0f;

	//現在のズーム倍率を適用したサイズを計算
	const float node_width = BASE_NODE_WIDTH * zoom_factor;
	const float pin_radius = BASE_PIN_RADIUS * zoom_factor;
	const float line_height = BASE_LINE_HEIGHT * zoom_factor;

	ImVec2 mouse_pos = ImGui::GetMousePos();
	bool mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

	int target_connect_node_idx = -1;

	//ズーム倍率を考慮した各ノードの座標・サイズ算出と描画
	for (size_t i = 0; i < editor_states.size(); i++)
	{
		StateNodeData& node = editor_states[i];

		int current_input_connection_count = 0;
		for (const auto& src_s : editor_states)
		{
			for (const auto& trans : src_s.transitions)
			{
				if (trans.next_state_name == node.state_name)
				{
					current_input_connection_count++;
				}
			}
		}

		int input_pin_display_count = (current_input_connection_count > 0) ? current_input_connection_count + 1 : 1;

		// 縦幅もズーム倍率を考慮
		float node_height = (60.0f + (input_pin_display_count * BASE_LINE_HEIGHT)) * zoom_factor;

		// 座標値の決定に zoom_factor を掛け合わせる
		ImVec2 node_rect_min = ImVec2(
			canvas_pos.x + scrolling_offset.x + node.graph_position.x * zoom_factor,
			canvas_pos.y + scrolling_offset.y + node.graph_position.y * zoom_factor
		);
		ImVec2 node_rect_max = ImVec2(node_rect_min.x + node_width, node_rect_min.y + node_height);

		ImU32 node_bg_color = IM_COL32(50, 50, 50, 255);
		if (static_cast<int>(i) == selected_state_index) node_bg_color = IM_COL32(80, 80, 110, 255);
		if (node.state_name == initial_state_name) node_bg_color = IM_COL32(50, 90, 50, 255);

		draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 8.0f * zoom_factor);
		draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 8.0f * zoom_factor, 0, 1.5f * zoom_factor);

		// テキスト描画の位置もスケール
		ImVec2 text_pos = ImVec2(node_rect_min.x + 12.0f * zoom_factor, node_rect_min.y + 10.0f * zoom_factor);
		draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), node.state_name.c_str());

		ImVec2 clip_pos = ImVec2(node_rect_min.x + 12.0f * zoom_factor, node_rect_min.y + 30.0f * zoom_factor);
		draw_list->AddText(clip_pos, IM_COL32(180, 180, 180, 255), node.animation_clip_name.c_str());

		draw_list->AddLine(
			ImVec2(node_rect_min.x, node_rect_min.y + 50.0f * zoom_factor),
			ImVec2(node_rect_max.x, node_rect_min.y + 50.0f * zoom_factor),
			IM_COL32(80, 80, 80, 255)
		);

		float pin_area_start_y = node_rect_min.y + 55.0f * zoom_factor;

		// 出力ピン
		ImVec2 output_pin_pos = ImVec2(node_rect_max.x - 20.0f * zoom_factor, pin_area_start_y + pin_radius + 2.0f * zoom_factor);
		std::string output_pin_id = "##out_pin_" + std::to_string(i);

		ImVec2 out_text_pos = ImVec2(output_pin_pos.x - 35.0f * zoom_factor, pin_area_start_y);
		// 縮小されすぎている時はテキストを非表示にして見映えを維持
		if (zoom_factor > 0.5f)
		{
			draw_list->AddText(out_text_pos, IM_COL32(230, 230, 230, 255), _U8("出力"));
		}

		ImGui::SetCursorScreenPos(ImVec2(output_pin_pos.x - pin_radius * 2.0f, output_pin_pos.y - pin_radius * 2.0f));
		ImGui::InvisibleButton(output_pin_id.c_str(), ImVec2(pin_radius * 4.0f, pin_radius * 4.0f));

		bool output_hovered = ImGui::IsItemHovered();
		ImU32 out_pin_color = output_hovered ? IM_COL32(255, 200, 50, 255) : IM_COL32(200, 200, 200, 255);
		draw_list->AddCircleFilled(output_pin_pos, pin_radius, out_pin_color);

		if (ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			link_source_node_index = static_cast<int>(i);
		}

		// 入力ピン
		for (int p = 0; p < input_pin_display_count; p++)
		{
			float current_pin_y = pin_area_start_y + (p * line_height) + pin_radius + 2.0f * zoom_factor;
			ImVec2 input_pin_pos = ImVec2(node_rect_min.x + 20.0f * zoom_factor, current_pin_y);

			if (zoom_factor > 0.5f)
			{
				ImVec2 in_text_pos = ImVec2(input_pin_pos.x + 12.0f * zoom_factor, pin_area_start_y + (p * line_height));
				draw_list->AddText(in_text_pos, IM_COL32(230, 230, 230, 255), _U8("入力"));
			}

			// 吸着距離の判定もズームに合わせて伸縮
			float dist_to_pin_sq = (mouse_pos.x - input_pin_pos.x) * (mouse_pos.x - input_pin_pos.x) + (mouse_pos.y - input_pin_pos.y) * (mouse_pos.y - input_pin_pos.y);
			float active_hit_radius = 14.0f * zoom_factor;
			if (active_hit_radius < 6.0f) active_hit_radius = 6.0f; // 縮小時も最低限のクリック判定を確保
			bool is_mouse_over_pin = (dist_to_pin_sq <= (active_hit_radius * active_hit_radius));

			ImU32 in_pin_color = is_mouse_over_pin ? IM_COL32(50, 255, 50, 255) : IM_COL32(150, 150, 150, 255);
			draw_list->AddCircleFilled(input_pin_pos, pin_radius, in_pin_color);

			if (link_source_node_index != -1 && link_source_node_index != static_cast<int>(i))
			{
				if (is_mouse_over_pin && mouse_released)
				{
					target_connect_node_idx = static_cast<int>(i);
				}
			}
		}

		// ノードの移動用ボディボックス
		ImGui::SetCursorScreenPos(node_rect_min);
		std::string body_id = "##node_body_" + std::to_string(i);

		if (ImGui::InvisibleButton(body_id.c_str(), ImVec2(node_width, 50.0f * zoom_factor)))
		{
			selected_state_index = static_cast<int>(i);
			selected_transition_src_index = -1;
			selected_transition_idx = -1;
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && link_source_node_index == -1)
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			//マウスの移動量をズーム倍率で割ることで、拡大時でもドラッグ速度がズレないように補正
			node.graph_position.x += delta.x / zoom_factor;
			node.graph_position.y += delta.y / zoom_factor;
		}
	}

	//ドラッグ中における予測線のズーム対応描画
	if (link_source_node_index != -1)
	{
		if (mouse_released)
		{
			if (target_connect_node_idx != -1)
			{
				TransitionNodeData new_link;
				new_link.next_state_name = editor_states[target_connect_node_idx].state_name;
				new_link.condition_type = ConditionType::InputLength;
				new_link.operator_type = ConditionOp::Equal;
				new_link.compare_value = 0.0f;
				new_link.target_action_name = "";
				new_link.blend_mode = TransitionBlendMode::CombineWithCommon;

				editor_states[link_source_node_index].transitions.push_back(new_link);
			}
			link_source_node_index = -1;
		}
		else
		{
			StateNodeData& src_node = editor_states[link_source_node_index];
			float src_pin_area_y = canvas_pos.y + scrolling_offset.y + (src_node.graph_position.y + 55.0f) * zoom_factor;
			ImVec2 src_pin = ImVec2(
				canvas_pos.x + scrolling_offset.x + (src_node.graph_position.x + BASE_NODE_WIDTH) * zoom_factor - 20.0f * zoom_factor,
				src_pin_area_y + pin_radius + 2.0f * zoom_factor
			);

			draw_list->AddBezierCurve(
				src_pin,
				ImVec2(src_pin.x + 30.0f * zoom_factor, src_pin.y),
				ImVec2(mouse_pos.x - 30.0f * zoom_factor, mouse_pos.y),
				mouse_pos,
				IM_COL32(255, 255, 0, 150),
				2.0f * zoom_factor
			);
		}
	}
}

//ノード同士を繋ぐ矢印線の描画とクリック選択・削除
void StateMachineEditor::DrawGraphTransitions(ImDrawList* draw_list, ImVec2 canvas_pos) 
{
	constexpr float BASE_NODE_WIDTH = 200.0f;
	constexpr float BASE_PIN_RADIUS = 5.0f;
	constexpr float BASE_LINE_HEIGHT = 20.0f;

	const float node_width = BASE_NODE_WIDTH * zoom_factor;
	const float pin_radius = BASE_PIN_RADIUS * zoom_factor;
	const float line_height = BASE_LINE_HEIGHT * zoom_factor;

	int trans_src_to_delete = -1;
	int trans_idx_to_delete = -1;

	//確定している全ての遷移線のズーム対応描画
	for (size_t i = 0; i < editor_states.size(); i++)
	{
		const StateNodeData& src_node = editor_states[i];

		float src_pin_y = canvas_pos.y + scrolling_offset.y + (src_node.graph_position.y + 55.0f) * zoom_factor + pin_radius + 2.0f * zoom_factor;
		ImVec2 p_start = ImVec2(canvas_pos.x + scrolling_offset.x + (src_node.graph_position.x + BASE_NODE_WIDTH) * zoom_factor - 20.0f * zoom_factor, src_pin_y);

		for (size_t t = 0; t < src_node.transitions.size(); t++)
		{
			const auto& trans = src_node.transitions[t];
			int input_slot_index = 0;

			for (size_t j = 0; j < editor_states.size(); j++)
			{
				const StateNodeData& dest_node = editor_states[j];

				if (dest_node.state_name == trans.next_state_name)
				{
					int current_match_idx = 0;
					for (size_t check_i = 0; check_i < editor_states.size(); check_i++)
					{
						const auto& s_node = editor_states[check_i];
						for (size_t check_t = 0; check_t < s_node.transitions.size(); check_t++)
						{
							if (s_node.transitions[check_t].next_state_name == dest_node.state_name)
							{
								if (check_i == i && check_t == t)
								{
									input_slot_index = current_match_idx;
									break;
								}
								current_match_idx++;
							}
						}
					}

					float dest_pin_y = canvas_pos.y + scrolling_offset.y + (dest_node.graph_position.y + 55.0f) * zoom_factor + (input_slot_index * line_height) + pin_radius + 2.0f * zoom_factor;
					ImVec2 p_end = ImVec2(canvas_pos.x + scrolling_offset.x + dest_node.graph_position.x * zoom_factor + 20.0f * zoom_factor, dest_pin_y);

					ImVec2 cp1 = ImVec2(p_start.x + 40.0f * zoom_factor, p_start.y);
					ImVec2 cp2 = ImVec2(p_end.x - 40.0f * zoom_factor, p_end.y);

					ImU32 line_color = (selected_transition_src_index == static_cast<int>(i) && selected_transition_idx == static_cast<int>(t)) ? IM_COL32(255, 150, 0, 255) : IM_COL32(200, 200, 200, 255);

					draw_list->AddBezierCurve(p_start, cp1, cp2, p_end, line_color, 3.0f * zoom_factor);

					// 矢印の頭（三角形）の大きさもズームスケールさせる
					draw_list->AddTriangleFilled(
						p_end,
						ImVec2(p_end.x - 6.0f * zoom_factor, p_end.y - 5.0f * zoom_factor),
						ImVec2(p_end.x - 6.0f * zoom_factor, p_end.y + 5.0f * zoom_factor),
						IM_COL32(255, 255, 100, 255)
					);

					// ヒットボックス判定もズームに応じてスケーリング
					ImVec2 p_mid = ImVec2((p_start.x + p_end.x) * 0.5f, (p_start.y + p_end.y) * 0.5f);
					ImVec2 m_pos = ImGui::GetMousePos();
					float distance_sq = (m_pos.x - p_mid.x) * (m_pos.x - p_mid.x) + (m_pos.y - p_mid.y) * (m_pos.y - p_mid.y);

					float hit_radius = 12.0f * zoom_factor;
					if (hit_radius < 5.0f) hit_radius = 5.0f;
					if (distance_sq < (hit_radius * hit_radius))
					{
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						{
							selected_transition_src_index = static_cast<int>(i);
							selected_transition_idx = static_cast<int>(t);
							selected_state_index = -1;
						}
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
						{
							selected_transition_src_index = static_cast<int>(i);
							selected_transition_idx = static_cast<int>(t);
							ImGui::OpenPopup(_U8("TransitionContextMenu"));
						}
					}
				}
			}
		}
	}

	if (ImGui::BeginPopup(_U8("TransitionContextMenu")))
	{
		if (ImGui::MenuItem(_U8("この矢印（接続パス）を削除")))
		{
			trans_src_to_delete = selected_transition_src_index;
			trans_idx_to_delete = selected_transition_idx;
		}
		ImGui::EndPopup();
	}

	if (trans_src_to_delete != -1 && trans_idx_to_delete != -1)
	{
		editor_states[trans_src_to_delete].transitions.erase(editor_states[trans_src_to_delete].transitions.begin() + trans_idx_to_delete);
		selected_transition_src_index = -1;
		selected_transition_idx = -1;
	}
}

//プロ/ティインスペクター詳細パネルの描画 --
void StateMachineEditor::DrawInspectorPane() 
{
	const char* type_names[] = { _U8("移動入力の長さ (InputLength)"), _U8("ボタンビットフラグ判定 (ButtonCommand)"), _U8("個別アクション入力検知 (ActionPressed)"), _U8("接地状態フラグ (IsGrounded)"), _U8("目標移動速度 (TargetMoveSpeed)") }; 
	const char* op_names[] = { _U8("一致 (==)"), _U8("不一致 (!=)"), _U8("より大きい (>)"), _U8("未満 (<)"), _U8("以上 (>=)"), _U8("以下 (<=)") }; 
	const char* blend_names[] = { _U8("共通条件と組み合わせる (AND)"), _U8("個別条件のみを適用する (Override)") }; 

	 //矢印が選択されている場合
	if (selected_transition_src_index >= 0 && selected_transition_idx >= 0) 
	{
		ImGui::Text(_U8("【選択中の接続矢印 プロパティ設定】")); 
		TransitionNodeData& trans = editor_states[selected_transition_src_index].transitions[selected_transition_idx]; 

		ImGui::Text(_U8("出発元: %s"), editor_states[selected_transition_src_index].state_name.c_str()); 
		ImGui::Text(_U8("遷移先ターゲット: %s"), trans.next_state_name.c_str()); 
		ImGui::Separator(); 

		int current_blend = static_cast<int>(trans.blend_mode); 
		if (ImGui::Combo(_U8("評価ブレンドモード"), &current_blend, blend_names, IM_ARRAYSIZE(blend_names))) 
		{
			trans.blend_mode = static_cast<TransitionBlendMode>(current_blend); 
		}

		int trans_type = static_cast<int>(trans.condition_type); 
		if (ImGui::Combo(_U8("個別比較対象"), &trans_type, type_names, IM_ARRAYSIZE(type_names))) 
		{
			trans.condition_type = static_cast<ConditionType>(trans_type); 
		}

		int trans_op = static_cast<int>(trans.operator_type); 
		if (ImGui::Combo(_U8("条件演算子"), &trans_op, op_names, IM_ARRAYSIZE(op_names))) 
		{
			trans.operator_type = static_cast<ConditionOp>(trans_op); 
		}

		if (trans.condition_type == ConditionType::ActionPressed) 
		{
			char action_name_buf[64]; 
			strncpy_s(action_name_buf, sizeof(action_name_buf), trans.target_action_name.c_str(), _TRUNCATE); 
			if (ImGui::InputText(_U8("登録アクション名"), action_name_buf, sizeof(action_name_buf))) 
			{
				trans.target_action_name = action_name_buf; 
			}
		}
		else 
		{
			ImGui::DragFloat(_U8("比較基準値"), &trans.compare_value, 0.01f); 
		}

		if (ImGui::Button(_U8("この接続パス（矢印）を削除"), ImVec2(-1, 0))) 
		{
			editor_states[selected_transition_src_index].transitions.erase(editor_states[selected_transition_src_index].transitions.begin() + selected_transition_idx); 
			selected_transition_src_index = -1; 
			selected_transition_idx = -1; 
		}
		return;
	}

	//ステートが選択されている場合
	ImGui::Text(_U8("【詳細プロパティ設定インスペクター】")); 

	if (selected_state_index < 0 || selected_state_index >= static_cast<int>(editor_states.size())) 
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), _U8("グラフからノード、または接続矢印を選択すると編集できます。")); 
		return;
	}

	StateNodeData& target_node = editor_states[selected_state_index]; 

	char name_buf[64]; 
	strncpy_s(name_buf, sizeof(name_buf), target_node.state_name.c_str(), _TRUNCATE); 
	if (ImGui::InputText(_U8("ステート名"), name_buf, sizeof(name_buf))) 
	{
		target_node.state_name = name_buf; 
	}

	if (!available_animations.empty()) 
	{
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

		if (ImGui::Combo(_U8("再生アニメーション"), &current_anim_index, combo_items.data(), static_cast<int>(combo_items.size()))) 
		{
			target_node.animation_clip_name = available_animations[current_anim_index]; 
		}
	}

	if (ImGui::Button(_U8("この状態をゲーム開始時の初期ステートに設定"))) 
	{
		initial_state_name = target_node.state_name; 
	}

	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
	if (ImGui::Button(_U8("ステートを削除する"), ImVec2(-1, 30.0f)))
	{
		std::string delete_target_name = target_node.state_name;
		for (auto& state : editor_states)
		{
			for (auto trans_it = state.transitions.begin(); trans_it != state.transitions.end();)
			{
				if (trans_it->next_state_name == delete_target_name)
				{
					trans_it = state.transitions.erase(trans_it);
				}
				else
				{
					trans_it++;
				}
			}
		}
		editor_states.erase(editor_states.begin() + selected_state_index);
		selected_state_index = -1; 
		ImGui::PopStyleColor();
		return;
	}
	ImGui::PopStyleColor();

	ImGui::Separator(); 

	//共通遷移条件（グループ設定）のエディタ描画
	ImGui::TextColored(ImVec4(0.3f, 1.0f, 1.0f, 1.0f), _U8("【 ステート共通の出力遷移条件グループ 】")); 
	ImGui::Checkbox(_U8("このステートからの出力に共通条件を強制する"), &target_node.has_common_condition); 

	if (target_node.has_common_condition) 
	{
		ImGui::Indent(); 

		int c_type = static_cast<int>(target_node.common_condition_type); 
		if (ImGui::Combo(_U8("共通比較対象"), &c_type, type_names, IM_ARRAYSIZE(type_names))) 
		{
			target_node.common_condition_type = static_cast<ConditionType>(c_type); 
		}

		int c_op = static_cast<int>(target_node.common_operator_type); 
		if (ImGui::Combo(_U8("共通条件演算子"), &c_op, op_names, IM_ARRAYSIZE(op_names))) 
		{
			target_node.common_operator_type = static_cast<ConditionOp>(c_op); 
		}

		if (target_node.common_condition_type == ConditionType::ActionPressed) 
		{
			char act_buf[64]; 
			strncpy_s(act_buf, sizeof(act_buf), target_node.common_target_action_name.c_str(), _TRUNCATE); 
			if (ImGui::InputText(_U8("共通登録アクション名"), act_buf, sizeof(act_buf))) 
			{
				target_node.common_target_action_name = act_buf; 
			}
		}
		else 
		{
			ImGui::DragFloat(_U8("共通比較基準値"), &target_node.common_compare_value, 0.01f); 
		}
		ImGui::Unindent(); 
	}
}

//Windows標準のファイルダイアログ取得ロジック
std::string StateMachineEditor::OpenFileDialog(bool is_save_mode, const char* filter) 
{
	char file_buffer[MAX_PATH] = ""; 
	OPENFILENAMEA ofn = {}; 
	ofn.lStructSize = sizeof(OPENFILENAMEA); 
	ofn.lpstrFilter = filter; 
	ofn.lpstrFile = file_buffer; 
	ofn.nMaxFile = MAX_PATH; 
	ofn.lpstrInitialDir = "DataJson"; 
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (is_save_mode) 
	{
		ofn.Flags |= OFN_OVERWRITEPROMPT; 
		return GetSaveFileNameA(&ofn) ? std::string(file_buffer) : ""; 
	}
	ofn.Flags |= OFN_FILEMUSTEXIST; 
	return GetOpenFileNameA(&ofn) ? std::string(file_buffer) : ""; 
}

//シリアライズ保存
void StateMachineEditor::SaveEditorData(const std::string& file_path) 
{
	nlohmann::json root_json; 
	root_json["initial_state"] = initial_state_name; 

	nlohmann::json states_array = nlohmann::json::array(); 
	for (const auto& node : editor_states) 
	{
		nlohmann::json node_json; 
		node_json["name"] = node.state_name; 
		node_json["clip_name"] = node.animation_clip_name; 
		node_json["is_loop"] = node.is_animation_loop; 
		node_json["has_common_condition"] = node.has_common_condition; 
		node_json["common_condition_type"] = static_cast<int>(node.common_condition_type); 
		node_json["common_operator_type"] = static_cast<int>(node.common_operator_type); 
		node_json["common_compare_value"] = node.common_compare_value; 
		node_json["common_target_action_name"] = node.common_target_action_name; 

		nlohmann::json trans_array = nlohmann::json::array(); 
		for (const auto& trans : node.transitions) 
		{
			nlohmann::json trans_json; 
			trans_json["next_state"] = trans.next_state_name; 
			trans_json["condition_type"] = static_cast<int>(trans.condition_type); 
			trans_json["operator_type"] = static_cast<int>(trans.operator_type); 
			trans_json["compare_value"] = trans.compare_value; 
			trans_json["target_action_name"] = trans.target_action_name; 
			trans_json["blend_mode"] = static_cast<int>(trans.blend_mode); 
			trans_array.push_back(trans_json); 
		}
		node_json["transitions"] = trans_array; 
		states_array.push_back(node_json); 
	}
	root_json["states"] = states_array; 

	std::ofstream output_file(file_path); 
	if (output_file.is_open()) 
	{
		constexpr int json_indent_space = 4; 
		output_file << root_json.dump(json_indent_space); 
		output_file.close(); 
		current_project_file_path = file_path; 
	}
	else 
	{
		assert(false && "StateMachineEditor::SaveEditorData - Failed to open output file path."); 
	}
}

//シリアライズ読み込み
void StateMachineEditor::LoadEditorData(const std::string& file_path) 
{
	std::ifstream input_file(file_path); 
	if (!input_file.is_open()) return; 

	nlohmann::json root_json; 
	input_file >> root_json; 
	input_file.close(); 

	Initialize(); 

	if (root_json.contains("initial_state")) 
	{
		initial_state_name = root_json["initial_state"].get<std::string>(); 
	}

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

					if (trans_json.contains("blend_mode"))
					{
						trans.blend_mode = static_cast<TransitionBlendMode>(trans_json["blend_mode"].get<int>());
					}
					node.transitions.push_back(trans); 
				}
			}
			editor_states.push_back(node);
		}
	}
	current_project_file_path = file_path;
}

//プレビュー用3Dモデルの割り当て
void StateMachineEditor::LoadPreviewModel(const std::string& glb_path)
{
	auto device = Graphics::Instance().GetDevice();
	preview_model = std::make_unique<Model>(device, glb_path);
	if (preview_model)
	{
		loaded_model_path = glb_path; 
		SetAvailableAnimations(preview_model->GetAnimationNames());
	}
}

//実行中のコンポーネント指向オブジェクトへの動的ホットリロード適用
void StateMachineEditor::ApplyStateMachineToClass(ObjectManager* object_manager, const std::string& target_class_name)
{
	assert(object_manager != nullptr && "StateMachineEditor::ApplyStateMachineToClass - ObjectManager is null.");
	if (!object_manager) return;

	int inject_count = 0;
	for (const std::unique_ptr<GameObject>& obj_ptr : object_manager->GetGameObjects())
	{
		GameObject* obj = obj_ptr.get();
		if (obj && obj->GetClassName() == target_class_name)
		{
			obj->LoadStateMachineConfig(current_project_file_path);
			inject_count++; 
		}
	}
	std::string debug_msg = "[Editor] Injected state machine to " + std::to_string(inject_count) + " instances of " + target_class_name + ".\n";
	OutputDebugStringA(debug_msg.c_str());
}