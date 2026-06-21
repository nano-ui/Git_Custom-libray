#pragma once
#include "JudgmentNode.h"

#include <random>

class RandomJugment : public JudgmentNode
{
public:
	RandomJugment(int success_probability);

	//判定処理
	bool Check()override;

private:
	int probability_threshold;	//判定の閾値
	std::mt19937 random_engine;	//メルセンヌ・ツイスタ乱数生成器
};

