#include "StateGraphSimulator.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"

#include <imgui.h>

//現在の階層におけるステートの遷移をブラックボードをもとに評価・更新
bool StateGraphSimulator::UpdateSimulation(
    StateGraphDataManager* data_manager,
    StateBlackboard* blackboard,
    GraphData* current_graph,
    uint32_t& in_out_active_node_id,
    uint32_t& out_flowing_link_id)
{
    //ブラックボードとグラフエディタ、マネージャーの有効チェック
    if (!blackboard || !current_graph || !data_manager)
    {
        return false;
    }

    bool is_state_changed = false;  //状態遷移フラグ

    //現在の階層の全リンクを走査して遷移条件を評価
    for (size_t i = 0; i < current_graph->links.size(); i++)
    {
        const GraphLink& link = current_graph->links[i];    //精査対象のリンク
        uint32_t src_node_id = data_manager->GetNodeIdFromPinId(current_graph->id, link.start_pin_id);  //リンク元のノードID

        //リンク元が現在実行中ノードか判定
        if (src_node_id == in_out_active_node_id)
        {
            bool is_all_condition_met = !link.conditions.empty();   //条件が満たされたかのフラグ

            //リンクに設定されたすべての条件を評価
            for (size_t c = 0; c < link.conditions.size(); c++)
            {
                const GraphTransitionCondition& graph_cond = link.conditions[c];    //評価先の条件

                TransitionCondition runtime_cond;   //実行時判定
                runtime_cond.type = graph_cond.type;
                runtime_cond.hash_key = graph_cond.hash_key;
                runtime_cond.reference_value = graph_cond.reference_value;
                runtime_cond.compart_op = static_cast<CompareOperator>(graph_cond.compare_operator);
                runtime_cond.param_second = graph_cond.param_second;
                runtime_cond.secondary_hash = graph_cond.secondary_hash;

                //条件を満たしていないか判定
                if (!runtime_cond.IsJudgment(*blackboard))
                {
                    is_all_condition_met = false;
                    break;
                }
            }

            //全ての遷移条件が満たされたか判定
            if (is_all_condition_met)
            {
                uint32_t dst_node_id = data_manager->GetNodeIdFromPinId(current_graph->id, link.end_pin_id);    //遷移先のノードID
                in_out_active_node_id = dst_node_id;
                out_flowing_link_id = link.id;
                is_state_changed = true;
                break;
            }

        }
    }
    return is_state_changed;
}
