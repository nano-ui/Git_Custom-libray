#include "EditorActionNode.h"

//コンストラクタ
EditorActionNode::EditorActionNode(
	int node_id,
	const std::string& anim_name,
	bool loop,
	bool root_motion,
	float duration,
	ResultType finish_res,
	std::function<void(uint32_t, const std::string&, bool, bool)> on_execute_callback)
	:BehaviorNode(node_id)
	, animation_name(anim_name)
	, is_loop(loop)
	, is_root_motion(root_motion)
	, action_duration(duration)
	, finish_result(finish_res)
	, on_execute(on_execute_callback)
{
}

//ノード実行
NodeState EditorActionNode::Execute(float elapsed_time)
{
	if (on_execute)
	{
		on_execute(static_cast<uint32_t>(GetID()), animation_name, is_loop, is_root_motion);
	}

	current_timer += elapsed_time;

	if (action_duration >= 0.0f)
	{
		if (current_timer >= action_duration)
		{
			current_timer = 0.0f;
			return(finish_result == ResultType::Failure) ? NodeState::Failure : NodeState::Success;
		}
		return NodeState::Running;
	}

	constexpr float fallback_action_timeout = 3.0f;
	if (current_timer >= fallback_action_timeout)
	{
		current_timer = 0.0f;
		return (finish_result == ResultType::Failure) ? NodeState::Failure : NodeState::Success;
	}
	return NodeState::Running;
}

//ノード検索
std::shared_ptr<BehaviorNode> EditorActionNode::SearchNode(int search_id)
{
	if (GetID() == search_id)
	{
		return nullptr;
	}
	return nullptr;
}
