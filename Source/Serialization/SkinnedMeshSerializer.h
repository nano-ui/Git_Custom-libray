#pragma once
#include <string>
#include <memory>

class FbxSkinnedResource;

class SkinnedMeshSerializer
{
public:
	//ファイルから読み込み
	static bool Load(const std::string& filename, std::shared_ptr<FbxSkinnedResource> resource);

	//ファイルへ保存
	static bool Save(const std::string& filename, std::shared_ptr<FbxSkinnedResource>& resource);
};

