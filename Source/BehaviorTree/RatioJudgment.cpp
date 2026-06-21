#include "RatioJudgment.h"

//===================
//コンストラクタ
//===================
RatioJudgment::RatioJudgment(std::reference_wrapper<const float> current_val, std::reference_wrapper<const float> max_val, float threshold_ratio, CompareType compare_type)
	: current_value(current_val), max_value(max_val), threshold_ratio(threshold_ratio), compare_type(compare_type)
{
}
//===========
//判定
//===========
bool RatioJudgment::Check()
{
	//------------------------------------------
	//データの取得とエラー防止チェック
	//------------------------------------------
	float current_val = current_value.get();	//現在の値を取得
	float max_val = max_value.get();			//対象の最大値を取得

	if (max_val <= 0.0f) return false;	//0以下か確認

	//----------------------
	//現在の割合の計算
	//----------------------
	float current_ratio = current_val / max_val;	//現在値を最大値で割り、現在の割合（0.0f ～ 1.0f）を計算

	//------------------------------------------
	// 条件に応じた判定の実行
	//------------------------------------------
	switch (compare_type)							//設定された比較方法に応じて処理を分岐させる
	{
	case CompareType::LessEqual:					//閾値「以下」か判定するモードの場合
		return current_ratio <= threshold_ratio;	//現在の割合が基準値以下であればtrueを返す
	case CompareType::GreaterEqual:					//閾値「以上」か判定するモードの場合
		return current_ratio >= threshold_ratio;	//現在の割合が基準値以上であればtrueを返す
	case CompareType::Less:							//閾値「未満」か判定するモードの場合
		return current_ratio < threshold_ratio;		//現在の割合が基準値未満であればtrueを返す
	case CompareType::Greater:						//閾値「より大きい」か判定するモードの場合
		return current_ratio > threshold_ratio;		//現在の割合が基準値より大きければtrueを返す
	default:										//予期しない列挙型が渡された場合
		return false;								//安全のためfalseを返す
	}
}
