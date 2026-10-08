#include "Editor\StateMachineEditor\Handlers\StateCanvasInteractionHandler.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Views\StateGraphPaletteWindow.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"
#include <cstdio>

namespace ed = ax::NodeEditor;

void StateCanvasInteractionHandler::HandleContextMenu(
    StateGraphDataManager* data_manager,
    GraphData* current_graph,
    uint32_t current_graph_id)
{
    if (!data_manager || !current_graph) return;

    bool trigger_add_node = false;
    bool trigger_add_subgraph = false;
    bool trigger_convert_subgraph = false;
    static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f);
    static ed::NodeId context_node_id = 0;

    ed::Suspend();
    if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Create New Node Context Menu");
        popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
    }
    if (ed::ShowNodeContextMenu(&context_node_id))
    {
        ed::SelectNode(context_node_id, true);
        ImGui::OpenPopup("Node Context Menu");
    }

    if (ImGui::BeginPopup("Create New Node Context Menu"))
    {
        if (ImGui::MenuItem(u8"ステートを追加")) trigger_add_node = true;
        if (ImGui::MenuItem(u8"サブグラフを追加")) trigger_add_subgraph = true;
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("Node Context Menu"))
    {
        if (ImGui::MenuItem(u8"サブグラフへ変換")) trigger_convert_subgraph = true;
        ImGui::EndPopup();
    }
    ed::Resume();

    if (trigger_add_node)
    {
        uint32_t new_state_id = data_manager->AddStateNode(
            current_graph_id,
            static_cast<float>(popup_click_pos.x),
            static_cast<float>(popup_click_pos.y));
        ed::SetNodePosition(new_state_id, popup_click_pos);
    }

    if (trigger_add_subgraph)
    {
        data_manager->AddSubGrapNode(
            current_graph_id,
            static_cast<float>(popup_click_pos.x),
            static_cast<float>(popup_click_pos.y));

        for (size_t g_idx = 0; g_idx < data_manager->GetGraphDatas().size(); g_idx++)
        {
            if (data_manager->GetGraphDatas()[g_idx].id == current_graph_id)
            {
                current_graph = &data_manager->GetGraphDatas()[g_idx];
                break;
            }
        }
        if (!current_graph->nodes.empty())
        {
            const uint32_t new_node_id = current_graph->nodes.back()->GetNodeBasicData().id;
            ed::SetNodePosition(new_node_id, popup_click_pos);
        }
    }

    if (trigger_convert_subgraph)
    {
        const uint32_t raw_node_id = static_cast<uint32_t>(context_node_id.Get());
        data_manager->ConvertToSubGraph(current_graph_id, raw_node_id);
    }
}

void StateCanvasInteractionHandler::HandlePendingPaletteNode(
    StateGraphDataManager* data_manager,
    StateGraphPaletteWindow* palette_window,
    GraphData*& current_graph,
    uint32_t current_graph_id)
{
    if (!data_manager || !palette_window || !current_graph) return;

    if (palette_window->HasPendingAddNode())
    {
        const ImVec2 center_pos = ImGui::GetMainViewport()->GetCenter();
        const ImVec2 canvas_pos = ed::ScreenToCanvas(center_pos);

        if (palette_window->IsPendingSubGraph())
        {
            data_manager->AddSubGrapNode(
                current_graph_id,
                static_cast<float>(canvas_pos.x),
                static_cast<float>(canvas_pos.y),
                palette_window->GetPendingNodeName());

            for (size_t g_idx = 0; g_idx < data_manager->GetGraphDatas().size(); g_idx++)
            {
                if (data_manager->GetGraphDatas()[g_idx].id == current_graph_id)
                {
                    current_graph = &data_manager->GetGraphDatas()[g_idx];
                    break;
                }
            }
        }
        else
        {
            uint32_t new_state_id = data_manager->AddStateNode(
                current_graph_id,
                static_cast<float>(canvas_pos.x),
                static_cast<float>(canvas_pos.y),
                palette_window->GetPendingNodeName());
            ed::SetNodePosition(new_state_id, canvas_pos);
        }
        palette_window->ClearPendingNode();
    }
}

void StateCanvasInteractionHandler::HandleDragAndDrop(
    StateGraphDataManager* data_manager,
    GraphData*& current_graph,
    uint32_t current_graph_id)
{
    if (!data_manager || !current_graph) return;

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_NORMAL"))
        {
            const char* dropped_node_name = static_cast<const char*>(payload->Data);
            const ImVec2 drop_mouse_canves_pos = ed::ScreenToCanvas(ImGui::GetMousePos());

            uint32_t new_node_id = data_manager->AddStateNode(
                current_graph_id,
                static_cast<float>(drop_mouse_canves_pos.x),
                static_cast<float>(drop_mouse_canves_pos.y),
                dropped_node_name);

            ed::SetNodePosition(new_node_id, drop_mouse_canves_pos);
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_SUB"))
        {
            const char* dropped_sub_name = static_cast<const char*>(payload->Data);
            const ImVec2 drop_mouse_canves_pos = ed::ScreenToCanvas(ImGui::GetMousePos());

            data_manager->AddSubGrapNode(
                current_graph_id,
                static_cast<float>(drop_mouse_canves_pos.x),
                static_cast<float>(drop_mouse_canves_pos.y),
                dropped_sub_name);

            for (size_t g_idx = 0; g_idx < data_manager->GetGraphDatas().size(); g_idx++)
            {
                if (data_manager->GetGraphDatas()[g_idx].id == current_graph_id)
                {
                    current_graph = &data_manager->GetGraphDatas()[g_idx];
                    break;
                }
            }
            if (!current_graph->nodes.empty())
            {
                const uint32_t new_node_id = current_graph->nodes.back()->GetNodeBasicData().id;
                ed::SetNodePosition(new_node_id, drop_mouse_canves_pos);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void StateCanvasInteractionHandler::HandleDeletion(
    StateGraphDataManager* data_manager,
    GraphData* current_graph,
    uint32_t current_graph_id)
{
    if (!data_manager || !current_graph) return;

    if (ed::BeginDelete())
    {
        DeleteNode(data_manager, current_graph, current_graph_id);
        DeleteLink(data_manager, current_graph, current_graph_id);
        ed::EndDelete();
    }
}

void StateCanvasInteractionHandler::DeleteNode(
    StateGraphDataManager* data_manager,
    GraphData* current_graph,
    uint32_t current_graph_id)
{
    ed::NodeId delete_node_id;
    while (ed::QueryDeletedNode(&delete_node_id))
    {
        if (ed::AcceptDeletedItem())
        {
            const uint32_t target_id = static_cast<uint32_t>(delete_node_id.Get());
            data_manager->DeleteNode(current_graph_id, target_id);
        }
    }
}

void StateCanvasInteractionHandler::DeleteLink(
    StateGraphDataManager* data_manager,
    GraphData* current_graph,
    uint32_t current_graph_id)
{
    ed::LinkId delete_link_id;
    while (ed::QueryDeletedLink(&delete_link_id))
    {
        if (ed::AcceptDeletedItem())
        {
            const uint32_t target_id = static_cast<uint32_t>(delete_link_id.Get());
            data_manager->DeleteLink(current_graph_id, target_id);
        }
    }
}