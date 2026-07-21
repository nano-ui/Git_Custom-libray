#include "AnimationSequenceSerializer.h"
#include "ThiedParty\json.hpp"

#include <fstream>
#include <filesystem>
#include <windows.h>

static const std::string BASE_JSON_DIR = "Data/Json/";	//保存用ベースフォルダ

//指定モデルの全アニメーション設定をJSONファイルに一括保存
bool AnimationSequenceSerializer::SaveToFile(
	const std::string& model_name,
	const std::unordered_map<std::string, AnimationSequenceData>& sequence_map)
{
	std::string clean_name = ExtractCleanModelName(model_name);
	std::string file_path = GetFullFilePath(clean_name);

	//保存用Jsonオブジェクトを構築
	nlohmann::json root_json;
	root_json["model_name"] = clean_name;

	nlohmann::json sequences_json = nlohmann::json::object();

	//全アニメーション設定を巡回してJSONに変換
	for (const auto& pair : sequence_map)
	{
		const std::string& anim_name = pair.first;
		const AnimationSequenceData& seq_data = pair.second;

		nlohmann::json anim_json;
		anim_json["animation_duration"] = seq_data.animation_duration;
		anim_json["effective_duration"] = seq_data.effective_duration;

		nlohmann::json keyframes_array = nlohmann::json::array();
		for (const auto& kf : seq_data.keyframes)
		{
			nlohmann::json kf_json;
			kf_json["sequencer_time"] = kf.sequencer_time;
			kf_json["speed_multiplier"] = kf.speed_multiplier;
			keyframes_array.push_back(kf_json);
		}
		anim_json["keyframes"] = keyframes_array;
		sequences_json[anim_name] = anim_json;
	}
	
	root_json["animation_sequences"] = sequences_json;

	//ファイルへ書き出し
	std::ofstream ofs(file_path);
	if (!ofs.is_open())
	{
		char err_buf[256];
		sprintf_s(err_buf, "[Serializer Error] Failed to open file for writing: %s\n", file_path.c_str());
		OutputDebugStringA(err_buf);
		return false;
	}

	//4インデントで整形出力
	ofs << root_json.dump(4);
	ofs.close();

	char log_buf[256];
	sprintf_s(log_buf, "[Serializer] Successfully saved sequence data to: %s\n", file_path.c_str());
	OutputDebugStringA(log_buf);

	return true;
}

//指定モデルのJSONファイルから全アニメーション設定を一括読み込み
bool AnimationSequenceSerializer::LoadFromFile(
	const std::string& model_name,
	std::unordered_map<std::string, AnimationSequenceData>& out_sequence_map)
{
	out_sequence_map.clear();

	std::string clean_name = ExtractCleanModelName(model_name);
	std::string file_path = GetFullFilePath(clean_name);

	std::ifstream ifs(file_path);
	if (!ifs.is_open())
	{
		char err_buf[256];
		sprintf_s(err_buf, "[Serializer Warning] Sequence file not found: %s\n", file_path.c_str());
		OutputDebugStringA(err_buf);
		return false;
	}

	nlohmann::json root_json;
	try
	{
		ifs >> root_json;
	}
	catch (...)
	{
		OutputDebugStringA("[Serializer Error] Failed to parse JSON file!\n");
		return false;
	}

	// 必須キーの存在チェック
	if (!root_json.contains("animation_sequences"))
	{
		OutputDebugStringA("[Serializer Error] JSON missing 'animation_sequences' key!\n");
		return false;
	}

	// JSONから各アニメーション設定を復元
	const auto& sequences_json = root_json["animation_sequences"];
	for (auto it = sequences_json.begin(); it != sequences_json.end(); ++it)
	{
		std::string anim_name = it.key();
		const auto& anim_json = it.value();

		AnimationSequenceData seq_data;
		if (anim_json.contains("animation_duration"))
		{
			seq_data.animation_duration = anim_json["animation_duration"].get<float>();
		}
		if (anim_json.contains("effective_duration"))
		{
			seq_data.effective_duration = anim_json["effective_duration"].get<float>();
		}

		if (anim_json.contains("keyframes") && anim_json["keyframes"].is_array())
		{
			for (const auto& kf_json : anim_json["keyframes"])
			{
				SequenceKeyframe kf;
				if (kf_json.contains("sequencer_time"))
				{
					kf.sequencer_time = kf_json["sequencer_time"].get<float>();
				}
				if (kf_json.contains("speed_multiplier"))
				{
					kf.speed_multiplier = kf_json["speed_multiplier"].get<float>();
				}
				seq_data.keyframes.push_back(kf);
			}
		}

		out_sequence_map[anim_name] = seq_data;
	}

	char log_buf[256];
	sprintf_s(log_buf, "[Serializer] Successfully loaded sequence data from: %s\n", file_path.c_str());
	OutputDebugStringA(log_buf);

	return true;
}

//パスや拡張子から純粋なモデル名を抽出
std::string AnimationSequenceSerializer::ExtractCleanModelName(const std::string& raw_model_name)
{
	if (raw_model_name.empty())return "DefaultModel";

	//パス構造体を利用してファイル名を取得
	std::filesystem::path path_obj(raw_model_name);
	return path_obj.stem().string();
}

//ディレクトリパスを取得し、存在しない場合は自動生成
std::string AnimationSequenceSerializer::GetDirectoryPath(const std::string& clean_model_name)
{
	std::string dir_path = BASE_JSON_DIR + clean_model_name;
	std::error_code ec;

	//フォルダが存在しない場合作成
	if (!std::filesystem::exists(dir_path, ec))
	{
		std::filesystem::create_directories(dir_path, ec);
		if (ec)
		{
			char err_buf[256];
			sprintf_s(err_buf, "[Serializer Error] Failed to create directory: %s\n", dir_path.c_str());
			OutputDebugStringA(err_buf);
		}
	}
	return dir_path;
}

//保存・読み込み用のフルJSONファイルパスを取得
std::string AnimationSequenceSerializer::GetFullFilePath(const std::string& clean_model_name)
{
	std::string dir_path = GetDirectoryPath(clean_model_name);
	return dir_path + "/" + clean_model_name + "_Sequence.json";
}
