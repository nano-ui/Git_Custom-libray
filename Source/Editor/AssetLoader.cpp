#include "AssetLoader.h"
#include <filesystem>

//コンストラクタ
AssetLoader::AssetLoader()
{

}

//モデルファイルを読み込んでアニメーション名リストを解析・抽出
bool AssetLoader::LoadModelAnimations(const std::string& model_path)
{
	//パスが空でないか確認
	if (model_path.empty())
	{
		return false;
	}

	loaded_model_path = model_path;
	animation_names.clear();

	return true;
}
