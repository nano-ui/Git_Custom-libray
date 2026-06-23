#pragma once

#include <string>
#include <vector>
#include <fbxsdk.h>

namespace FbxUtility
{
	//指定されたFBXファイルにアニメーションデータが存在するかをチェック
	inline bool CheckForAnimation(const wchar_t* fbx_filename)
	{
		//-----------------
		//FbxManagerの作成
		//-----------------
		FbxManager* fbx_manager = FbxManager::Create();
		if (!fbx_manager) return false;

		//------------------
		//FbxImporterの作成
		//------------------
		FbxImporter* fbx_importer = FbxImporter::Create(fbx_manager, "");
		if (!fbx_importer)
		{
			fbx_manager->Destroy();
			return false;
		}

		//--------------------------------------------------------------------
		//ファイルパスのワイド文字(wchar_t)からマルチバイト文字(char)への変換
		//--------------------------------------------------------------------
		size_t len = wcslen(fbx_filename) + 1;
		std::vector<char> filename_char(len);
		wcstombs_s(nullptr, filename_char.data(), len, fbx_filename, _TRUNCATE);

		//---------------------------
		//ファイルの初期化とチェック
		//---------------------------
		if (!fbx_importer->Initialize(filename_char.data(), -1, fbx_manager->GetIOSettings()))
		{
			fbx_importer->Destroy();
			fbx_manager->Destroy();
			return false;
		}

		//--------------------------
		//FbxSceneの作成とインポート
		//--------------------------
		FbxScene* fbx_scene = FbxScene::Create(fbx_manager, "TempScene");
		fbx_importer->Import(fbx_scene);

		//-------------------------------------
		//アニメーションスタックの数をチェック
		//-------------------------------------
		int anim_stack_count = fbx_scene->GetSrcObjectCount<FbxAnimStack>();

		//---------------
		//リソースの開放
		//---------------
		fbx_importer->Destroy();
		fbx_manager->Destroy();

		//結果を返す
		return anim_stack_count > 0;
	}
}