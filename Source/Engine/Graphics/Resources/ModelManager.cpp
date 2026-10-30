#include "ModelManager.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModelData.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModelRenderer.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "ThiedParty\json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <Windows.h>

//インスタンス取得
ModelManager& ModelManager::Instance()
{
	static ModelManager instance;
	return instance;
}

//モデルデータのロードと登録
std::shared_ptr<GltfModelData> ModelManager::LoadModelData(const std::string& file_path)
{
	const std::string key_path = NormalizePath(file_path);

	auto iterator = model_cache.find(key_path);
	if (iterator != model_cache.end())
	{
		std::shared_ptr<GltfModelData> cached_data = iterator->second.lock();
		if (cached_data)
		{
			return cached_data;
		}
	}

	ID3D11Device* device = Graphics::Instance().GetDevice();

	if (!device)
	{
		OutputDebugStringA("[ModelManager エラー] LoadModelData: ID3D11Device が nullptr です。\n");
		return nullptr;
	}

	std::shared_ptr<GltfModelData> new_model_data = GltfModelData::Load(device, file_path);
	if (!new_model_data)
	{
		std::string debug_message = "[ModelManager エラー] モデルファイルのロードに失敗しました。 パス: " + file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
		return nullptr;
	}

	model_cache[key_path] = new_model_data;
	return new_model_data;
}

//モデルデータの一括読み込み
void ModelManager::PreloadModels(const std::vector<std::string>& file_paths)
{
	//渡されたパスリストをループして順番にロード処理を実行
	for (const std::string& file_path : file_paths)
	{
		std::shared_ptr<GltfModelData>loaded_data = LoadModelData(file_path);
		if (!loaded_data)
		{
			std::string debug_message = "[ModelManager 警告] PreloadModels: 事前ロードに失敗しました。 パス: " + file_path + "\n";
			OutputDebugStringA(debug_message.c_str());
		}
	}
}

//Jsonファイルから事前ロードリストを一括読み込み
bool ModelManager::LoadPreloadListFromJson(const std::string& json_file_path)
{
	std::ifstream input_file(json_file_path);

	if (!input_file.is_open())
	{
		std::string debug_message = "[ModelManager 警告] LoadPreloadListFromJson: JSONファイルが開けません。 パス: " + json_file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
		return false;
	}

	nlohmann::json preload_json;
	input_file >> preload_json;
	input_file.close();

	if (!preload_json.contains("preload_models") || !preload_json["preload_models"].is_array())
	{
		std::string debug_message = "[ModelManager エラー] LoadPreloadListFromJson: JSON内に 'preload_models' 配列が存在しません。 パス: " + json_file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
		return false;
	}

	std::vector<std::string>file_paths;
	for (const auto& item : preload_json["preload_models"])
	{
		if (item.is_string())file_paths.push_back(item.get<std::string>());
	}

	PreloadModels(file_paths);
	return false;
}

//事前ロード用リストをJsonファイルとして保存作成
bool ModelManager::SavePreloadListToJson(const std::vector<std::string>& file_paths, const std::string& json_file_path)
{
	std::filesystem::path target_path(json_file_path);

	//親フォルダがない場合は生成
	if (target_path.has_parent_path())
	{
		std::filesystem::create_directories(target_path.parent_path());
	}

	nlohmann::json preload_json;
	preload_json["preload_models"] = file_paths;

	constexpr int JSON_INDENT_SPACES = 4;
	std::ofstream output_file(json_file_path);

	if (!output_file.is_open())
	{
		std::string debug_message = "[ModelManager エラー] SavePreloadListToJson: ファイルの保存に失敗しました。 パス: " + json_file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
		return false;
	}

	output_file << preload_json.dump(JSON_INDENT_SPACES);
	output_file.close();
	return true;
}

//登録済みモデルデータ取得
std::shared_ptr<GltfModelData> ModelManager::GetModelData(const std::string& file_path) const
{
	const std::string key_path = NormalizePath(file_path);

	auto iterator = model_cache.find(key_path);
	if (iterator != model_cache.end())
	{
		std::shared_ptr<GltfModelData> cached_data = iterator->second.lock();
		if (cached_data)
		{
			return cached_data;
		}
	}

	std::string debug_message = "[ModelManager 警告] 指定されたモデルデータが見つからないか破棄されています。 パス: " + file_path + "\n";
	OutputDebugStringA(debug_message.c_str());
	return nullptr;
}

//指定したモデルデータを削除
void ModelManager::UnloadModelData(const std::string& file_path)
{
	const std::string key_path = NormalizePath(file_path);

	auto iterator = model_cache.find(key_path);
	if (iterator != model_cache.end())
	{
		model_cache.erase(iterator);
	}
	else
	{
		std::string debug_message = "[ModelManager 警告] 削除対象のモデルデータが存在しません。 パス: " + file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
	}
}

//全てのモデルデータキャッシュを開放
void ModelManager::Clear()
{
	model_cache.clear();
	renderer.reset();
}

//モデル描画を取得
std::shared_ptr<GltfModelRenderer> ModelManager::GetRenderer()
{
	if (!renderer)
	{
		ID3D11Device* device = Graphics::Instance().GetDevice();
		if (!device)
		{
			OutputDebugStringA("[ModelManager エラー] GetRenderer: ID3D11Device が nullptr です。\n");
			return nullptr;
		}

		renderer = std::make_shared<GltfModelRenderer>(device);
		if (!renderer)
		{
			OutputDebugStringA("[ModelManager エラー] GetRenderer: GltfModelRenderer の生成に失敗しました。\n");
			return nullptr;
		}
	}
	return renderer;
}

//使われていないモデルデータの一括開放
void ModelManager::UnloadUnusedModels()
{
	for (auto iterator = model_cache.begin(); iterator != model_cache.end();)
	{
		if (iterator->second.expired())
		{
			std::string debug_message = "[ModelManager 情報] 参照が切れたモデルデータを自動解放しました。 パス: " + iterator->first + "\n";
			OutputDebugStringA(debug_message.c_str());
			iterator = model_cache.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}
}

//パスの表記揺れを統一
std::string ModelManager::NormalizePath(const std::string& file_path) const
{
	std::filesystem::path path(file_path);
	std::string normalized = path.lexically_normal().string();
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
	return normalized;
}
