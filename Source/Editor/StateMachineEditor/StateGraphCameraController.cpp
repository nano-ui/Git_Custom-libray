#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateGraphCameraController.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <imgui_node_editor_internal.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//コンストラクタ
StateGraphCameraController::StateGraphCameraController()
{
	pending_focus_node_id = 0;
	focus_margin = 50.0f;
	focus_duration_time = 0.0f;
	is_zoom_correction_enabled = false;
}

//特定のノードへのフォーカス移動を要求
void StateGraphCameraController::RequestFocusNode(
	uint32_t node_id)	//フォーカス対象のノードID
{
	//無効なノードIDか判定
	if (node_id == 0 || node_id == UINT32_MAX)
	{
		return;
	}
	pending_focus_node_id = node_id;
}

//カメラのフォーカス更新処理
void StateGraphCameraController::UpdateCameraFocus()
{
	//---------------------------------
	//フォーカス要求の存在チェック
	//---------------------------------
	//保留中のフォーカス対象がないか判定
	if (pending_focus_node_id == 0)
	{
		return;
	}

	const uint32_t target_node_id = pending_focus_node_id;	//対象ノードID
	pending_focus_node_id = 0;

	//----------------------------------
	//NodeEditorでの選択と画面内外判定
	//----------------------------------
	ed::SelectNode(target_node_id, false);

	//ノードが既に画面内に収まっているか判定
	if (!IsNodeInScreen(target_node_id))
	{
		ed::NavigateToSelection(is_zoom_correction_enabled, focus_duration_time);

		//補完アニメーション時間が設定されているか判定
		if (focus_duration_time > 0.0f)
		{
			printf("StateGraphCameraController: ノード ID:%u が画面外のためカメラフォーカスを実行しました。\n", target_node_id);
		}
	}
}

//指定ノードが現在の画面内に収まっているか判定
bool StateGraphCameraController::IsNodeInScreen(
	uint32_t node_id)	//判定対象ノードのID
{
	auto* internal_context = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ed::GetCurrentEditor());	//内部エディタコンテキスト
	
	//コンテキストポインタの健全性チェック
	if (!internal_context)
	{
		printf("Error: StateGraphCameraController::IsNodeInScreen - EditorContext が nullptr です。\n");
		return true;
	}

	const ImRect view_rect = internal_context->GetViewRect();	//表示領域短形
	auto* internal_node = internal_context->FindNode(node_id);	//対象ノード

	//対象ノードが存在するか判定
	if (!internal_node)
	{
		return true;
	}

	const ImRect node_rect = internal_node->m_Bounds;	//ノードの短形領域

	//---------------------------------
	//マージンを考慮した短形包含判定
	//---------------------------------
	//ノードの四方がマージン込みで表示領域内に収まっているか判定
	if ((view_rect.Min.x + focus_margin) <= node_rect.Min.x &&
		(view_rect.Max.x - focus_margin) >= node_rect.Max.x &&
		(view_rect.Min.y + focus_margin) <= node_rect.Min.y &&
		(view_rect.Max.y - focus_margin) >= node_rect.Max.y)
	{
		return true;
	}
	
	return false;
}
