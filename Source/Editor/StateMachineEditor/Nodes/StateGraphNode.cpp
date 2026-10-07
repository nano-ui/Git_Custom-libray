#include "StateGraphNode.h"
#include "Serialization\JsonSerializer.h"

//コンストラクタ
StateGraphNode::StateGraphNode()
{
	animation_data.animation_name = "";
	animation_data.is_loop = false;
	animation_data.is_root_motion = false;
	animation_data.blend_duration = 0.2f;
	link_color = { 1.0f,1.0f,1.0f };
	action_category = ActionCategory::Idle;
}

//デストラクタ
StateGraphNode::~StateGraphNode() = default;

//ピンの登録処理
void StateGraphNode::SetupPins()
{
	AddInputPin("入力");
	AddOutputPin("出力");
}

//保存変数登録処理
void StateGraphNode::SetupSerializer(JsonSerializer* serializer)
{
	GraphNode::SetupSerializer(serializer);
	serializer->RegisterVariable(u8"アクションカテゴリー", &action_category);
	serializer->RegisterVariable(u8"リンク色", &link_color);
	serializer->RegisterVariable(u8"アニメーション名", &animation_data.animation_name);
	serializer->RegisterVariable(u8"ループフラグ", &animation_data.is_loop);
	serializer->RegisterVariable(u8"ルートモーションフラグ", &animation_data.is_root_motion);
	serializer->RegisterVariable(u8"ブレンド時間", &animation_data.blend_duration);
}
