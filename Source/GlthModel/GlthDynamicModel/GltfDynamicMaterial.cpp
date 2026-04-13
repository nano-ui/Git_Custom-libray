#include "GltfDynamicMaterial.h"
#include "texture.h"

#include "tiny_gltf.h"

#include <filesystem>

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

	auto load_tex = [&](int tex_index, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& target_srv, uint32_t dummy_color)
	{
		//指定がある場合
		if (tex_index >= 0)
		{
			int image_index = model.textures[tex_index].source;	//画像番号を取得
			const auto& image = model.images[image_index];	//画像データ参照

			//外部ファイル形式の場合
			if (!image.uri.empty())
			{
				std::filesystem::path full_path = std::filesystem::path(directory) / image.uri;
				D3D11_TEXTURE2D_DESC desc = {};	//ダミー用
				load_texture_from_file(device, full_path.wstring().c_str(), target_srv.GetAddressOf(), &desc);
			}
			//埋め込み形式の場合
			else if (!image.image.empty())
			{
				std::wstring cache_name = L"Embedded_" + std::to_wstring(image_index);
				LoadTextureFromMemory(device, image.image.data(), image.image.size(), target_srv.GetAddressOf(), cache_name);
			}
			else
			{
				//指定がない場合
				make_dummy_texture(device, target_srv.GetAddressOf(), dummy_color, 16);
			}
		}
	};

	//各マップに対して実行
	load_tex(gltf_mat.pbrMetallicRoughness.baseColorTexture.index, base_map, 0xFFFFFFFF);
	load_tex(gltf_mat.normalTexture.index, normal_map, 0xFF8080FF);
}

//====================
//シェーダーを設定
//====================
void GltfDynamicMaterial::SetShader(ID3D11VertexShader* vs, ID3D11PixelShader* ps, ID3D11InputLayout* layout)
{
	//------------------------------
	//シェーダーオブジェクトの保持
	//------------------------------
	vertex_shader = vs;
	pixel_shader = ps;
	input_layout = layout;
}

//=================================================================
//シェーダーへのバインド
//=================================================================
void GltfDynamicMaterial::Bind(ID3D11DeviceContext* dc)
{
	//------------------------
	//シェーダー自体のセット
	//------------------------
	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	//シェーダーリソースをピクセルシェーダーにセット
	ID3D11ShaderResourceView* srvs[] = {
		base_map.Get(),
		normal_map.Get(),
	};

	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}
