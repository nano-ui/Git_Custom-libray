#include "StateGraphNode.h"

//コンストラクタ
StateGraphNode::StateGraphNode()
{
	animation_data.animation_name = "";
	animation_data.is_loop = false;
	animation_data.is_root_motion = false;
	animation_data.blend_duration = 0.2f;
	link_color = { 1.0f,1.0f,1.0f };
	action_category = 0;
}

//デストラクタ
StateGraphNode::~StateGraphNode() = default;

//ピンの登録処理
void StateGraphNode::SetupPins()
{
	AddInputPin("入力");
	AddOutputPin("出力");
}
