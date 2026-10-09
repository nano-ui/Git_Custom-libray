#include "BehaviorNodeRenderer.h"
#include "Editor\StateMachineEditor\StateGraphDataManager.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//ノード描画
void BehaviorNodeRenderer::DrawNode(const GraphNode& node, bool is_active)
{
	int pushed_style_count = 0;	//色の適用数

	//アクティブ実行中のボーダー色設定
	if (is_active)
	{
		const ImVec4 active_border_color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, active_border_color);
		pushed_style_count++;
	}

	//ノードカテゴリーに応じた背景色と枠線色のカスタマイズ
	ImVec4 bg_color = ImVec4(0.2f, 0.2f, 0.2f, 0.9f);
	switch (node.behavior_data.category)
	{
	case BehaviorCategory::Root:
		bg_color = ImVec4(0.35f, 0.15f, 0.45f, 0.9f);	//紫
		break;
	case BehaviorCategory::Composite:
		if (node.behavior_data.composite_node_type == CompositeNodeType::Select)	//中間ノード属性が優先順位の場合
		{
			bg_color = ImVec4(0.15f, 0.25f, 0.45f, 0.9f);	//青
		}
		else if (node.behavior_data.composite_node_type == CompositeNodeType::Weight)	//重みの場合
		{
			bg_color = ImVec4(0.45f, 0.35f, 0.15f, 0.9f);	//オレンジ
		}
		break;
	case BehaviorCategory::Action:
		bg_color = ImVec4(0.15f, 0.4f, 0.25f, 0.9f);	//緑
		break;
	default:
		break;
	}

	ed::PushStyleColor(ed::StyleColor_NodeBg, bg_color);
	pushed_style_count++;

	//各ノードの固有名表示
	ed::BeginNode(node.id);

	ImGui::Text(u8"[&s]", node.name.c_str());	//ノード名

	if (node.behavior_data.category == BehaviorCategory::Root)
	{
		ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), u8"ルートノード");
	}
	else if (node.behavior_data.category == BehaviorCategory::Composite)
	{
		const char* comp_name = (node.behavior_data.composite_node_type == CompositeNodeType::Select) ? u8"優先順位 (Select)" : u8"重み抽選 (Weight)";
		ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", comp_name);
	}
	else if (node.behavior_data.category == BehaviorCategory::Action)
	{
		ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"行動(Action)");
		if (!node.animation_name.empty())
		{
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Anim: %s", node.animation_name.c_str());
		}
	}

	ImGui::Spacing();

	//入力ピンと出力ピンの描画
	DrawInputPins(node);
	ImGui::SameLine();
	constexpr float middle_spacer_width = 30.0f;
	ImGui::Dummy(ImVec2(middle_spacer_width, 0.0f));
	ImGui::SameLine();
	DrawOutputPins(node);

	ed::EndNode();

	//色の適用
	for (int color_idx = 0; color_idx < pushed_style_count; color_idx++)
	{
		ed::PopStyleColor();
	}
}

//入力ピン描画
void BehaviorNodeRenderer::DrawInputPins(const GraphNode& node)
{
	//Rootノードは入力ピンを持たない
	if (node.behavior_data.category == BehaviorCategory::Root)
	{
		return;
	}

	ImGui::BeginGroup();
	for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
	{
		const GraphPin& pin = node.inputs[in_idx];
		ed::BeginPin(pin.id, ed::PinKind::Input);
		ImGui::Text(u8"↓親");
		ed::EndPin();
	}
	ImGui::EndGroup();
}

//出力ピン描画
void BehaviorNodeRenderer::DrawOutputPins(const GraphNode& node)
{
	//末端ノードは出力ピンを持たない
	if (node.behavior_data.category == BehaviorCategory::Action)
	{
		return;
	}

	ImGui::BeginGroup();
	for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
	{
		const GraphPin& pin = node.outputs[out_idx];
		ed::BeginPin(pin.id, ed::PinKind::Input);
		ImGui::Text(u8"↓子");
		ed::EndPin();
	}
	ImGui::EndGroup();
}
