#pragma once

#include <string>
#include <memory>

class GltfModelData;

class GltfModelSerializer
{
public:
	//バイナリファイルからデータを読み込み
	static bool Load(const std::string& filename, const std::shared_ptr<GltfModelData>& resource);

	//データをバイナリファイルへ保存
	static bool Save(const std::string& filename, const std::shared_ptr<GltfModelData>& resource);
};

