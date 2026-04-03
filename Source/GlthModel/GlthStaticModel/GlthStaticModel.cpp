#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "GlthStaticModel.h"

#include <filesystem>
#include <fstream>
#include <algorithm>

// GLTF ファイル読み込み
std::shared_ptr<GltfModel> GlthStaticModel::LoadModel(
	ID3D11Device* device,
	const std::string& filename)
{
	// ファイルパスの検証
	std::filesystem::path filepath(filename);
	if (!std::filesystem::exists(filepath))
	{
		OutputDebugStringA("ERROR: GLTF file not found\n");
		return nullptr;
	}

	// GLTF ファイル読み込み
	tinygltf::Model gltf_model;
	if (!LoadGltfFile(filepath.string(), gltf_model))
	{
		OutputDebugStringA("ERROR: Failed to load GLTF file\n");
		return nullptr;
	}

	// モデルデータ作成
	auto model = std::make_shared<GltfModel>();
	DirectX::XMStoreFloat4x4(&model->transform, DirectX::XMMatrixIdentity());

	//マテリアルデータ抽出
	ExtractMaterialData(device, gltf_model, model);

	// メッシュデータ抽出
	ExtractMeshData(device, gltf_model, model);

	if (model->meshes.empty())
	{
		OutputDebugStringA("WARNING: No meshes extracted from GLTF file\n");
	}

	return model;
}

// GLTF ファイル読み込み
bool GlthStaticModel::LoadGltfFile(
	const std::string& filepath,
	tinygltf::Model& out_model)
{
	tinygltf::TinyGLTF loader;
	std::string error, warning;

	std::string extension = std::filesystem::path(filepath).extension().string();

	// 大文字小文字を統一
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return std::tolower(c); });

	bool result = false;

	if (extension == ".glb")
	{
		result = loader.LoadBinaryFromFile(&out_model, &error, &warning, filepath);
	}
	else if (extension == ".gltf")
	{
		result = loader.LoadASCIIFromFile(&out_model, &error, &warning, filepath);
	}
	else
	{
		OutputDebugStringA("ERROR: Unsupported file format. Only .glb and .gltf are supported.\n");
		return false;
	}

	if (!warning.empty())
	{
		OutputDebugStringA("WARNING: ");
		OutputDebugStringA(warning.c_str());
		OutputDebugStringA("\n");
	}

	if (!result)
	{
		OutputDebugStringA("ERROR: ");
		OutputDebugStringA(error.c_str());
		OutputDebugStringA("\n");
		return false;
	}

	return true;
}

// POSITION データ抽出
void GlthStaticModel::ExtractPositionData(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	std::vector<GlthVertex>& vertices)
{
	auto positionIt = primitive.attributes.find("POSITION");
	if (positionIt == primitive.attributes.end())
	{
		return;
	}

	const tinygltf::Accessor& posAccessor = model.accessors[positionIt->second];
	const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
	const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

	const float* posData = reinterpret_cast<const float*>(
		posBuffer.data.data() + posBufferView.byteOffset + posAccessor.byteOffset);

	for (size_t i = 0; i < posAccessor.count; i++)
	{
		vertices[i].position.x = posData[i * 3];
		vertices[i].position.y = posData[i * 3 + 1];
		vertices[i].position.z = posData[i * 3 + 2];
	}
}

// NORMAL データ抽出
void GlthStaticModel::ExtractNormalData(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	std::vector<GlthVertex>& vertices)
{
	auto normalIt = primitive.attributes.find("NORMAL");
	if (normalIt == primitive.attributes.end())
	{
		// NORMAL がない場合はデフォルト値を設定
		for (auto& vertex : vertices)
		{
			vertex.normal = { 0.0f, 1.0f, 0.0f };
		}
		return;
	}

	const tinygltf::Accessor& normalAccessor = model.accessors[normalIt->second];
	const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
	const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];

	const float* normalData = reinterpret_cast<const float*>(
		normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset);

	for (size_t i = 0; i < normalAccessor.count; i++)
	{
		vertices[i].normal.x = normalData[i * 3];
		vertices[i].normal.y = normalData[i * 3 + 1];
		vertices[i].normal.z = normalData[i * 3 + 2];
	}
}

