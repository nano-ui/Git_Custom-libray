#include "FbxLoader.h"

#include "../FbxModel/FbxBone.h"
#include "../FbxModel/FbxAnimation.h"
#include "../FbxModel/FbxMaterial.h"
#include "../FbxModel/FbxSkinnedMesh.h"

#include "../FbxModel/FbxSkinnedResource.h"
#include "../Serialization/SkinnedMeshSerializer.h"
#include "../ObjectsRender/texture.h"

#include <fbxsdk.h>
#include <filesystem>

//=========================================================
// 指定したファイルからデータを読み込み、リソースに構築
//=========================================================
bool FbxLoader::Load(
	ID3D11Device* device,
	const std::string& filename,
	std::shared_ptr<FbxSkinnedResource> out_resource)
{
	if (!out_resource) return false;

	//キャッシュの確認
	std::string cache_filename = filename + ".bin";
	bool use_cache = false;

	namespace fs = std::filesystem;
	std::error_code ec;

	//キャッシュファイルが存在し、かつFBXファイルより新しいか確認
	if (fs::exists(cache_filename, ec) && fs::exists(filename, ec))
	{
		auto fbx_time = fs::last_write_time(filename, ec);
		auto bin_time = fs::last_write_time(cache_filename, ec);

		//キャッシュのほうが新しい
		if (bin_time > fbx_time)
		{
			//キャッシュから読み込み
			if (SkinnedMeshSerializer::Load(cache_filename,out_resource))
			{
				//バッファの再生成
				for (auto& mesh : out_resource->meshes)
				{
					FbxSkinnedMesh::CreateComBuffers(out_resource->GetDevice(), mesh);
				}

				//テクスチャの再ロード
				for (auto& pair : out_resource->materials)
				{
					MaterialData& mat = pair.second;
					for (int i = 0; i < 2; i++)
					{
						if (mat.texture_filenames[i].empty())continue;

						fs::path fbx_path(filename);
						fs::path tex_path = fbx_path.parent_path() / mat.texture_filenames[i];

						//テクスチャ情報を格納
						D3D11_TEXTURE2D_DESC desc{};

						load_texture_from_file(
							device,
							tex_path.wstring().c_str(),
							mat.shader_resource_views[i].GetAddressOf(),
							&desc
						);
					}
				}
				return true;
			}
		}
	}


	//------------------------------
	//FBX　SDKのマネージャー初期化
	//------------------------------
	FbxManager* manager = FbxManager::Create();
	if (!manager) return false;

	//インポートの設定の作成
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);

	//シーンの作成
	FbxScene* scene = FbxScene::Create(manager, "");

	//インポーターの作成
	FbxImporter* importer = FbxImporter::Create(manager, "");

	//---------------------
	//ファイルの読み込み
	//---------------------
	if (!importer->Initialize(filename.c_str(), -1, manager->GetIOSettings()) || !importer->Import(scene))
	{
		//読み込み失敗時の処理
		importer->Destroy();
		scene->Destroy();
		manager->Destroy();
		return false;
	}

	//インポーターは不要なので破棄
	importer->Destroy();

	//----------------------
	//座標系と形状の変換
	//----------------------
	//DirectXの座標系に変換
	//FbxAxisSystem::DirectX.ConvertScene(scene);

	//ポリゴンを三角形に変換
	FbxGeometryConverter converter(manager);
	converter.Triangulate(scene, true);

	//--------------------------
	//各パーツのデータを抽出
	//--------------------------
	 
	//マテリアルの抽出
	std::unordered_map<uint64_t, MaterialData> materials;
	FbxMaterial::Fetch(device, scene, filename, materials);

	//ボーンの抽出
	std::vector<BoneData> bones;
	FbxBone::Fetch(scene, bones);

	//メッシュの抽出
	std::vector<MeshData> meshes;
	FbxSkinnedMesh::Fetch(device, scene, bones, meshes);

	//アニメーションの抽出
	std::unordered_map<std::string, AnimationClip> animations;
	FbxAnimation::Fetch(scene, bones, animations);

	//-------------------------
	//リソースへデータを転送
	//-------------------------
	//抽出したデータを、出力先に送る
	out_resource->meshes = std::move(meshes);
	out_resource->bones = std::move(bones);
	out_resource->animations = std::move(animations);
	out_resource->materials = std::move(materials);

	//-----------------------
	//キャッシュを保存
	//-----------------------
	SkinnedMeshSerializer::Save(cache_filename, out_resource);

	//---------
	//後始末
	//---------
	scene->Destroy();
	manager->Destroy();

	return true;
}
