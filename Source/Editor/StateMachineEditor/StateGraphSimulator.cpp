#include "StateGraphSimulator.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Engine\Core\Input.h"

#include <cstdio>

//コンストラクタ
StateGraphSimulator::StateGraphSimulator()
	:current_node_id(UINT32_MAX)
	, current_animation_name("")
	, current_animation_loop(true)
{
}

//デストラクタ
StateGraphSimulator::~StateGraphSimulator() = default;

//グラフデータのセットアップ
void StateGraphSimulator::SetupRuntimeGraph(
	const std::vector<SimulatorRuntimeNode>& in_nodes,
	const std::vector<SimulatorRuntimeLink>& in_links,
	const std::unordered_map<uint32_t, uint32_t>& in_layer_entries)
{
	//メンバ変数へのコピー
	runtime_nodes = in_nodes;
	runtime_links = in_links;
	layer_entry_nades = in_layer_entries;

	//ピン逆引きマップの構築
	pin_to_node_map.clear();

	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		const SimulatorRuntimeNode& node = runtime_nodes[n];

		//入力ピンの登録
		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			pin_to_node_map[node.inputs[p]] = node.id;
		}

		//出力ピンの登録
		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			pin_to_node_map[node.outputs[p]] = node.id;
		}
	}
}

//更新処理
bool StateGraphSimulator::UpdateSimulation(float elapsed_time, StateBlackboard* blackboard, bool is_anim_finished)
{
	//--------------------------------------
	//事前検証と初期アクティブノードの確定
	//--------------------------------------
	//ブラックボードポインタの有効性、ノード配列およびリンク配列が空でないかを検証
	if (!blackboard || runtime_nodes.empty() || runtime_links.empty())
	{
		return false;
	}

	//現在のアクティブノードが無効値であるかを判定
	if (current_node_id == UINT32_MAX)
	{
		constexpr uint32_t root_layer_id = 0;	//ルート階層ID
		auto entry_it = layer_entry_nades.find(root_layer_id);	//エントリーノード検索イテレーター

		//ルートレイヤーのエントリーノードが登録されているかを判定
		if (entry_it != layer_entry_nades.end())
		{
			current_node_id = entry_it->second;
		}
		else
		{
			current_node_id = runtime_nodes.front().id;
		}

		//初期ノードのアニメーション情報を反映するための走査ループ処理
		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			//ノードIDが初期ノードIDと一致したか判定
			if (runtime_nodes[n].id == current_node_id)
			{
				current_animation_name = runtime_nodes[n].animation_name;
				current_animation_loop = runtime_nodes[n].is_loop;
				break;
			}
		}
	}

	uint32_t current_parent_node_id = GetParentNodeId(current_node_id);	//親サブグラフノード
	size_t global_max_conditions = 0;	//条件を満たしたリンクの最大件数

	//----------------------------------------------
	//同一階層内における最大条件数の算出(第一パス)
	//----------------------------------------------
	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const SimulatorRuntimeLink& link = runtime_links[i];	//精査対象のリンク
		const uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id);

		//出発元ノードの親階層が現在の階層と一致するか判定
		if (GetParentNodeId(src_node_id) == current_parent_node_id)
		{
			bool is_all_conditions_mat = !link.conditions.empty();	//全条件を満たしたフラグ

			//リンクに設定された全ての遷移条件を個別に精査
			for (size_t c = 0; c < link.conditions.size(); c++)
			{
				//単一の遷移条件が成立しているか判定
				if (!EvaluateCondition(link.conditions[c], blackboard, is_anim_finished))
				{
					is_all_conditions_mat = false;
					break;
				}
			}

			//すべての条件をクリアし、かつ条件数が最大値を超えているか判定
			if (is_all_conditions_mat && link.conditions.size() > global_max_conditions)
			{
				global_max_conditions = link.conditions.size();
			}
		}
	}

	//-----------------------------
	//遷移先リンクの決定(第2パス)
	//-----------------------------
	//最適なリンク候補を保持する変数の準備
	const SimulatorRuntimeLink* best_link = nullptr;	//最も適合度の高いリンク
	size_t max_conditions = 0;								//成立条件数の最大値
	uint32_t next_node_id = current_node_id;			//遷移先となるノードID
	bool is_transition_triggered = false;				//遷移発生フラグ

	//全リンク走査による最適遷移リンクの探索処理
	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const SimulatorRuntimeLink& link = runtime_links[i];			//精査対象のリンク
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id);	//接続元のノード

		if (src_node_id == UINT32_MAX)
		{
			continue;
		}

		//アクティブツリーへの包含チェック
		bool is_active_tree = false;
		uint32_t curr = current_node_id;	//探索用ID
		
		while (curr != UINT32_MAX)
		{
			if (curr == src_node_id)
			{
				is_active_tree = true;
				break;
			}
			curr = GetParentNodeId(curr);
		}

		if (!is_active_tree)
		{
			continue;
		}

		//第1パスの最大条件数を用いた同一階層の足切り判定
		if (GetParentNodeId(src_node_id) == current_parent_node_id)
		{
			if (link.conditions.size() < global_max_conditions)
			{
				continue;
			}
		}

		if (link.conditions.empty())
		{
			continue;
		}

		//遷移条件の個別評価
		bool is_all_mat = !link.conditions.empty();	//全条件成立フラグ

		for (size_t i = 0; i < link.conditions.size(); i++)
		{
			if (!EvaluateCondition(link.conditions[i], blackboard, is_anim_finished))
			{
				is_all_mat = false;
				break;
			}
		}

		//最良リンクの更新と遷移先ノードの仮決定
		if (is_all_mat && link.conditions.size() >= max_conditions)
		{
			uint32_t dst_node_id = GetNodeIdFromPinId(link.end_pin_id);	//遷移先ノードID
			if (dst_node_id != UINT32_MAX)
			{
				best_link = &link;
				max_conditions = link.conditions.size();
				next_node_id = dst_node_id;
				is_transition_triggered = true;
			}
		}
	}

	//状態遷移の実行とコールバック通知
	if (is_transition_triggered && next_node_id != current_node_id)
	{
		if (on_transition_link_callback && best_link)
		{
			on_transition_link_callback(best_link->id);
		}
		current_node_id = next_node_id;

		//サブグラフの自動潜り込み処理
		bool check_sub_graph = true;	//サブグラフの展開継続フラグ

		while (check_sub_graph)
		{
			check_sub_graph = false;

			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				if (current_node_id == runtime_nodes[n].id)
				{
					if (runtime_nodes[n].is_sub_graph)
					{
						auto sub_graph_entry = layer_entry_nades.find(runtime_nodes[n].sub_graph_id);	//下位グラフのエントリーノード検索イテレーター

						if (sub_graph_entry != layer_entry_nades.end())
						{
							current_node_id = sub_graph_entry->second;
							check_sub_graph = true;
						}
						else
						{
							printf("Warning: StateGraphSimulator::UpdateSimulation - サブグラフ ID:%u にエントリーノードが設定されていません。\n", runtime_nodes[n].sub_graph_id);
						}
					}
					break;
				}
			}
		}

		//アニメーション情報の同期と結果返却
		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			if (current_node_id == runtime_nodes[n].id)
			{
				current_animation_name = runtime_nodes[n].animation_name;
				current_animation_loop = runtime_nodes[n].is_loop;
				break;
			}
		}
		return true;
	}
	return false;
}

