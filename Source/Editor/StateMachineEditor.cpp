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
	,initial_state_name("")
	,loaded_model_path("")
	,selected_target_class_index(0)
{
	json_serializer = std::make_unique<JsonSerializer>();
	available_animations.clear();
	scannable_class_names.clear();
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
	current_project_file_path = "";
}

//描画
void StateMachineEditor::RenderGui(ObjectManager* object_manager)
{
	//メインのエディタウィンドウを構築
	ImGui::Begin(_U8("ステートマシンエディタ"));

	//上部メニューバーの描画
	DrawMenuBar(object_manager);

	//現在のロード状況を表示
	ImGui::Text(_U8("読み込み中の3Dモデル : %s"), loaded_model_path.empty() ? _U8("[未ロード]") : loaded_model_path.c_str());
	if (!current_project_file_path.empty())
	{
		ImGui::Text(_U8("現在の設定ファイルパス : %s"), current_project_file_path.c_str());
	}
	ImGui::Separator();

	//画面を左右に分割するためのテーブルレイアウト
	constexpr int split_column_count = 2;
	ImGui::Columns(split_column_count, "EditorLayoutColumns", true);

	//ステート一覧
	DrawNodeGraphCanves();

	ImGui::NextColumn();

	//インスペクター（詳細設定）
	DrawInspectorPane();

	//画面分割モードを終了して通常の１列レイアウトに戻す
	ImGui::Columns(1);

	ImGui::End();
}

