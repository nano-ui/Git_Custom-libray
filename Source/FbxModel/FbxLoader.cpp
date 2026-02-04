#include "FbxLoader.h"

#include "../FbxModel/FbxBone.h"
#include "../FbxModel/FbxAnimation.h"
#include "../FbxModel/FbxMaterial.h"
#include "../FbxModel/FbxSkinnedMesh.h"

#include "../FbxModel/FbxSkinnedResource.h"

#include <fbxsdk.h>

//=========================================================
// 指定したファイルからデータを読み込み、リソースに構築
//=========================================================
bool FbxLoader::Load(
	ID3D11Device* device,
	const std::string& filename,
	std::shared_ptr<FbxSkinnedResource> out_resource)
{
	if (!out_resource) return false;

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
	FbxAxisSystem::DirectX.ConvertScene(scene);

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

	//---------
	//後始末
	//---------
	scene->Destroy();
	manager->Destroy();

	return true;
}
