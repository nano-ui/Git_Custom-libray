#include "FbxSkinnedResource.h"
#include "../FbxModel/FbxLoader.h"

//===================
//コンストラクタ
//===================
FbxSkinnedResource::FbxSkinnedResource(ID3D11Device* device)
	:device(device)
{
}

//=========================
//FBXファイルの読み込み
//=========================
bool FbxSkinnedResource::Load(const std::string& fbx_filename)
{
	return FbxLoader::Load(device.Get(), fbx_filename, shared_from_this());
}
