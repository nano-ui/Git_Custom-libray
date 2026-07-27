#include "PathHelper.h"

#include <filesystem>
#include <cstdio>

//Jsonファイルパスを構築
std::string PathHelper::GenerateJsonFilePath(const std::string& folder_name, const std::string& suffix_name)
{
	if (folder_name.empty())
	{
		printf("Error: PathHelper::GenerateJsonFilePath - folder_name が空です。\n");
		return "";
	}

	//ベースディレクトリパスと各文字列の組み立て
	const std::string base_dir_path = "Data/Json/";
	const std::string full_dir_path = base_dir_path + folder_name + "/";
	const std::string file_name = folder_name + suffix_name + ".json";
	const std::string full_file_path = full_dir_path + file_name;

	//保存先のフォルダが存在するか確認し、存在しない場合は自動生成
	if (!EnsureDirectoryExists(full_dir_path))
	{
		printf("Error: PathHelper::GenerateJsonFilePath - ディレクトリ「%s」の確認・作成に失敗しました。\n", full_dir_path.c_str());
		return "";
	}

	return full_file_path;
}

//保存先ディレクトリが存在するか確認し、無ければ自動作成
bool PathHelper::EnsureDirectoryExists(const std::string& dir_path)
{
    try
    {
        std::filesystem::path path_obj(dir_path);

        //ディレクトリが存在しない場合のみ生成処理を実行
        if (!std::filesystem::exists(path_obj))
        {
            //ディレクトリの多階層一括作成
            if (std::filesystem::create_directories(path_obj))
            {
                printf("PathHelper: 新しいディレクトリ「%s」を作成しました。\n", dir_path.c_str());
            }
            else
            {
                printf("Error: PathHelper::EnsureDirectoryExists - ディレクトリ「%s」の作成に失敗しました。\n", dir_path.c_str());
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& exception_data)
    {
        printf("Error: PathHelper::EnsureDirectoryExists - 例外が発生しました: %s (対象パス: %s)\n",
            exception_data.what(), dir_path.c_str());
        return false;
    }
}
