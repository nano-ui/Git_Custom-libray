#include "RandomJugment.h"

//===================
//コンストラクタ
//===================
RandomJugment::RandomJugment(int success_probability)
	:probability_threshold(success_probability)
{
	//乱数生成器の初期化
	std::random_device seed_gen;
	random_engine = std::mt19937(seed_gen());
}

//==============
//判定処理
//==============
bool RandomJugment::Check()
{
	//---------------------------------
	//0～100の範囲で乱数を生成
	//---------------------------------
	std::uniform_int_distribution<int>dist(0, 100);	//一様分布
	int random_value = dist(random_engine);			//乱数の値

	//----------------
	//確率判定
	//----------------
	return random_value <= probability_threshold;
}
