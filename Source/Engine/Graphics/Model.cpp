#include "Model.h"
#include <filesystem>
#include <stdexcept>

#include "GltfModel\GltfModelRenderer.h"
#include "Engine/Graphics/GltfModel/GltfModel.h"

// アニメーションが見つからない場合のエラー用定数
constexpr int ERROR_ANIMATION_NOT_FOUND = -1;


// コンストラクタ
Model::Model(ID3D11Device* device, const std::string& file_path)
{
	model_path = file_path;

	// 常にglTFモデルとしてロードを行う
	data = GltfModelData::Load(device, file_path);
	renderer = std::make_shared<GltfModelRenderer>(device);
	model = std::make_unique<GltfModel>(data, renderer);
}


// デストラクタ
Model::~Model() = default;


// 更新処理
void Model::Update(float elapsed_time)
{
	if (model)
	{
		model->Update(elapsed_time);
	}
}


// 描画処理
void Model::Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color)
{
	if (model)
	{
		model->Render(context, world);
	}
}


// アニメーション再生
void Model::PlayAnimation(const std::string& animation_name, bool is_loop)
{
	if (model)
	{
		model->PlayAnimation(animation_name, is_loop);
	}
}


// アニメーション名一覧取得
std::vector<std::string> Model::GetAnimationNames() const
{
	std::vector<std::string> names;
	if (data)
	{
		for (const auto& pair : data->animation_index_map)
		{
			names.push_back(pair.first);
		}
	}
	return names;
}


// 頂点座標リストの取得
std::vector<DirectX::XMFLOAT3> Model::GetVertices() const
{
	std::vector<DirectX::XMFLOAT3> vertices_data;
	if (data)
	{
		for (const auto& mesh : data->meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				auto it = primitive.vertex_buffer_views.find("POSITION");
				if (it != primitive.vertex_buffer_views.end())
				{
					const GltfModelData::buffer_view& view = it->second;
					if (view.buffer > -1)
					{
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


// インデックスリストの取得
std::vector<uint32_t> Model::GetIndices() const
{
	std::vector<uint32_t> indices_data;
	uint32_t vertex_offset = 0;
	if (data)
	{
		for (const auto& mesh : data->meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				const GltfModelData::buffer_view& index_view = primitive.index_buffer_view;
				if (index_view.buffer > -1)
				{
					const std::vector<unsigned char>& raw_buffer = data->raw_buffers[index_view.buffer];
					const uint8_t* data_ptr = raw_buffer.data() + index_view.byte_offset;
					size_t stride = (index_view.stride_in_bytes > 0) ? index_view.stride_in_bytes : ((index_view.format == DXGI_FORMAT_R32_UINT) ? sizeof(uint32_t) : sizeof(uint16_t));
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


// アニメーション終了判定
bool Model::IsAnimationFinished() const
{
	if (model)
	{
		return model->IsAnimationFinished();
	}
	return true;
}


// 再生時間取得
float Model::GetAnimationTime() const
{
	if (model)
	{
		return model->GetAnimationCurrentTime();
	}
	return 0.0f;
}

//アニメーション再生時間を設定
void Model::SetAnimationTime(float time)
{
	if (model)
	{
		model->SetAnimationTime(time);
	}
	else
	{
		// 意図しない挙動を防ぐためのデバッグ出力
		OutputDebugStringA("[Model Error] SetAnimationTime: Internal glTF model is null!\n");
	}
}


// アニメーション総時間取得
float Model::GetAnimationDuration() const
{
	if (model)
	{
		return model->GetAnimationDuration();
	}
	return 0.0f;
}


// アニメーション名からインデックス取得
int Model::GetAnimationIndex(const char* name) const
{
	if (!data || !name)
	{
		return ERROR_ANIMATION_NOT_FOUND;
	}
	std::string search_name(name);
	auto it = data->animation_index_map.find(search_name);
	if (it != data->animation_index_map.end())
	{
		return static_cast<int>(it->second);
	}
	return ERROR_ANIMATION_NOT_FOUND;
}


// glTFデータ構造の取得
std::shared_ptr<const GltfModelData> Model::GetGltfModelData() const
{
	return data;
}


// オリジナルノード配列取得
std::vector<GltfModelData::node>& Model::GetNodes()
{
	return data->nodes;
}


// アニメーション後のノード配列取得
const std::vector<GltfModelData::node>& Model::GetAnimatedNodes() const
{
	return model->GetAnimatedNodes();
}


// 外部からノード位置を上書き
void Model::SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation)
{
	if (model)
	{
		model->SetNodeTranslation(node_index, translation);
	}
}


// グローバル行列の再計算
void Model::RecalculateTransforms()
{
	if (model)
	{
		model->RecalculateTransforms();
	}
}