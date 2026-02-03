#include "FbxMaterial.h"

#include <fbxsdk.h>
#include <filesystem>

#include "../Common/ModelData.h"
#include "../ObjectsRender/texture.h"

//====================================
//マテリアル情報とテクスチャ読み込み
//=====================================
void FbxMaterial::Fetch(
	ID3D11Device* device,
	fbxsdk::FbxScene* scene,
	const std::string& fbx_filename,
	std::unordered_map<uint64_t, MaterialData>& out_materials)
{
	out_materials.clear();
	int material_count = scene->GetMaterialCount();
	for (int i = 0; i < material_count; i++)
	{
		FbxSurfaceMaterial* fbx_material = scene->GetMaterial(i);
		MaterialData material_data;
		material_data.name = fbx_material->GetName();
		material_data.unique_id = fbx_material->GetUniqueID();

		//----------------------------------------------------------------
		//拡散反射光（Diffuse）プロパティの取得
		//LambertやPhongなどのシェーディングモデルに関わらず色情報を取得
		//----------------------------------------------------------------
		FbxProperty prop = fbx_material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (prop.IsValid())
		{
			FbxDouble3 color = prop.Get<FbxDouble3>();
			material_data.color = DirectX::XMFLOAT4(
				static_cast<float>(color[0]),
				static_cast<float>(color[1]),
				static_cast<float>(color[2]),
				1.0f
			);

			//-----------------
			//テクスチャの取得
			//-----------------
			int texture_count = prop.GetSrcObjectCount<FbxFileTexture>();
			if (texture_count > 0)
			{
				FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>(0);
				if (texture)
				{
					//---------------------------
					//テクスチャファイル名を保存
					//---------------------------
					material_data.texture_filenames[0] = texture->GetRelativeFileName();
				}
			}
		}

		//-----------------
		//法線マップの取得
		//-----------------
		prop = fbx_material->FindProperty(FbxSurfaceMaterial::sNormalMap);
		if (prop.IsValid())
		{
			int texture_count = prop.GetSrcObjectCount<FbxFileTexture>();
			if (texture_count > 0)
			{
				FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>();
				if (texture)
				{
					material_data.texture_filenames[1] = texture->GetRelativeFileName();
				}
			}
		}

		//---------------------
		//テクスチャの読み込み
		//---------------------
		for (size_t tex_idx = 0; tex_idx < 2; tex_idx++)
		{
			//ファイル名が取得できている場合
			if (material_data.texture_filenames[tex_idx].length() > 0)
			{
				//FBXファイルのパスを基準に、テクスチャの相対パスを解決
				std::filesystem::path fbx_path(fbx_filename);
				std::filesystem::path tex_path = fbx_path.parent_path() / material_data.texture_filenames[tex_idx];

				//テクスチャをファイルから読み込んでSRVを作成
				D3D11_TEXTURE2D_DESC texture2d_desc{};
				load_texture_from_file(
					device,
					tex_path.c_str(),
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					&texture2d_desc
				);
			}
			else
			{
				//テクスチャがない場合はダミーテクスチャを作成
				uint32_t dummy_color = (tex_idx == 1) ? 0xFFFF7F7F : 0xFFFFFFFF;
				make_dummy_texture(
					device,
					material_data.shader_resource_views[tex_idx].GetAddressOf(),
					dummy_color,
					16
				);
			}
		}


		//-------------------------
		//解析したマテリアルを登録
		//-------------------------
		out_materials[material_data.unique_id] = std::move(material_data);
	}
}
