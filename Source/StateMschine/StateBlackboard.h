#pragma once

#include <DirectXMath.h>
#include <unordered_map>
#include <string>
#include <memory>

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

class IBlackboardParam
{
public:
	virtual ~IBlackboardParam() = default;
};

//型を保持
template<typename T>
class TypedBlackboardParam :public IBlackboardParam
{
public:
	TypedBlackboardParam(const T& initial_value):value(initial_value){}
	~TypedBlackboardParam()override = default;

public:
	T value;
};


class StateBlackboard
{
public:
	//コンストラクタ
	StateBlackboard();

	//デストラクタ
	~StateBlackboard();

	//汎用アクセス関数
	bool IsActionPressed(const std::string& action_name)const;

	//入力状態を書き込む
	void SetActionState(const std::string& action_name, bool is_pressed);

	//あらゆる型に対応したパラメータ書き込み関数
	template<typename T>
	void SetParameter(const std::string& param_name, const T& value)
	{
		//既存のパラメータがあるか検索
		auto it = generic_parameters.find(param_name);
		if (it != generic_parameters.end())
		{
			auto typed_param = dynamic_cast<TypedBlackboardParam<T>*>(it->second.get());
			assert(typed_param != nullptr && "StateBlackboard::SetParameter - Type mismatch for existing parameter.");
			if (typed_param)
			{
				typed_param->value = value;
				return;
			}
		}
		generic_parameters[param_name] = std::make_unique<TypedBlackboardParam<T>>(value);
	}

	//あらゆる型に対応したパラメータ読み込み関数
	template<typename T>
	T GetParameter(const std::string& param_name)const
	{
		auto it = generic_parameters.find(param_name);
		if (it == generic_parameters.end())
		{
			return T;
		}

		auto typed_param = dynamic_cast<TypedBlackboardParam<T>*>(it->second.get());

		assert(typed_param != nullptr && "StateBlackboard::GetParameter - Requested type does not match stored type.");
		if (!typed_param)
		{
			return T();
		}

		return typed_param->value;
	}

public:
	DirectX::XMFLOAT3 move_input;	//移動入力
	uint32_t current_input_flags;	//合成された入力ビットフラグ

	ActionCategory action_category;	//アクションカテゴリー
	int sub_action_id;				//技ID等の詳細な識別番号
	float target_move_speed;		//目標移動速度

	bool is_grounded;					//接地フラグ
	DirectX::XMFLOAT3 current_velocity;	//現在の速度

private:
	std::unordered_map<std::string, bool> action_states;	//データマップ
	std::unordered_map<std::string, std::unique_ptr<IBlackboardParam>> generic_parameters;	//あらゆる型を一括管理できるコンテナ
};

