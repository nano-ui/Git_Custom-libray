#include "GltfDynamicMaterial.h"
#include "texture.h"

#include "tiny_gltf.h"

//=================================================================
//コンストラクタ
//=================================================================
GltfDynamicMaterial::GltfDynamicMaterial()
{
	base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

//=================================================================
//GLTFデータからマテリアルを初期化
//=================================================================
void GltfDynamicMaterial::Initialize(
	ID3D11Device* device,
	const tinygltf::Model& model,
	const tinygltf::Material& gltf_mat,
	const std::string& directory)
{
	//------------------------------------------------
	//基本色の取得
	//------------------------------------------------

	//PBRの基本色を取得
	const auto& color_factor = gltf_mat.pbrMetallicRoughness.baseColorFactor;
	//double型からXMFLOAT4型に値をコピー
	base_color = {
		static_cast<float>(color_factor[0]),	//赤成分
		static_cast<float>(color_factor[1]),	//緑成分
		static_cast<float>(color_factor[2]),	//青成分
		static_cast<float>(color_factor[3])		//アルファ成分
	};

	//------------------------------------------------
	//テクスチャの取得と読み込み
	//------------------------------------------------

	//作業用の構造体
	D3D11_TEXTURE2D_DESC texture_desc{};
	//基本テクスチャ
	int base_color_texture_index = gltf_mat.pbrMetallicRoughness.baseColorTexture.index;
	//テクスチャが指定されている場合
	if (base_color_texture_index >= 0)
	{
		//テクスチャのインデックスを取得
		int image_index = model.textures[base_color_texture_index].source;
		//テクスチャのファイルパスを取得
		std::string image_path = directory + model.images[image_index].uri;
		//ワイド文字に変換
		std::wstring w_image_path(image_path.begin(), image_path.end());
		//キャッシュ機能付きでロード
		load_texture_from_file(device, w_image_path.c_str(), base_map.GetAddressOf(), &texture_desc);
	}
	//テクスチャがない場合
	else
	{
		//白色のダミーテクスチャを作成
		make_dummy_texture(device, base_map.GetAddressOf(), 0xFFFFFFFF, 16);
	}

	//------------------------------------------------
	//法線マップ
	//------------------------------------------------

	//法線マップのインデックスを取得
	int normal_texture_index = gltf_mat.normalTexture.index;
	//指定されている場合
	if (normal_texture_index >= 0)
	{
		//法線マップのインデックスを取得
		int image_index = model.textures[normal_texture_index].source;
		//法線マップのファイルパスを取得
		std::string image_path = directory + model.images[image_index].uri;
		//ワイド文字に変換
		std::wstring w_image_path(image_path.begin(), image_path.end());
		//キャッシュ機能付きでロード
		load_texture_from_file(device, w_image_path.c_str(), normal_map.GetAddressOf(), &texture_desc);
	}
	//指定されていない場合
	else
	{
		//法線マップのダミーテクスチャを作成
		make_dummy_texture(device, normal_map.GetAddressOf(), 0xFF8080FF, 16);
	}
}

//=================================================================
//シェーダーへのバインド
//=================================================================
void GltfDynamicMaterial::Bind(ID3D11DeviceContext* dc)
{
	//シェーダーリソースをピクセルシェーダーにセット
	ID3D11ShaderResourceView* srvs[] = {
		base_map.Get(),
		normal_map.Get(),
	};

	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}
