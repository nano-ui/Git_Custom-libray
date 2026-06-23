#pragma once
#include "JudgmentNode.h"

#include <functional>

//割合の比較方法
enum class CompareType
{
	LessEqual,		//閾値以下
	GreaterEqual,	//閾値以上
	Less,			//閾値未満
	Greater			//閾値より大きい
};

class RatioJudgment : public JudgmentNode
{
public:
	RatioJudgment(
		std::reference_wrapper<const float> current_val,
		std::reference_wrapper<const float> max_val,
		float threshold_ratio,
		CompareType compare_type = CompareType::LessEqual
	);

	//判定
	bool Check() override;

private:
	std::reference_wrapper<const float> current_value;	//現在地を取得するための関数を保持
	std::reference_wrapper<const float> max_value;		//最大値を取得するための関数を保持
	float threshold_ratio;						//判定の基準となる割合
	CompareType compare_type;					//どのように比較するか
	
};