//メニューバー
void StateMachineEditor::DrawMenuBar(ObjectManager* object_manager)
{
	if (ImGui::Button(_U8("新規プロジェクト")))
	{
		Initialize();
	}

	//モデルをロード
	if (ImGui::Button(_U8("プレビューモデル読み込み")))
	{
		std::string model_path = OpenFileDialog(false, _U8("3Dモデルアセット (*.glb;*.gltf)\0*.glb;*.gltf\0"));
		if (!model_path.empty())
		{
			LoadPreviewModel(model_path);
		}
	}
	ImGui::SameLine();

	ImGui::SameLine();
	if (ImGui::Button(_U8("JSONを開く")))
	{
		std::string selected_path = OpenFileDialog(false, _U8("ステートマシン設定ファイル (*.json)\0*.json\0"));		if (!selected_path.empty())
		{
			LoadEditorData(selected_path);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button(_U8("保存")))
	{
		//編集中のファイルパスが空なら、名前を付けて保存させる
		if (current_project_file_path.empty())
		{
			std::string new_save_path = OpenFileDialog(true, _U8("ステートマシン設定ファイル (*.json)\0*.json\0"));			if (!new_save_path.empty())
			{
				SaveEditorData(new_save_path);
			}
		}
		else
		{
			//既に開いているパスがあるならそのまま上書き保存
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

	//シーン上の該当オブジェクトクラスを自動スキャンして適用
	if (object_manager)
	{
		ImGui::Text(_U8("[シーンオブジェクトへの適用・ホットリロード]"));

		//シーン上にいる全オブジェクトからクラス名の一覧を構築
		if (ImGui::Button(_U8("シーン上の有効なクラスを検索")))
		{
			scannable_class_names.clear();

			//マネージャーからゲームオブジェクトのリストを取得してループ
			for (const std::unique_ptr<GameObject>& obj_ptr : object_manager->GetGameObjects()) 
			{
				if (!obj_ptr)continue;
				const std::string& name = obj_ptr->GetClassName();

				//重複していなければコンボボックスの選択肢として追加
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

			//適用実行
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

//登録ステートの一覧・追加・削除
void StateMachineEditor::DrawStateListPane()
{
	ImGui::Text(_U8("[ステート一覧リスト]"));

	//ステートの新規追加ボタン
	if (ImGui::Button(_U8("新規ステートを追加"), ImVec2(-1, 0)))
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
			label += "_U8([初期起動ステート])";
		}

		//リストアイテムが選択されたらインデックスを更新
		if (ImGui::Selectable(label.c_str(), selected_state_index == static_cast<int>(i)))
		{
			selected_state_index = static_cast<int>(i);
		}
	}
	ImGui::EndChild();
}

//グラフ無限キャンパスの描画
void StateMachineEditor::DrawNodeGraphCanves()
{
	ImGui::Text(_U8("【ビジュアル・ノードグラフ】 (マウス中ボタンドラッグ: 画面スクロール / 右クリック: メニュー展開)"));

	//キャンバスを描画する枠を作成
	ImGui::BeginChild("CanvasChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	//キャンバスの背景をダークグレーで敷き詰める
	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));

	//背景グリッド線の描画処理
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

	//キャンバス外にノードがはみ出ても枠内に綺麗にクリッピングされるようにクリップ設定
	ImGui::PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

	//ノード同士を結ぶ矢印線の描画
	DrawGraphTransitions(draw_list, canvas_pos);

	//ステートを表す四角形本体の描画とドラッグ処理
	DrawGraphNodes(draw_list, canvas_pos);

	//現在線を引っ張っている最中（矢印新規追加中）なら、マウスカーソルへの仮の線を描画
	if (link_source_node_index != -1)
	{
		ImVec2 src_pos = ImVec2(canvas_pos.x + editor_states[link_source_node_index].graph_position.x + scrolling_offset.x + 140.0f,
			canvas_pos.y + editor_states[link_source_node_index].graph_position.y + scrolling_offset.y + 25.0f);
		draw_list->AddLine(src_pos, ImGui::GetMousePos(), IM_COL32(255, 255, 0, 255), 3.0f);

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			link_source_node_index = -1; //虚空で離したらキャンセル
		}
	}

	ImGui::PopClipRect();

	//キャンバス上でのマウス入力インタラクション処理
	if (ImGui::IsWindowHovered())
	{
		//マウス中ボタンのドラッグでキャンバス全体をスクロール
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
		{
			scrolling_offset.x += ImGui::GetIO().MouseDelta.x;
			scrolling_offset.y += ImGui::GetIO().MouseDelta.y;
		}

		//右クリックで何もない場所ならコンテキストメニュー（新規State作成）を開く
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("CanvasContextMenu");
		}
	}

	//キャンバス右クリックメニュー
	if (ImGui::BeginPopup("CanvasContextMenu"))
	{
		if (ImGui::MenuItem(_U8("新規ステートノードを追加(配置)")))
		{
			StateNodeData new_node;
			new_node.state_name = "NewState_" + std::to_string(editor_states.size());
			new_node.animation_clip_name = "Idle"; //
			new_node.is_animation_loop = true; //

			//マウスを右クリックしたキャンバス上の世界座標に直接配置
			new_node.graph_position.x = ImGui::GetIO().MouseClickedPos[1].x - canvas_pos.x - scrolling_offset.x;
			new_node.graph_position.y = ImGui::GetIO().MouseClickedPos[1].y - canvas_pos.y - scrolling_offset.y;

			editor_states.push_back(new_node);
			selected_state_index = static_cast<int>(editor_states.size()) - 1;
		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();
}

//すべてのノードの描画とドラッグ処理
void StateMachineEditor::DrawGraphNodes(ImDrawList* draw_list, ImVec2 canvas_pos)
{
	constexpr float node_width = 160.0f;
	constexpr float node_height = 55.0f;

	for (size_t i = 0; i < editor_states.size(); i++)
	{
		StateNodeData& node = editor_states[i];
		ImGui::PushID(static_cast<int>(i));

		//画面上の絶対描画位置を算出
		ImVec2 node_screen_pos = ImVec2(canvas_pos.x + node.graph_position.x + scrolling_offset.x,
			canvas_pos.y + node.graph_position.y + scrolling_offset.y);

		//通常ノード、選択中ノード、初期ステートノードで外枠の色を切り替える
		ImU32 node_bg_color = IM_COL32(60, 60, 60, 255);
		ImU32 node_border_color = (selected_state_index == static_cast<int>(i)) ? IM_COL32(255, 255, 0, 255) : IM_COL32(100, 100, 100, 255);

		if (node.state_name == initial_state_name)
		{
			node_bg_color = IM_COL32(110, 70, 30, 255);
		}

		//ノードの土台をカスタム描画
		draw_list->AddRectFilled(node_screen_pos, ImVec2(node_screen_pos.x + node_width, node_screen_pos.y + node_height), node_bg_color, 6.0f);
		draw_list->AddRect(node_screen_pos, ImVec2(node_screen_pos.x + node_width, node_screen_pos.y + node_height), node_border_color, 6.0f, 0, 2.0f);

		//ステート名テキストの描画
		ImVec2 text_pos = ImVec2(node_screen_pos.x + 10.0f, node_screen_pos.y + 10.0f);
		draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), node.state_name.c_str());

		//再生されるアニメーション名も小さく四角形内に表示
		ImVec2 sub_text_pos = ImVec2(node_screen_pos.x + 10.0f, node_screen_pos.y + 30.0f);
		draw_list->AddText(sub_text_pos, IM_COL32(180, 180, 180, 255), node.animation_clip_name.c_str());

		//矢印を引っ張るための「右側結合丸ボタン」を配置
		ImVec2 pin_pos = ImVec2(node_screen_pos.x + node_width - 12.0f, node_screen_pos.y + node_height * 0.5f);
		draw_list->AddCircleFilled(pin_pos, 5.0f, IM_COL32(150, 255, 150, 255));

		//見えない透明なImGuiボタンを四角形の上に重ねてマウス判定を検出
		ImGui::SetCursorScreenPos(node_screen_pos);
		ImGui::InvisibleButton("NodeHitBox", ImVec2(node_width, node_height));

		if (ImGui::IsItemHovered())
		{
			//左クリックでその四角形を選択
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				selected_state_index = static_cast<int>(i);
			}

			//ピン付近でドラッグを開始したら矢印（Transition）の引っ張りモードへ
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MousePos.x > (node_screen_pos.x + node_width - 25.0f))
			{
				link_source_node_index = static_cast<int>(i);
			}
		}

		//四角形ドラッグによる位置の更新処理
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && link_source_node_index == -1)
		{
			node.graph_position.x += ImGui::GetIO().MouseDelta.x;
			node.graph_position.y += ImGui::GetIO().MouseDelta.y;
		}

		//矢印線を引っ張ったまま別の四角形の上でマウスを離したら、全自動で遷移コネクションを結合
		if(link_source_node_index != -1 && link_source_node_index != static_cast<int>(i))
		{
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				TransitionNodeData new_link;
				new_link.next_state_name = node.state_name; //離した先の四角形の名前を遷移先にセット
				editor_states[link_source_node_index].transitions.push_back(new_link);
				link_source_node_index = -1; //結びつけ完了
			}
		}

		ImGui::PopID();
	}
}

