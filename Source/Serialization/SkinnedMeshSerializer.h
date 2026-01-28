#pragma once
#include <string>

class SkinnedMeshResource;

class SkinnedMeshSerializer
{
public:
	//ファイルから読み込み
	static bool Load(const std::string& filename, SkinnedMeshResource& resource);

	//ファイルへ保存
	static bool Save(const std::string& filename, SkinnedMeshResource& resource);
};

