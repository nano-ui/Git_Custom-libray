#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <unordered_map>

class GltfModelData;

class ModelManager
{
public:
	//インスタンス取得
	static ModelManager& Instance();

	//モデルデータのロードと登録
	std::shared_ptr<GltfModelData> LoadModelData(const std::string& file_path);

	//モデルデータの一括読み込み
	void PreloadModels(const std::vector<std::string>& file_paths);

	//登録済みモデルデータ取得
	std::shared_ptr<GltfModelData> GetModelData(const std::string& file_path)const;

	//指定したモデルデータを削除
	void UnloadModelData(const std::string& file_path);

	//使われていないモデルデータの一括開放
	void UnloadUnusedModels();

	//全てのモデルデータキャッシュを開放
	void Clear();

private:
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator = (const ModelManager&) = delete;

	//パスの表記揺れを統一
	std::string NormalizePath(const std::string& file_path)const;

private:
	std::unordered_map<std::string, std::shared_ptr<GltfModelData>> model_registry;	//識別名とモデルデータを紐づけて保持するマップ
};