//ノード同士を繋ぐ矢印線の描画
void StateMachineEditor::DrawGraphTransitions(ImDrawList* draw_list, ImVec2 canvas_pos)
{
	constexpr float node_width = 160.0f;
	constexpr float node_height = 55.0f;

	for (size_t i = 0; i < editor_states.size(); i++)
	{
		const StateNodeData& src_node = editor_states[i];

		for (const auto& trans : src_node.transitions)
		{
			//遷移先の名前を持つ四角形を検索
			for (size_t j = 0; j < editor_states.size(); j++)
			{
				const StateNodeData& dest_node = editor_states[j];
				if (dest_node.state_name == trans.next_state_name)
				{
					//出発点（右側）と到着点（左側）の2D座標を逆算
					ImVec2 p_start = ImVec2(canvas_pos.x + src_node.graph_position.x + scrolling_offset.x + node_width,
						canvas_pos.y + src_node.graph_position.y + scrolling_offset.y + node_height * 0.5f);
					ImVec2 p_end = ImVec2(canvas_pos.x + dest_node.graph_position.x + scrolling_offset.x,
						canvas_pos.y + dest_node.graph_position.y + scrolling_offset.y + node_height * 0.5f);

					//UnityやUEのように、滑らかな曲線（三次ベジェ曲線）で四角形同士を接続
					ImVec2 cp1 = ImVec2(p_start.x + 50.0f, p_start.y);
					ImVec2 cp2 = ImVec2(p_end.x - 50.0f, p_end.y);

					//線の太さを 3.0f にして視認性を劇的に向上
					draw_list->AddBezierCurve(p_start, cp1, cp2, p_end, IM_COL32(200, 200, 200, 255), 3.0f);

					//到着地点に小さな矢印マーク（三角形）を描画して流れる方向を視覚化
					draw_list->AddTriangleFilled(p_end, ImVec2(p_end.x - 8.0f, p_end.y - 6.0f), ImVec2(p_end.x - 8.0f, p_end.y + 6.0f), IM_COL32(255, 255, 100, 255));
				}
			}
		}
	}
}

