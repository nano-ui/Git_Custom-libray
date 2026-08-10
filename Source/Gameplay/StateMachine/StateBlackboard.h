#pragma once

#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <iostream>
#include <cstdint>
#include <string_view>
#include <imgui.h>
#include <vector>
#include <cmath>

#include "StateGraphDataManager.h"

//キャラクターのアクションカテゴリー
enum class ActionCategory
{
	Idle,			//待機状態
	Locomotion,		//移動状態
	Airborne,		//空中状態
	Attack,			//攻撃状態
	Avoid,			//回避状態
	Skile,			//固有アビリティ状態
};

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

struct TransitionCondition;

//登録可能な型
using BlackboardData = std::variant<bool, int, float, DirectX::XMFLOAT3, std::vector<std::string>, uint32_t>;
class StateBlackboard
{
public:
	//コンストラクタ
	StateBlackboard();

	//デストラクタ
	~StateBlackboard();

	//ImGui描画
	void RenderGui();

	//値の登録
	template<typename T>
	void SetValue(const std::string& variable_name, const T& variable_value)
	{
#ifdef _DEBUG
		//事前登録リストに指定された名が存在するか検索
		if (allowed_variables.find(variable_name) == allowed_variables.end())
		{
			std::cerr << "Error : 名 [" << variable_name << "] は事前登録されていません。\n";
			return;
		}
#endif // _DEBUG

		uint32_t hash_key = CalculateHash(variable_name);	//検索用の32ビットハッシュ値
		data_map[hash_key].value = variable_value;
		name_map[hash_key] = variable_name;
	}

	//値の取得
	template<typename T>
	T GetValue(const std::string& variable_name, const T& default_value)const
	{
		//ハッシュ値でのデータ検索
		uint32_t hash_key = CalculateHash(variable_name);				//検索用の32ビットハッシュ値
		auto iterator = data_map.find(hash_key);	//検索した値

		//名が登録されていない際のエラー処理
		if (iterator == data_map.end())
		{
			auto name_it = name_map.find(hash_key);
			std::string original_name = (name_it != name_map.end()) ? name_it->second : variable_name;	//出力用の文字列

			std::cerr << "Error : 指定された名［" << original_name << "]は登録されていません。\n";
			return default_value;
		}

		//取得したの型が要求された方と一致するか確認
		const T* target_value = std::get_if<T>(&(iterator->second.value));	//対象の値

		//型が一致しなかった場合
		if (!target_value)
		{
			auto name_it = name_map.find(hash_key);
			std::string original_name = (name_it != name_map.end()) ? name_it->second : variable_name;	//出力用の文字列

			std::cerr << "Error:  [" << original_name << "] の型が一致しません。\n";
			return default_value;
		}
		return *target_value;
	}

	//値の取得(ハッシュキー指定)
	template<typename T>
	T GetValue(uint32_t hash_key, const T& default_value)const
	{
		auto iterator = data_map.find(hash_key);	//ハッシュからデータを検索

		//データが存在しない場合の安全処理
		if (iterator == data_map.end())
		{
			return default_value;
		}

		const T* target_value = std::get_if<T>(&(iterator->second.value));	//ポインタキャスト

		//型が不一致の場合の安全処理
		if (!target_value)
		{
			return default_value;
		}
		return *target_value;
	}

	//コンパイル時のハッシュ
	static constexpr uint32_t CalculateHash(const std::string_view& target_string)
	{
		uint32_t hash_value = 2166136261u;

		for (char c : target_string)
		{
			hash_value ^= static_cast<uint32_t>(c);
			hash_value *= 16777619u;
		}
		return hash_value;
	}

	//ハッシュキーからデータの値を取得
	const BlackboardData& GetAttributeValue(uint32_t hash_key)const;

	//の事前登録
	void RegisterVariable(const std::string& variable_name) { allowed_variables.insert(variable_name); }

	//名リストを取得
	std::vector<std::string> GetRegisteredVariableNames()const;

	//ハッシュキーから名を取得
	std::string GetVariableNameFromHash(uint32_t hash_key) const;

	//名からハッシュキーを取得
	uint32_t GetVariableHash(const std::string& variable_name)const;

