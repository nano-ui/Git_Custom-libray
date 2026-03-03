#pragma once

#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <tiny_gltf.h>

#include "GlthStaticModelData.h"

class GlthStaticModel
{
public:
	GlthStaticModel() = default;
	~GlthStaticModel() = default;

	// GLTF ファイル読み込み
	static std::shared_ptr<GltfModel> LoadModel(
		ID3D11Device* device,
		const std::string& filename
	);

	// メッシュ描画
	static void DrawMesh(
		ID3D11DeviceContext* context,
		const GlthMesh& mesh,
		const GlthMaterial* material = nullptr
	);

private:
	// 内部ヘルパー関数
	static bool LoadGltfFile(
		const std::string& filepath,
		tinygltf::Model& out_model);

	static void ExtractMeshData(
		ID3D11Device* device,
		const tinygltf::Model& gltf_model,
		std::shared_ptr<GltfModel> model);

	static void ExtractMaterialData(
		ID3D11Device* device,
		const tinygltf::Model& gltf_model,
		const std::string& model_path,
		std::shared_ptr<GltfModel> model);

	static Microsoft::WRL::ComPtr<ID3D11Buffer> CreateVertexBuffer(
		ID3D11Device* device,
		const void* data,
		size_t size);

	static Microsoft::WRL::ComPtr<ID3D11Buffer> CreateIndexBuffer(
		ID3D11Device* device,
		const void* data,
		size_t size);

	// 頂点データ抽出ヘルパー
	static void ExtractPositionData(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		std::vector<GlthVertex>& vertices);

	static void ExtractNormalData(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		std::vector<GlthVertex>& vertices);

	static void ExtractTexCoordData(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		std::vector<GlthVertex>& vertices);

	//テクスチャ抽出ヘルパー
	static std::string ExtractTextureFromGltf(
		const tinygltf::Model& glth_model,
		int texture_index,
		const std::string& model_path
	);
};