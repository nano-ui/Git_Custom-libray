#include "Model.h"
#include <DirectXMath.h>

Model::Model(AssetManager* asset_manager, const wchar_t* fbx_filename)
{
	//AssetManagerにFBXファイルのロードを依頼し、結果（インデックスとタイプ）を受け取る
	MeshReference ref = asset_manager->LoadModelAsset(fbx_filename);

	is_skinned_ = ref.is_skinned;
	mesh_index_ = ref.mesh_index;
}
