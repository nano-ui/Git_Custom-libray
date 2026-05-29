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

	//頂点座標リストの取得
	virtual std::vector<DirectX::XMFLOAT3> GetVertices()const = 0;

	//インデックスリストの取得
	virtual std::vector<uint32_t> GetIndices()const = 0;

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

	//頂点座標リストの取得
	std::vector<DirectX::XMFLOAT3> GetVertices()const override
	{
		std::vector<DirectX::XMFLOAT3> vertices_data;

		//全メッシュの頂点座標を結合
		if (resource)
		{
			for (const auto& mesh : resource->GetMeshes())
			{
				for (const auto& vertex : mesh.vertices)
				{
					vertices_data.push_back(vertex.position);
				}
			}
		}
		return vertices_data;
	}

	//インデックスリストの取得
	std::vector<uint32_t> GetIndices()const override
	{
		std::vector<uint32_t> indeices_data;
		uint32_t vertex_offset = 0;

		//全メッシュのインデックスを結合し、オフセットを適用
		if (resource)
		{
			for (const auto& mesh : resource->GetMeshes())
			{
				for (uint32_t index : mesh.indices)
				{
					indeices_data.push_back(index + vertex_offset);
					vertex_offset += static_cast<uint32_t>(mesh.vertices.size());
				}
			}
		}
		return indeices_data;
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

	//頂点座標リストの取得
	std::vector<DirectX::XMFLOAT3> GetVertices() const override
	{
		std::vector<DirectX::XMFLOAT3> vertices_data;

		//全メッシュの頂点座標を結合
		if (data)
		{
			for (const auto& mesh : data->meshes)
			{
				for (const auto& primitive : mesh.primitives)
				{
					//"POSITION" のマップ要素を検索し、独自のbuffer_view構造体を取得
					auto it = primitive.vertex_buffer_views.find("POSITION");
					if (it != primitive.vertex_buffer_views.end())
					{
						const GltfModelData::buffer_view& view = it->second;
						if (view.buffer > -1)
						{
							//raw_buffersから生データを取得し、指定された間隔でXMFLOAT3を抽出
							const std::vector<unsigned char>& raw_buffer = data->raw_buffers[view.buffer];
							const uint8_t* data_ptr = raw_buffer.data() + view.byte_offset;
							size_t stride = (view.stride_in_bytes > 0) ? view.stride_in_bytes : sizeof(DirectX::XMFLOAT3);
							for (size_t i = 0; i < view.count; i++)
							{
								const DirectX::XMFLOAT3* pos = reinterpret_cast<const DirectX::XMFLOAT3*>(data_ptr + (i * stride));
								vertices_data.push_back(*pos);
							}

						}
					}
				}
			}
		}
		return vertices_data;
	}

	//インデックスリストの取得
	std::vector<uint32_t> GetIndices()const override
	{
		std::vector<uint32_t> indices_data;
		uint32_t vertex_offset = 0;

		if (data)
		{
			for (const auto& mesh : data->meshes)
			{
				for (const auto& primitive : mesh.primitives)
				{
					//プリミティブが保持しているインデックス用のbuffer_viewを取得
					const GltfModelData::buffer_view& index_view = primitive.index_buffer_view;
					if (index_view.buffer > -1)
					{
						const std::vector<unsigned char>& raw_buffer = data->raw_buffers[index_view.buffer];
						const uint8_t* data_ptr = raw_buffer.data() + index_view.byte_offset;
						size_t stride = (index_view.stride_in_bytes > 0) ? index_view.stride_in_bytes : ((index_view.format == DXGI_FORMAT_R32_UINT) ? sizeof(uint32_t) : sizeof(uint16_t));

						//ビット数に応じて生データを整数として抽出
						for (size_t i = 0; i < index_view.count; i++)
						{
							if (stride == sizeof(uint32_t))
							{
								uint32_t index_value = *reinterpret_cast<const uint32_t*>(data_ptr + (i * stride));
								indices_data.push_back(index_value + vertex_offset);
							}
							else
							{
								uint16_t index_value = *reinterpret_cast<const uint16_t*>(data_ptr + (i * stride));
								indices_data.push_back(static_cast<uint32_t>(index_value) + vertex_offset);
							}
						}
					}
					//次のパーツのために頂点オフセットを進める
					auto it = primitive.vertex_buffer_views.find("POSITION");
					if (it != primitive.vertex_buffer_views.end())
					{
						vertex_offset += static_cast<uint32_t>(it->second.count);
					}
				}
			}
		}
		return indices_data;
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

//頂点座標リストの取得
std::vector<DirectX::XMFLOAT3> Model::GetVertices()const
{
	std::vector<DirectX::XMFLOAT3> vertices;
	if (model_impl)
	{
		vertices = model_impl->GetVertices();
	}
	return vertices;
}

//インデックスリストの取得
std::vector<uint32_t> Model::GetIndices()const
{
	std::vector<uint32_t> indices;
	if (model_impl)
	{
		indices = model_impl->GetIndices();
	}
	return indices;
}