	//の最小値制限を取得
	float GetMinLimit(uint32_t hash_key) const { auto it = data_map.find(hash_key); return (it != data_map.end()) ? it->second.min_value : 0.0f; }

	//の最大値制限を取得
	float GetMaxLimit(uint32_t hash_key) const { auto it = data_map.find(hash_key); return (it != data_map.end()) ? it->second.max_value : 100.0f; }

	//指定されたの変化スピード（感度）を取得
	float GetChangeSpeed(uint32_t hash_key) const { auto it = data_map.find(hash_key); return (it != data_map.end()) ? it->second.speed : 0.1f; }

private:
	//ブラックボードのデータとその属性を管理
	struct BlackboardAttribute
	{
		BlackboardData value;		//実際のデータ
		float min_value = 0.0f;		//最小値制限
		float max_value = 100.0f;	//最大値制限
		float speed = 0.1f;			//移動時の変化感度
		std::string tooltip = "";	//の説明文
	};

	//ImGuiの型描画を担当する関数オブジェクト
	struct GuiVisitor
	{
		std::string name;	//名
		float min_val;		//最小値制限
		float max_val;		//最大値制限
		float speed_val;	//移動時の感度

		//コンストラクタ
		GuiVisitor(const std::string& variable_name, float min_v, float max_v, float speed)
			:name(variable_name), min_val(min_v), max_val(max_v), speed_val(speed)
		{

		}

		//int型用の描画・編集処理
		void operator()(int& val) { ImGui::DragInt(name.c_str(), &val, speed_val, static_cast<int>(min_val), static_cast<int>(max_val)); }

		//float型用の描画・編集処理
		void operator()(float& val) { ImGui::DragFloat(name.c_str(), &val, speed_val, min_val, max_val); }

		//DirectX::MFLOAT3用の描画・編集処理
		void operator()(DirectX::XMFLOAT3& val) { ImGui::DragFloat3(name.c_str(), &val.x, speed_val, min_val, max_val); }

		//bool型用の描画・編集処理
		void operator()(bool& val) { ImGui::Checkbox(name.c_str(), &val); }

		void operator()(std::vector<std::string>& val)
		{
			//ツリーを展開してリストの中にを見えるようにする
			if (ImGui::TreeNode(name.c_str()))
			{
				for (size_t i = 0; i < val.size(); i++)
				{
					ImGui::Text("[%zn] : %s", i, val[i].c_str());
				}
				ImGui::TreePop();
			}
		}

		//uint32_t型用の描画・編集
		void operator()(uint32_t& val) { ImGui::Text("%s (Hash) : %u", name.c_str(), val); }
	};

private:
	std::unordered_map<uint32_t, BlackboardAttribute> data_map;	//名とデータを保存する辞書
	std::unordered_map<uint32_t,std::string> name_map;			//デバッグ出力用の辞書
	std::unordered_set<std::string> allowed_variables;			//事前登録された名リスト
};

struct CompareVisitor
{
	CompareOperator op;			//比較演算子
	BlackboardData ref_val;		//基準値

	//コンストラクタ
	CompareVisitor(CompareOperator compare_op, const BlackboardData& reference_value)
		:op(compare_op), ref_val(reference_value)
	{
	}

	//float型用の比較判定処理
	bool operator()(float& val)
	{
		const float* ref = std::get_if<float>(&ref_val);	//基準値

		//基準値が違う型だった場合
		if (!ref) { return false; }

		//演算子に応じた比較処理
		switch (op)
		{
		case CompareOperator::Equal:		return static_cast<float>(val) == *ref; break;
		case CompareOperator::NotEqual:		return static_cast<float>(val) != *ref; break;
		case CompareOperator::Greater:		return static_cast<float>(val) > *ref;  break;
		case CompareOperator::Less:			return static_cast<float>(val) < *ref;  break;
		case CompareOperator::GreaterEqual:	return static_cast<float>(val) >= *ref; break;
		case CompareOperator::LessEqual:	return static_cast<float>(val) <= *ref; break;
		default:							return false;			  break;
		}
	}