//ルートモーション有効フラグ取得
bool StateGraphSimulator::GetRootMotionEnabled() const
{
	//全実行時ノードを巡回して現在のアクティブノードを探索
	for (size_t n = 0; n < runtime_nodes.size(); n++)
	{
		//走査対象ノードの固有IDが現在のアクティブノードIDと一致するか判定
		if (runtime_nodes[n].id == current_node_id)
		{
			return runtime_nodes[n].is_root_motion;
		}
	}
	return false;
}

//単一の遷移条件の評価
bool StateGraphSimulator::EvaluateCondition(
	const TransitionCondition& cond,
	StateBlackboard* blackboard,
	bool is_anim_finished)const
{
	//-----------------------------------
	//アニメーション再生完了条件の判定
	//-----------------------------------
	if (cond.type == ConditionNodeType::AnimationEnd)
	{
		return is_anim_finished;
	}

	//--------------------
	//キー入力条件の判定
	//--------------------
	if (cond.type == ConditionNodeType::InputCheck)
	{
		const int v_key_code = static_cast<int>(cond.hash_key);					//判定対象の仮想キーコード
		const int input_behavior_mode = static_cast<int>(cond.param_second);	//入力挙動モード

		constexpr int mode_press = 0;		//押されている間
		constexpr int mode_trigger = 1;		//押された瞬間
		constexpr int mode_not_press = 2;	//未入力状態
		constexpr int mode_release = 3;		//離れた瞬間

		//キーが未設定の場合は早期リターン
		if (v_key_code == 0)
		{
			return false;
		}

		// 各モードに応じた入力判定の分岐実行
		if (input_behavior_mode == mode_trigger)
		{
			return Input::Instance().IsKeyTrigger(v_key_code);
		}
		else if (input_behavior_mode == mode_press)
		{
			return Input::Instance().IsKeyPress(v_key_code);
		}
		else if (input_behavior_mode == mode_not_press)
		{
			return !Input::Instance().IsKeyPress(v_key_code);
		}
		else if (input_behavior_mode == mode_release)
		{
			return Input::Instance().IsKeyRelease(v_key_code);
		}
		return false;
	}

	//--------------------
	//値の比較条件の判定
	//--------------------
	if (blackboard)
	{
		return cond.IsJudgment(*blackboard);
	}

	printf("Warning: StateGraphSimulator::EvaluateCondition - blackboard が nullptr です。\n");
	return false;
}


//ピンIDから所属ノードIDを逆引き検索
uint32_t StateGraphSimulator::GetNodeIdFromPinId(uint32_t pin_id)const
{
	//キャッシュマップからピンIDを検索
	auto it = pin_to_node_map.find(pin_id);

	//見つかった場合は対応するノードIDを返却
	if (it != pin_to_node_map.end())
	{
		return it->second;
	}

	//見つからなかった場合
	return UINT32_MAX;
}

//ノードIDから所属サブグラフノードIDを逆引き検索
uint32_t StateGraphSimulator::GetParentNodeId(uint32_t node_id)const
{
	//---------------------------------------------------
	//全実行時のノードから指定されたノードIDを線形探索
	//---------------------------------------------------
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		const SimulatorRuntimeNode& node = runtime_nodes[i];	//対象ノード

		//目的のノードIDと一致するか判定
		if (node.id == node_id)
		{
			return node.parent_node_id;
		}
	}
	return UINT32_MAX;
}