// TEXCOORD データ抽出
void GlthStaticModel::ExtractTexCoordData(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	std::vector<GlthVertex>& vertices)
{
	auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
	if (texcoordIt == primitive.attributes.end())
	{
		// TEXCOORD がない場合はデフォルト値を設定
		for (auto& vertex : vertices)
		{
			vertex.texcoord = { 0.0f, 0.0f };
		}
		return;
	}

	const tinygltf::Accessor& texAccessor = model.accessors[texcoordIt->second];
	const tinygltf::BufferView& texBufferView = model.bufferViews[texAccessor.bufferView];
	const tinygltf::Buffer& texBuffer = model.buffers[texBufferView.buffer];

	const float* texData = reinterpret_cast<const float*>(
		texBuffer.data.data() + texBufferView.byteOffset + texAccessor.byteOffset);

	for (size_t i = 0; i < texAccessor.count; i++)
	{
		vertices[i].texcoord.x = texData[i * 2];
		vertices[i].texcoord.y = texData[i * 2 + 1];
	}
}

//タンジェントデータ抽出
void GlthStaticModel::ExtractTangentData(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	std::vector<GlthVertex>& vertices)
{
	auto tangent_it = primitive.attributes.find("TANGENT");
	if (tangent_it == primitive.attributes.end())
	{
		//TANGENT がない場合はデフォルト値を設定
		for (auto& vertex : vertices)
		{
			vertex.tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
		}
		return;
	}

	const tinygltf::Accessor& tangent_accessor = model.accessors[tangent_it->second];
	const tinygltf::BufferView& tangent_buffer_view = model.bufferViews[tangent_accessor.bufferView];
	const tinygltf::Buffer& tangent_buffer = model.buffers[tangent_buffer_view.buffer];
	const float* tangent_data = reinterpret_cast<const float*>(tangent_buffer.data.data() + tangent_buffer_view.byteOffset + tangent_accessor.byteOffset);

	for (size_t i = 0; i < tangent_accessor.count; i++)
	{
		vertices[i].tangent.x = tangent_data[i * 4];
		vertices[i].tangent.y = tangent_data[i * 4 + 1];
		vertices[i].tangent.z = tangent_data[i * 4 + 2];
		vertices[i].tangent.w = tangent_data[i * 4 + 3];
	}
}

// メッシュデータ抽出
void GlthStaticModel::ExtractMeshData(
	ID3D11Device* device,
	const tinygltf::Model& gltf_model,
	std::shared_ptr<GltfModel> model)
{
	if (gltf_model.meshes.size() == 0)
	{
		OutputDebugStringA("WARNING: No meshes found in GLTF model\n");
		return;
	}

	for (const auto& gltfMesh : gltf_model.meshes)
	{
		for (const auto& primitive : gltfMesh.primitives)
		{
			GlthMesh mesh;

			// インデックスデータ抽出
			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor& indexAccessor = gltf_model.accessors[primitive.indices];
				const tinygltf::BufferView& indexBufferView = gltf_model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = gltf_model.buffers[indexBufferView.buffer];

				const void* indexData = indexBuffer.data.data() +
					indexBufferView.byteOffset +
					indexAccessor.byteOffset;

				mesh.indices.resize(indexAccessor.count);

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					memcpy(mesh.indices.data(), indexData,
						indexAccessor.count * sizeof(uint32_t));
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					// uint16 -> uint32 に変換
					const uint16_t* indexData16 = static_cast<const uint16_t*>(indexData);
					for (size_t i = 0; i < indexAccessor.count; i++)
					{
						mesh.indices[i] = indexData16[i];
					}
				}
			}

			// 頂点データ抽出
			auto positionIt = primitive.attributes.find("POSITION");
			if (positionIt != primitive.attributes.end())
			{
				const tinygltf::Accessor& posAccessor = gltf_model.accessors[positionIt->second];
				mesh.vertices.resize(posAccessor.count);

				// 各頂点属性を抽出
				ExtractPositionData(gltf_model, primitive, mesh.vertices);
				ExtractNormalData(gltf_model, primitive, mesh.vertices);
				ExtractTexCoordData(gltf_model, primitive, mesh.vertices);
				ExtractTangentData(gltf_model, primitive, mesh.vertices);
			}

			// バッファ作成
			if (!mesh.vertices.empty())
			{
				mesh.vertex_buffer = CreateVertexBuffer(device,
					mesh.vertices.data(),
					mesh.vertices.size() * sizeof(GlthVertex));

				if (!mesh.vertex_buffer)
				{
					OutputDebugStringA("ERROR: Failed to create vertex buffer\n");
					continue;
				}
			}

			if (!mesh.indices.empty())
			{
				mesh.index_buffer = CreateIndexBuffer(device,
					mesh.indices.data(),
					mesh.indices.size() * sizeof(uint32_t));

				if (!mesh.index_buffer)
				{
					OutputDebugStringA("ERROR: Failed to create index buffer\n");
					continue;
				}
			}

			mesh.material_index = primitive.material;
			model->meshes.push_back(mesh);
		}
	}
}

