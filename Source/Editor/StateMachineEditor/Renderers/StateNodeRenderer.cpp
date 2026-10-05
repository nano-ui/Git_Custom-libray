#include "Editor\StateMachineEditor\Renderers\StateNodeRenderer.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"

#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

//単一ノードとその入力・出力ピンを描画
void StateNodeRenderer::DrawNode(const GraphNode& node, bool is_active)
{
	int pushed_style_count = 0;

	//アクティブノード時のボーダー色設定
	if (is_active)
	{
		const ImVec4 active_border_color = ImVec4(0.0f, 1.0f, 0.3f, 1.0f);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, active_border_color);
		pushed_style_count++;
	}

	//サブグラフノード時の背景色及び選択枠色設定
	if (node.is_sub_graph)
	{
		const ImVec4 sub_bg_color = ImVec4(0.1f, 0.2f, 0.4f, 0.85f);
		const ImVec4 sub_sel_color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f);
		ed::PushStyleColor(ed::StyleColor_NodeBg, sub_bg_color);
		ed::PushStyleColor(ed::StyleColor_SelNodeBorder, sub_sel_color);
		pushed_style_count += 2;
	}

	ed::BeginNode(node.id);
	ImGui::Text("%s", node.name.c_str());
	ImGui::Spacing();

	//入力ピン群の描画
	DrawInputPins(node);

	ImGui::SameLine();
	constexpr float middle_spacer_wodtj = 40.0f;
	ImGui::Dummy(ImVec2(middle_spacer_wodtj, 0.0f));
	ImGui::SameLine();

	//出力ピン群の描画
	DrawOutputPins(node);

	ed::EndNode();

	//適用したスタイルの解除
	for (int color_idx = 0; color_idx < pushed_style_count; color_idx++)
	{
		ed::PopStyleColor();
	}
}

//入力ピン群を描画
void StateNodeRenderer::DrawInputPins(const GraphNode& node)
{
	ImGui::BeginGroup();
	for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
	{
		const GraphPin& pin = node.inputs[in_idx];
		ed::BeginPin(pin.id, ed::PinKind::Input);
		ImGui::Text("->%s", pin.name.c_str());
		ed::EndPin();
	}
	ImGui::EndGroup();
}

//出力ピン群を描画
void StateNodeRenderer::DrawOutputPins(const GraphNode& node)
{
	ImGui::BeginGroup();
	for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
	{
		const GraphPin& pin = node.outputs[out_idx];
		ed::BeginPin(pin.id, ed::PinKind::Output);
		ImGui::Text("->%s", pin.name.c_str());
		ed::EndPin();
	}
	ImGui::EndGroup();
}