	//int型用の比較判定処理
	bool operator()(int& val)
	{
		const int* ref = std::get_if<int>(&ref_val);	//基準値

		//基準値が違う型だった場合
		if (!ref) { return false; }

		//演算子に応じた比較処理
		switch (op)
		{
		case CompareOperator::Equal:		return val == *ref; break;
		case CompareOperator::NotEqual:		return val != *ref; break;
		case CompareOperator::Greater:		return val > *ref;	break;
		case CompareOperator::Less:			return val < *ref;	break;
		case CompareOperator::GreaterEqual:	return val >= *ref; break;
		case CompareOperator::LessEqual:	return val <= *ref; break;
		default:							return false;		break;
		}
	}

	//bool型用の比較判定処理
	bool operator()(bool& val)
	{
		const bool* ref = std::get_if<bool>(&ref_val);	//基準値

		//基準値が違う型だった場合
		if (!ref) { return false; }

		//演算子に応じた比較処理
		switch (op)
		{
		case CompareOperator::Equal:		return val == *ref; break;
		case CompareOperator::NotEqual:		return val != *ref; break;
		default:							return false;		break;
		}
	}

	//DirectX::XMFLOAT3型用の比較判定処理
	bool operator()(DirectX::XMFLOAT3& val)
	{
		//基準値が XMFLOAT3 型として格納されているか判定
		const DirectX::XMFLOAT3* ref_vec = std::get_if<DirectX::XMFLOAT3>(&ref_val);
		//基準値が float 型（共通閾値）として格納されているか判定
		const float* ref_float = std::get_if<float>(&ref_val);

		//基準値の型がいずれでもない（比較不可能）場合は false
		if (!ref_vec && !ref_float) { return false; }

		//各成分ごとの比較判定を行うラムダ式
		auto compare_component = [this](float value, float reference) -> bool
		{
			constexpr float float_epsilon = 0.0001f; // 浮動小数点数比較時の誤差吸収用定数
			switch (op)
			{
			case CompareOperator::Equal:        return std::abs(value - reference) < float_epsilon;
			case CompareOperator::NotEqual:     return std::abs(value - reference) >= float_epsilon;
			case CompareOperator::Greater:      return value > reference;
			case CompareOperator::Less:         return value < reference;
			case CompareOperator::GreaterEqual: return value >= reference;
			case CompareOperator::LessEqual:    return value <= reference;
			default:                            
				printf("Error: CompareVisitor - 未定義の比較演算子 (%d) が指定されました。\n", static_cast<int>(op));
				return false;
			}
		};

		//基準値の型に応じて X, Y, Z それぞれの比較ターゲット値を決定
		float ref_x = ref_vec ? ref_vec->x : *ref_float; // X成分の基準値
		float ref_y = ref_vec ? ref_vec->y : *ref_float; // Y成分の基準値
		float ref_z = ref_vec ? ref_vec->z : *ref_float; // Z成分の基準値

		//X, Y, Z の全成分が指定した比較演算子の条件を満たしているか判定
		return compare_component(val.x, ref_x) &&
			compare_component(val.y, ref_y) &&
			compare_component(val.z, ref_z);
	}

	//比較を行わないためfalseを返す
	bool operator()(std::vector<std::string>& val)
	{
		return false;
	}

	//uint32_t用の比較評価処理
	bool operator()(uint32_t& val)
	{
		const uint32_t* ref = std::get_if<uint32_t>(&ref_val);
		if (!ref) { return false; }
		switch (op)
		{
		case CompareOperator::Equal:		return val == *ref; break;
		case CompareOperator::NotEqual:		return val != *ref; break;
		default:							return false;		break;
		}
	}

};

//遷移条件
struct TransitionCondition
{
	ConditionNodeType type = ConditionNodeType::NormalCompare;	//判定タイプ
	uint32_t hash_key;				//対象のハッシュ値
	BlackboardData reference_value;	//基準値
	CompareOperator compart_op = CompareOperator::Equal;	//比較演算子
	float param_second = 0.0f;		//第２引数パラメータ
	uint32_t secondary_hash = 0;	//比較対象のハッシュキー
	DirectX::XMFLOAT3 vector_reference_value = { 0.0f, 0.0f, 0.0f }; // XMFLOAT3用の比較基準値

	//条件を満たしているか判定
	bool IsJudgment(const StateBlackboard& blackboard)const;
};