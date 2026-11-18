#pragma once
#include "StateGraphDataManager.h"

class Blackboard;

//比較演算子
enum class CompareOperator
{
	Equal,			//等しい(==)
	NotEqual,		//等しくない(!=)
	Greater,		//より大きい(>)
	Less,			//未満(<)
	GreaterEqual,	//以上(>=)
	LessEqual,		//以下(<=)
};

//遷移条件
struct TransitionCondition
{
	ConditionNodeType type = ConditionNodeType::NormalCompare;			//条件判定のタイプ
	std::string variable_name = "";										//主対象となる変数名（入力判定時はキーコード文字列）
	std::string secondary_variable_name = "";							//比較対象となる副変数名（距離の相手座標や割合の最大値など）
	float reference_value = 0.0f;										//数値・フラグ比較用の基準値
	CompareOperator compart_op = CompareOperator::Equal;				//比較演算子
	float param_second = 0.0f;											//補助パラメータ（最大距離や入力判定モードなど）
	DirectX::XMFLOAT3 vector_reference_value = { 0.0f, 0.0f, 0.0f };	//3Dベクトル比較用の基準値(XYZ)

	//条件を満たしているか判定
	bool IsJudgment(const Blackboard& blackboard) const;
private:
	//通常比較（数値・フラグ・ベクトル）の判定
	bool EvaluateNormalCompare(const Blackboard& blackboard) const;

	//2点間の距離条件の判定
	bool EvaluateDistance(const Blackboard& blackboard) const;

	//現在値と最大値の割合条件の判定
	bool EvaluateRatio(const Blackboard& blackboard) const;

	//2つのfloat値を指定の演算子で比較
	bool CompareFloat(float value, float reference) const;

	//キーボード・マウスの入力条件の判定
	bool CheckInputConditon() const;
private:
	static constexpr uint32_t INVALID_HASH_KEY = 0;				//未計算状態を表す無効なハッシュ値
	mutable uint32_t cached_hash = INVALID_HASH_KEY;			//主変数名のキャッシュ済み検索キー
	mutable uint32_t cached_secondary_hash = INVALID_HASH_KEY;	//副変数名のキャッシュ済み検索キー
};