//選択中のステート・遷移条件のプロパティ編集
void StateMachineEditor::DrawInspectorPane()
{
	ImGui::Text(_U8("【詳細プロパティ設定インスペクター】"));

	//編集ターゲットが選ばれていない場合は警告を表示して処理を抜ける
	if (selected_state_index < 0 || selected_state_index >= static_cast<int>(editor_states.size()))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Select a state from the left list to edit.");
		return;
	}

	//編集中のノードの参照を取得
	StateNodeData& target_node = editor_states[selected_state_index];

	//UIテキスト用表示文字列配列を関数のトップへ配置
	const char* type_names[] = { _U8("移動入力の長さ (InputLength)"), _U8("ボタンビットフラグ判定 (ButtonCommand)"), _U8("個別アクション入力検知 (ActionPressed)"), _U8("接地状態フラグ (IsGrounded)"), _U8("目標移動速度 (TargetMoveSpeed)") };
	const char* op_names[] = { _U8("一致 (==)"), _U8("不一致 (!=)"), _U8("より大きい (>)"), _U8("未満 (<)"), _U8("以上 (>=)"), _U8("以下 (<=)") };
	const char* blend_names[] = { _U8("共通条件と組み合わせる (Common AND Individual)"), _U8("個別条件のみを適用する (Override)") };

	//基本プロパティの編集
	char name_buf[64];
	strncpy_s(name_buf, sizeof(name_buf), target_node.state_name.c_str(), _TRUNCATE);
	if (ImGui::InputText(_U8("ステート名"), name_buf, sizeof(name_buf)))
	{
		target_node.state_name = name_buf;
	}

	//アニメーション選択
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

	ImGui::Separator();
	ImGui::Text(_U8("【 矢印（個別遷移リンクパス）ごとの固有条件設定 】)"));

	//各矢印（個別遷移条件）のエディタ描画ループ
	for (size_t t = 0; t < target_node.transitions.size(); t++)
	{
		ImGui::PushID(static_cast<int>(t));
		TransitionNodeData& trans = target_node.transitions[t];

		ImGui::Text(_U8("接続パス番号 [%d] -> 遷移先ターゲット: %s"), static_cast<int>(t), trans.next_state_name.c_str());

		//共通条件とのブレンド設定のコンボボックス
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

		if (ImGui::Button(_U8("この接続パス（矢印）を削除")))
		{
			target_node.transitions.erase(target_node.transitions.begin() + t);
			ImGui::PopID();
			break;
		}

		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		ImGui::Separator();
		ImGui::PopStyleColor();
		ImGui::PopID();
	}
}

//ファイルダイアログを開いてパスを取得
std::string StateMachineEditor::OpenFileDialog(bool is_save_mode, const char* filter)
{
	char file_buffer[MAX_PATH] = "";

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = nullptr; // メインウィンドウハンドルがあれば指定
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = file_buffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = "Data//Json"; // デフォルトを開くフォルダに指定
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (is_save_mode)
	{
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn))
		{
			return std::string(file_buffer);
		}
	}
	else
	{
		if (GetOpenFileNameA(&ofn))
		{
			return std::string(file_buffer);
		}
	}

	return "";
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
		node_json["has_common_condition"] = node.has_common_condition;
		node_json["common_condition_type"] = static_cast<int>(node.common_condition_type);
		node_json["common_operator_type"] = static_cast<int>(node.common_operator_type);
		node_json["common_compare_value"] = node.common_compare_value;
		node_json["common_target_action_name"] = node.common_target_action_name;

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
		current_project_file_path = file_path;
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
	current_project_file_path = file_path;
}

//プレビューモデルの個別読込
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

//特定のクラスへ編集中のデータを同期
void StateMachineEditor::ApplyStateMachineToClass(ObjectManager* object_manager, const std::string& target_class_name)
{
	assert(object_manager != nullptr && "StateMachineEditor::ApplyStateMachineToClass - ObjectManager is null.");
	if (!object_manager)return;

	int inject_count = 0;

	for (const std::unique_ptr<GameObject>& obj_ptr : object_manager->GetGameObjects())
	{
		GameObject* obj = obj_ptr.get();
		if (!obj)continue;

		//UIで選択したクラス名と一致するか判定
		if (obj->GetClassName() == target_class_name)
		{
			obj->LoadStateMachineConfig(current_project_file_path);
			inject_count++;
		}
	}
	std::string debug_msg = "[Editor] Injected state machine to " + std::to_string(inject_count) + " instances of " + target_class_name + ".\n";
	OutputDebugStringA(debug_msg.c_str());
}