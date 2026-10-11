#pragma once

#include "Gameplay\AI\BehaviorNode.h"
#include "Editor\Behavior\BehaviorNodeData.h"

#include <string>
#include <functional>

class EditorActionNode : public BehaviorNode
{
public:
	//コンストラクタ
	EditorActionNode(
		int node_id,
		const std::string& anim_name,
		bool loop,
		bool root_motion,
		float duration,
		ResultType finish_res,
		std::function<void(uint32_t, const std::string&, bool, bool)> on_execute_callback
	);

	//デストラクタ
	virtual ~EditorActionNode() = default;

	//ノード実行
	virtual NodeState Execute(float elapsed_time)override;

	//ノード検索
	virtual std::shared_ptr<BehaviorNode> SearchNode(int search_id)override;

	//リセット
	void ResutTimer() { current_timer = 0.0f; }

private:
	std::string animation_name;						//アニメーション名
	bool is_loop = true;							//再生フラグ
	bool is_root_motion = true;						//ルートモーションフラグ
	float action_duration = -1.0f;					//実行時間
	ResultType finish_result = ResultType::Success;	//実行結果
	float current_timer = 0.0f;						//現在の時間
	std::function<void(uint32_t, const std::string&, bool, bool)> on_execute;	//実行処理
};