//マテリアルデータ抽出
void GlthStaticModel::ExtractMaterialData(
	ID3D11Device* device,
	const tinygltf::Model& gltf_model,
	std::shared_ptr<GltfModel> model)
{
	for (const auto& gltf_mat : gltf_model.materials)
	{
		auto material = std::make_shared<GltfStaticMaterial>();
		material->SetMaterialName(gltf_mat.name);

		//シェーダーの初期化
		material->InitializeShader(device, L"gltf_static_mesh_vsr.cso", L"gltf_static_mesh_ps.cso");

		//パラメータ設定
		GltfStaticMaterialData material_data = {};

		//ベースカラー
		auto& pbr = gltf_mat.pbrMetallicRoughness;
		material_data.color = DirectX::XMFLOAT4(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2]),
			static_cast<float>(pbr.baseColorFactor[3])
		);

		//メタリック・ラフネス
		material_data.metallic = static_cast<float>(pbr.metallicFactor);
		material_data.roughness = static_cast<float>(pbr.roughnessFactor);

		//放射色
		material_data.emissive = DirectX::XMFLOAT3(
			static_cast<float>(gltf_mat.emissiveFactor[0]),
			static_cast<float>(gltf_mat.emissiveFactor[1]),
			static_cast<float>(gltf_mat.emissiveFactor[2])
		);
		material_data.normal_scale = static_cast<float>(gltf_mat.normalTexture.scale);
		material_data.occlusion_strength = static_cast<float>(gltf_mat.occlusionTexture.strength);

		material->SetMaterialData(material_data);

		//テクスチャの読み込み
		GltfStaticMaterialTextures mat_tex;

		//ベースカラーテクスチャ
		if (pbr.baseColorTexture.index >= 0)
		{
			int img_idx = gltf_model.textures[pbr.baseColorTexture.index].source;
			mat_tex.color_texture = CreateShaderResourceViewFromGltfImage(device, gltf_model.images[img_idx]);
		}

		//法線テクスチャ
		if (gltf_mat.normalTexture.index >= 0) {
			int img_idx = gltf_model.textures[gltf_mat.normalTexture.index].source;
			mat_tex.normal_texture = CreateShaderResourceViewFromGltfImage(device, gltf_model.images[img_idx]);
		}

		material->SetTextures(mat_tex);
		model->materials.push_back(material);
	}
}

//GLTFのImageデータからSRVを作成
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GlthStaticModel::CreateShaderResourceViewFromGltfImage(
	ID3D11Device* device, 
	const tinygltf::Image& gltf_image)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = gltf_image.width;
	desc.Height = gltf_image.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 基本的にRGBA
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = gltf_image.image.data();
	init_data.SysMemPitch = gltf_image.width * 4;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	device->CreateTexture2D(&desc, &init_data, texture.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf());

	return srv;
}

// メッシュ描画
void GlthStaticModel::DrawMesh(
	ID3D11DeviceContext* context,
	const GlthMesh& mesh,
	const std::vector<std::shared_ptr<GltfStaticMaterial>>& materials,
	Microsoft::WRL::ComPtr<ID3D11Buffer> material_cb)
{
	if (!context || !mesh.vertex_buffer || !mesh.index_buffer)
	{
		return;
	}

	//マテリアルの適用
	if (mesh.material_index >= 0 && mesh.material_index < static_cast<int>(materials.size()))
	{
		//シェーダー、定数バッファ(b0)、テクスチャを一括適用 [cite: 3, 8]
		materials[mesh.material_index]->ApplyToShader(context, material_cb, 0);
	}

	// 頂点バッファ設定
	UINT stride = sizeof(GlthVertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, mesh.vertex_buffer.GetAddressOf(), &stride, &offset);

	// インデックスバッファ設定
	context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// プリミティブトポロジー設定
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 描画
	context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
}

// 頂点バッファ作成
Microsoft::WRL::ComPtr<ID3D11Buffer> GlthStaticModel::CreateVertexBuffer(
	ID3D11Device* device,
	const void* data,
	size_t size)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(size);
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource = {};
	subresource.pSysMem = data;

	Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
	HRESULT hr = device->CreateBuffer(&desc, &subresource, buffer.GetAddressOf());

	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to create vertex buffer\n");
		return nullptr;
	}

	return buffer;
}

// インデックスバッファ作成
Microsoft::WRL::ComPtr<ID3D11Buffer> GlthStaticModel::CreateIndexBuffer(
	ID3D11Device* device,
	const void* data,
	size_t size)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(size);
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource = {};
	subresource.pSysMem = data;

	Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
	HRESULT hr = device->CreateBuffer(&desc, &subresource, buffer.GetAddressOf());

	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to create index buffer\n");
		return nullptr;
	}

	return buffer;
}