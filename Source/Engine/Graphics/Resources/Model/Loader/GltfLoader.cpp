#include "GltfLoader.h"
#include "Engine\Graphics\Resources\Model\Resource\GltfModelResource.h"
#include "Engine\Graphics\Resources\Model\Resource\GltfMeshResource.h"
#include "Engine\Graphics\Resources\Model\Resource\GltfMaterialResource.h"
#include "Engine\Graphics\Resources\Model\Resource\GltfSkeletonResource.h"
#include "Engine\Graphics\Resources\Model\Resource\GltfAnimationResource.h"
#include "Engine\Core\Log\Log.h"

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf-release/tiny_gltf.h"

namespace
{
	const std::string EXTENSION_GLB = ".glb";	//バイナリglTFの拡張子
	const std::string EXTENCION_GLTH = ".gltf";	//テキストglTFの拡張子

	//ダミーコールバック
	bool NullLoadImageData(tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*)
	{
		return true;
	}
}

//読み込み
std::shared_ptr<GltfModelResource> GltfLoader::Load(const std::string& file_path)
{
	tinygltf::Model gltf_model;

	//ファイルの読み込み
	if (!LoadFile(file_path, gltf_model))
	{
		LOG_ERROR({ u8"モデル",u8"読み込み" }, u8"gltfファイルの解析に失敗");
		return nullptr;
	}

	auto model_resource = std::make_shared<GltfModelResource>();
	model_resource->SetFilePath(file_path);

	return model_resource;
}

//ファイル読み込み
bool GltfLoader::LoadFile(const std::string& file_path, tinygltf::Model& out_model)
{
	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(NullLoadImageData, nullptr);

	std::string 

	return false;
}
