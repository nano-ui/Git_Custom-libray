#pragma once

#include <string>
#include <vector>

class AssetLoader
{
public:
	//コンストラクタ
	AssetLoader();

	//デストラクタ
	~AssetLoader() = default;

	//モデルファイルを読み込んでアニメーション名リストを解析・抽出
	bool LoadModelAnimations(const std::string& model_path);

	//モデルパスを取得
	const std::string& GetLoadedModelPath()const { return loaded_model_path; }

	//アニメーション名リストを取得
	const std::vector<std::string>& GetAnimationNames()const { return animation_names; }

private:
	std::string loaded_model_path = "";			//読み込んだモデルパス
	std::vector<std::string> animation_names;	//抽出したアニメーション名のリスト
};

