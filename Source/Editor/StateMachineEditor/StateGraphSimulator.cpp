#include "StateGraphSimulator.h"
#include "Gameplay/StateMachine/StateGraphDataManager.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Engine\Core\Input.h"

#include <imgui.h>
#include <windows.h>

//現在の階層におけるステートの遷移をブラックボードをもとに評価・更新
bool StateGraphSimulator::UpdateSimulation(
	StateGraphDataManager* data_manager,
	StateBlackboard* blackboard,
	GraphData* current_graph,
	uint32_t& in_out_active_node_id,
	uint32_t& out_flowing_link_id
)
{
	if (!data_manager || !current_graph)	//必要なデータポインタが安全であるかを判定
	{
		return false;
	}

	std::unordered_map<uint32_t, uint32_t> pin_cache_map;	//ピンIDから所属ノードIDを逆引き

	//階層内の全ノードを1回だけ巡回してピンと親ノードのペアをキャッシュするループ処理
	for (size_t n = 0; n < current_graph->nodes.size(); n++)	
	{
		const GraphNode& node = current_graph->nodes[n];	//走査対象ノードのデータ

		//入力ピンのIDをハッシュマップへ登録するループ処理
		for (size_t p = 0; p < node.inputs.size(); p++)	
		{
			pin_cache_map[node.inputs[p].id] = node.id;
		}

		//出力ピンのIDをハッシュマップへ登録するループ処理
		for (size_t p = 0; p < node.outputs.size(); p++)	
		{
			pin_cache_map[node.outputs[p].id] = node.id;
		}
	}

	bool is_state_changed = false;	//ステートが遷移したかを表すフラグ
	const GraphLink* best_link = nullptr;	//最も多くの条件を満たした最適なリンク
	size_t max_conditions = 0;				//満たされた条件の最大数

	//現在の階層に存在するすべてのリンク条件を走査
	for (size_t i = 0; i < current_graph->links.size(); i++)	
	{
		const GraphLink& link = current_graph->links[i];	//対象のリンク情報の参照を格納
		uint32_t src_node_id = 0;	//リンクの出発元ノードIDを保持

		auto start_it = pin_cache_map.find(link.start_pin_id);	//開始ピンIDからキャッシュを探索した結果イテレーター

		//開始ピンのキャッシュ情報がマップ内に存在するかを判定
		if (start_it != pin_cache_map.end())	
		{
			src_node_id = start_it->second;
		}

		//リンクの出発元が現在の実行中アクティブノードと一致するかを判定
		if (src_node_id != in_out_active_node_id)	
		{
			continue;
		}

		bool is_all_condition_met = true;	//すべての条件を満たしたかを表す判定フラグ

		for (size_t c = 0; c < link.conditions.size(); c++)	//リンクが持つすべての遷移条件を個別に精査するループ処理
		{
			const GraphTransitionCondition& graph_cond = link.conditions[c];    //評価先の条件

			//キー入力判定 (InputCheck) の場合の処理
			if (graph_cond.type == ConditionNodeType::InputCheck)
			{
				int v_key_code = static_cast<int>(graph_cond.hash_key); // 仮想キーコード
				int input_behavior_mode = static_cast<int>(graph_cond.param_second); // 0:Press, 1:Trigger

				constexpr int mode_trigger_val = 1; // マジックナンバーの回避：トリガーモード
				bool is_key_satisfied = false;

				if (input_behavior_mode == mode_trigger_val)
				{
					is_key_satisfied = Input::Instance().IsKeyTrigger(v_key_code);
				}
				else
				{
					is_key_satisfied = Input::Instance().IsKeyPress(v_key_code);
				}

				if (!is_key_satisfied)
				{
					is_all_condition_met = false;
					break;
				}
			}
			else
			{
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
		}

		if (is_all_condition_met)	//すべての遷移条件を完全にクリアしたかを判定
		{
			//リンクの条件数が最大値よりも大きいか、または最初の適合リンクか判定
			if (!best_link || link.conditions.size() > max_conditions)
			{
				best_link = &link;
				max_conditions = link.conditions.size();
			}
		}
	}

	//条件を満たす最適な遷移リンクが見つかったか判定
	if (best_link)
	{
		uint32_t dst_node_id = 0;	//遷移先のノードID
		auto end_it = pin_cache_map.find(best_link->end_pin_id);	//終了ピンIDからキャッシュを探索した結果イテレーター

		//終了ピンのキャッシュ情報がマップ内に存在するか判定
		if (end_it != pin_cache_map.end())
		{
			dst_node_id = end_it->second;
		}

		//不正なピン定義
		if (dst_node_id == 0)
		{
			OutputDebugStringA("[StateGraphSimulator] Error: 終了ピンに対応するノードIDがキャッシュに存在しません。\n");
		}

		in_out_active_node_id = dst_node_id;
		out_flowing_link_id = best_link->id;
		is_state_changed = true;
	}

	return is_state_changed;
}