#pragma once

//ノード属性
enum class  BehaviorCategory
{
	Root,		//基底ノード
	Composite,	//中間ノード
	Action		//末端ノード
};

//中間ノード属性
enum class CompositeNodeType
{
	Select,		//優先順位
	Weight,		//重み
};

//末端ノード実行結果属性
enum class ResultType
{
	Success,	//成功
	Running,	//実行中
	Failure		//失敗
};

//ビヘイビアツリーノード情報
struct BehaviorNodeData
{
	BehaviorCategory category = BehaviorCategory::Root;					//ノード属性
	CompositeNodeType composite_node_type = CompositeNodeType::Select;	//中間ノード属性
	ResultType result_type = ResultType::Success;						//実行結果

	int weight = 1;						//抽選割合
	float action_duration = -1.0f;		//行動持続時間
	float cooldown_time = 0.0f;			//クールダウン時間
	float cooldown_random_range = 0.0f;	//クールダウンのランダム変動幅
};