#include "ModelManager.h"
#include "Engine\Graphics\Resources\GltfModel\GltfModelData.h"
#include "Engine\Graphics\Renderers\Graphics.h"

#include <algorithm>
#include <filesystem>
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
	//検索用にパスを正規化
	const std::string key_path = NormalizePath(file_path);

	//既に登録済みか検索
	auto iterator = model_registry.find(key_path);
	if (iterator != model_registry.end())return iterator->second;

	ID3D11Device* device = Graphics::Instance().GetDevice();

	if (!device)
	{
		OutputDebugStringA("[ModelManager エラー] LoadModelData: ID3D11Device が nullptr です。\n");
		return nullptr;
	}

	//新規モデルデータの読み込み
	std::shared_ptr<GltfModelData> new_model_data = GltfModelData::Load(device, file_path);
	if (!new_model_data)
	{
		std::string debug_message = "[ModelManager エラー] モデルファイルのロードに失敗しました。 パス: " + file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
		return nullptr;
	}

	//正規化したパスをキーにして登録
	model_registry[key_path] = new_model_data;
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

//登録済みモデルデータ取得
std::shared_ptr<GltfModelData> ModelManager::GetModelData(const std::string& file_path) const
{
	const std::string key_path = NormalizePath(file_path);

	auto iterator = model_registry.find(key_path);
	if (iterator != model_registry.end())return iterator->second;
	std::string debug_message = "[ModelManager 警告] 指定されたモデルデータが見つかりません。 パス: " + file_path + "\n";
	OutputDebugStringA(debug_message.c_str());
	return nullptr;
}

//指定したモデルデータを削除
void ModelManager::UnloadModelData(const std::string& file_path)
{
	const std::string key_path = NormalizePath(file_path);

	auto iterator = model_registry.find(key_path);
	if (iterator != model_registry.end())model_registry.erase(iterator);
	else
	{
		std::string debug_message = "[ModelManager 警告] 削除対象のモデルデータが存在しません。 パス: " + file_path + "\n";
		OutputDebugStringA(debug_message.c_str());
	}
}

//使われていないモデルデータの一括開放
void ModelManager::UnloadUnusedModels()
{
	constexpr long UNUSED_REFERENCE_COUNT = 1;

	//登録されているモデルの参照カウントを確認しながら安全にループ削除
	for (auto iterator = model_registry.begin(); iterator != model_registry.end();)
	{
		if (iterator->second.use_count() == UNUSED_REFERENCE_COUNT)
		{
			std::string debug_message = "[ModelManager 情報] 未使用のモデルデータを自動解放しました。 パス: " + iterator->first + "\n";
			OutputDebugStringA(debug_message.c_str());
			iterator = model_registry.erase(iterator);
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
