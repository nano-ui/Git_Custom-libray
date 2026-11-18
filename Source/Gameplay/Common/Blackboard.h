#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <cstdint>
#include <memory>
#include <string_view>

#include "Engine\Core\Log\Log.h"

//ブラックボード
class Blackboard
{
private:
	//使用できる型
	using BlackboardValue = std::variant<int, float, bool, uint32_t, std::string, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, std::vector<uint32_t>, std::vector<std::string>>;
	
	//ブラックボード登録情報
	struct BlackboardEntry
	{
		BlackboardValue value;						//現在の値
		std::function<BlackboardValue()> getter;	//最新値取得
	};
public:
	//コンストラクタ
	Blackboard() = default;

	//デストラクタ
	~Blackboard() = default;

	//値の登録
	template<typename Getter>
	void BindVariable(const std::string& variable_name, const Getter& getter)
	{
		//ガード処理
		if (variable_name.empty())
		{
			LOG_ERROR({ u8"設定" }, u8"設定された文字列が空でした");
			return;
		}
		std::function<BlackboardValue()> value = getter;
		if (!value)
		{
			LOG_ERROR({ u8"設定" }, u8"設定された変数の型が登録できる型ではありませんでした");
			return;
		}
		uint32_t hash_key = CalculateHash(variable_name);
		auto iterator = data_map.find(hash_key);
		if (iterator != data_map.end())
		{
			LOG_WARNING({ u8"設定" }, u8"同じ値を登録しようとしているため弾きます(変数名:%s)", variable_name.c_str());
			return;
		}

		//辞書への登録
		data_map[hash_key].getter = value;
		data_map[hash_key].value = value();
		name_map[hash_key] = variable_name;
	}

	//値の設定
	template<typename T>
	void SetValue(const std::string& variable_name, const T& value)
	{
		//ガード処理
		if (variable_name.empty())
		{
			LOG_ERROR({ u8"設定" }, u8"設定された文字列が空でした");
			return;
		}
		if constexpr(std::is_constructible_v<BlackboardValue, T>)
		{
			//設定
			uint32_t hash_key = CalculateHash(variable_name);
			data_map[hash_key].getter = nullptr;
			data_map[hash_key].value = value;
			name_map[hash_key] = variable_name;
		}
		else
		{
			LOG_ERROR({ u8"設定" }, u8"設定された変数の型が登録できる型ではありませんでした(登録名:%s)", variable_name.c_str());
			return;
		}
	}

	//文字列での値の取得
	template<typename T>
	T GetValue(const std::string& variable_name, const T& value)const
	{
		//情報の検索
		uint32_t hash_key = CalculateHash(variable_name);
		auto iterator = data_map.find(hash_key);
		if (iterator == data_map.end())
		{
			std::shared_ptr<Blackboard> blackboard = parent_blackboard.lock();
			if (!blackboard)return value;
			return blackboard->GetValue(variable_name, value);
		}
		//紐づけ情報の取得
		BlackboardValue target_value = iterator->second.value;
		if (iterator->second.getter)target_value = iterator->second.getter();
		const T* result_value = std::get_if<T>(&target_value);
		if (!result_value)
		{
			LOG_WARNING({ u8"取得" }, u8"登録した時の型と取得しようとした変数の型が異なっています(変数名:%s)", variable_name.c_str());
			return value;
		}
		return *result_value;;
	}

	//ハッシュ値での値の取得
	template<typename T>
	T GetValue(const std::uint32_t& variable_hash, const T& value)const
	{
		auto iterator = data_map.find(variable_hash);
		if (iterator == data_map.end())
		{
			std::shared_ptr<Blackboard> blackboard = parent_blackboard.lock();
			if (!blackboard)return value;
			return blackboard->GetValue(variable_hash, value);
		}
		//紐づけ情報の取得
		BlackboardValue target_value = iterator->second.value;
		if (iterator->second.getter)target_value = iterator->second.getter();
		const T* result_value = std::get_if<T>(&target_value);
		if (!result_value)
		{
			LOG_WARNING({ u8"取得" }, u8"登録した時の型と取得しようとした変数の型が異なっています");
			return value;
		}
		return *result_value;
	}


	//文字列での値の取得できるか確認
	template<typename T>
	bool TryGetValue(const std::string& variable_name, T& value)const
	{
		//情報の検索
		uint32_t hash_key = CalculateHash(variable_name);
		auto iterator = data_map.find(hash_key);
		if (iterator == data_map.end())
		{
			std::shared_ptr<Blackboard> blackboard = parent_blackboard.lock();
			if (!blackboard)return false;
			return blackboard->TryGetValue(variable_name, value);
		}
		//紐づけ情報の取得
		BlackboardValue target_value = iterator->second.value;
		if (iterator->second.getter)target_value = iterator->second.getter();
		const T* result_value = std::get_if<T>(&target_value);
		if (!result_value)
		{
			return false;
		}
		value = *result_value;
		return true;
	}

	//ハッシュ値での値の取得できるか確認
	template<typename T>
	bool TryGetValue(const std::uint32_t& variable_hash, T& value)const
	{
		auto iterator = data_map.find(variable_hash);
		if (iterator == data_map.end())
		{
			std::shared_ptr<Blackboard> blackboard = parent_blackboard.lock();
			if (!blackboard)return false;
			return blackboard->TryGetValue(variable_hash, value);
		}
		//紐づけ情報の取得
		BlackboardValue target_value = iterator->second.value;
		if (iterator->second.getter)target_value = iterator->second.getter();
		const T* result_value = std::get_if<T>(&target_value);
		if (!result_value)
		{
			return false;
		}
		value = *result_value;
		return true;
	}

	//親ブラックボードの設定
	void SetParent(std::shared_ptr<Blackboard> blackboard) { parent_blackboard = blackboard; }

	//変数名リストの取得
	std::vector<std::string> GetRegisteredNames()const;

	//変数名のIDを取得
	uint32_t GetKeyId(const std::string& variable_name)const { return CalculateHash(variable_name); }

private:
	//文字列をビットに変換
	static constexpr uint32_t CalculateHash(std::string_view variable_name)
	{
		uint32_t hash_value = FNV_OFFSET_BASIC;

		for (char c : variable_name)
		{
			hash_value ^= static_cast<uint32_t>(c);
			hash_value *= FNV_PRIME;
		}
		return hash_value;
	}

private:
	static constexpr uint32_t FNV_OFFSET_BASIC = 2166136261u;	//ハッシュ計算のスタート地点
	static constexpr uint32_t FNV_PRIME = 16777619u;			//掛け合わせる素数

private:
	std::unordered_map<uint32_t, BlackboardEntry> data_map;	//登録情報の辞書
	std::unordered_map<uint32_t, std::string> name_map;		//登録名の辞書
	std::weak_ptr<Blackboard> parent_blackboard;			//親ブラックボード
};
