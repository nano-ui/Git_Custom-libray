#pragma once

#include <string>

//ファイルのパス情報
struct PathResult
{
	std::string absolute_path = "";	//絶対パス
	std::string relative_path = "";	//相対パス
	std::string extension = "";		//拡張子
};

class FileDialogHelper
{
public:

	//各種パス情報を返す
	static PathResult OpenGenericFileDialog();

	//ファイルを開くダイアログを表示して絶対パスを返す
	static std::string OpenFileDialog(const std::string& default_dir = "Data\\Json", const std::string& filter = "JSON Files (*.json)\0*.json\0");

	//ファイルを保存ダイアログを表示して絶対パスを返す
	static std::string SaveFileDialog(const std::string& default_dir = "Data\\Json", const std::string& filter = "JSON Files (*.json)\0*.json\0", const std::string& default_ext = "json");
};

