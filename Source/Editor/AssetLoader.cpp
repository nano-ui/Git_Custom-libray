#include "Editor\AssetLoader.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Graphics\Resources\Model.h"

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

	auto device_context = Graphics::Instance().GetDevice();	//デバイス

	//グラフィックデバイスの有効性を確認
	if (!device_context)
	{
		printf("Error: LoadModelAnimations - Graphics Device が nullptr です。\n");
		return false;
	}

	std::unique_ptr<Model> temporary_model = std::make_unique<Model>();	//一時的なモデル読み込み
	temporary_model->Initialize(model_path);

	//モデルの読み込み成否を確認
	if (temporary_model)
	{
		animation_names = temporary_model->GetAnimationNames();
	}
	else
	{
		printf("Error: LoadModelAnimations - モデル「%s」の読み込みに失敗しました。\n", model_path.c_str());
		return false;
	}

	return true;
}
