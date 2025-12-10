#include "AssetManager.h"
#include "misc.h"
#include "static_mesh.h"
#include "skinned_mesh.h"
#include "framebuffer.h"
#include "fbx_utility.h"
#include <Windows.h>

//受け取ったデバイスポインタをメンバ変数に保持
AssetManager::AssetManager(ID3D11Device* device)
	:device_ptr_(device)
{
}

//全てのアセットの読み込みと作成を実行
void AssetManager::Initializa()
{
	LoadMeshes();			//メッシュの読み込みを実行
	CreateFramebuffers();	//フレームバッファの作成を実行
	LoadShaders();			//シェーダーの読み込みを実行
}

//FBXファイルを読み込み、アニメーションの有無に応じて適切なメッシュに登録し、参照情報を返す
MeshReference AssetManager::LoadModelAsset(const wchar_t* fbx_filename)
{
	bool has_animation = FbxUtility::CheckForAnimation(fbx_filename);

	MeshReference ref;

	if (has_animation)
	{
		//------------------
		//アニメーションあり
		//------------------

		const size_t MAX_PATH_LENGTH = 1024;
		char fbx_filename_narrow[MAX_PATH_LENGTH] = {};

		//WideCharToMultiByte 関数を使用して変換
		int result = WideCharToMultiByte(
			CP_ACP, 0,
			fbx_filename,
			-1,
			fbx_filename_narrow,
			MAX_PATH_LENGTH,
			NULL, NULL
		);

		ref.mesh_index = skinned_meshs_.size();//新しいskinned_meshの生成位置（vectorの末尾）をインデックスとして記録
		skinned_meshs_.push_back(std::make_unique<skinned_mesh>(device_ptr_, fbx_filename_narrow));
		ref.is_skinned = true;
	}
	else
	{
		//-------------------
		//アニメーション無し
		//-------------------

		ref.mesh_index = static_meshes_.size();//新しいstatic_meshの生成位置（vectorの末尾）をインデックスとして記録
		static_meshes_.push_back(std::make_unique<static_mesh>(device_ptr_, fbx_filename));
		ref.is_skinned = false;

	}

	return ref;	//結果をModelクラスに返す
}

//メッシュの読み込み
void AssetManager::LoadMeshes()
{
}

//フレームバッファの作成
void AssetManager::CreateFramebuffers()
{
}

//シェーダーの読み込み
void AssetManager::LoadShaders()
{
}
