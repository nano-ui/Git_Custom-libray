#pragma once

#include <string>

class PathHelper
{
public:
	//コンストラクタ
	PathHelper() = delete;

	//デストラクタ
	~PathHelper() = delete;

	//Jsonファイルパスを構築
	static std::string GenerateJsonFilePath(const std::string& folder_name, const std::string& suffix_name);

private:
	//保存先ディレクトリが存在するか確認し、無ければ自動作成
	static bool EnsureDirectoryExists(const std::string& dir_path);
};

