#include "Model.h"
#include <filesystem>
#include <stdexcept>

#include "../FbxModel/FbxSkinnedResource.h"
#include "../FbxModel/FbxSkinnedModel.h"
#include "../GltfModel/GltfModelData.h"
#include "../GltfModel/GltfModelRenderer.h"
#include "../GltfModel/GltfModel.h"


//各モデルコンポーネントの共通インターフェース
class Model::ModelImpl
{
public:
	//デストラクタ
	virtual  ~ModelImpl() = default;

	//更新処理
	virtual void Update(float elapsed_time) = 0;

	//描画処理
	virtual void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color) = 0;

	//アニメーション再生
	virtual void PlayAnimation(const std::string& animation_name, bool is_loop) = 0;
};

//FBXモデルを扱うための実装クラス
class FbxModelImpl : public Model::ModelImpl 
{
private:
	std::shared_ptr<FbxSkinnedResource> resource;	//FBXのリソースデータを保持
	std::unique_ptr<FbxSkinnedModel> model;			//FBXのモデル描画本体を保持
public:
	//コンストラクタ
	FbxModelImpl(ID3D11Device* device, const std::string& file_path)
	{
		resource = std::make_shared<FbxSkinnedResource>(device);	//リソースクラスのインスタンス生成
		resource->Load(file_path);									//FBXデータ読み込み
		model = std::make_unique<FbxSkinnedModel>(resource);		//モデル本体を生成
	}

	//更新処理
	void Update(float elapsed_time)override
	{
		model->AnimationUpdate(elapsed_time);
	}

	//描画処理
	void Render(ID3D11DeviceContext* contex, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color)override
	{
		model->Render(contex, world, color);
	}

	//アニメーション再生
	void PlayAnimation(const std::string& animation_name, bool is_loop)override
	{
		model->PlayAnimation(animation_name, is_loop);
	}
};

class GltfModelImpl :public Model::ModelImpl
{
private:
	std::shared_ptr<GltfModelData> data;			//glTFのデータ本体を保持
	std::shared_ptr<GltfModelRenderer> renderer;	//glTFの描画用レンダラーを保持
	std::unique_ptr<GltfModel> model;				//glTFモデル本体を保持
public:
	//コンストラクタ
	GltfModelImpl(ID3D11Device* device, const std::string& file_path)
	{
		data = GltfModelData::Load(device, file_path);			//glTFのデータをファイルから読み込んで生成
		renderer = std::make_shared<GltfModelRenderer>(device);	//描画に必要なレンダラークラスを生成
		model = std::make_unique<GltfModel>(data, renderer);	//データとレンダラーを渡してモデル本体を生成
	}

	//更新処理
	void Update(float elapsed_time)override
	{
		model->Update(elapsed_time);
	}

	//描画処理
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color)override
	{
		model->Render(context, world);
	}

	//アニメーション再生
	void PlayAnimation(const std::string& animation_nama, bool is_loop)override
	{
		model->PlayAnimation(animation_nama, is_loop);
	}
};

//===================
//コンストラクタ
//===================
Model::Model(ID3D11Device* device, const std::string& file_path)
{
	std::string extension = std::filesystem::path(file_path).extension().string();	//拡張子を抽出

	//----------------------
	//拡張子の小文字化処理
	//----------------------
	for (char& character : extension)	//抽出した拡張子の文字数分だけループ
	{
		character = std::tolower(character);	//各文字を小文字に変換して上書き
	}

	//------------------------------
	//拡張子に応じたクラスを生成
	//------------------------------
	if (extension == ".fbx")	//fbxか判定
	{
		model_impl = std::make_unique<FbxModelImpl>(device, file_path);
	}
	else if (extension == ".gltf" || extension == ".glb")	//glb、gltfか判定
	{
		model_impl = std::make_unique<GltfModelImpl>(device, file_path);
	}
	else
	{
		throw std::runtime_error("Unsupported file format");	//読み込みに対応していないため例外を投げる
	}
}

//===============
//デストラクタ
//================
Model::~Model() = default;

//==========
//更新処理
//==========
void Model::Update(float elapsed_time)
{
	if (model_impl)model_impl->Update(elapsed_time);
}

//=========
//描画処理
//=========
void Model::Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color)
{
	if (model_impl)model_impl->Render(context, world, color);
}

//=====================
//アニメーション再生
//=====================
void Model::PlayAnimation(const std::string& animation_name, bool is_loop)
{
	if (model_impl)model_impl->PlayAnimation(animation_name, is_loop);
}