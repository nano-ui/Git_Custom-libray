#pragma once

#include <string>

class FileDialogHelper
{
public:
	//ファイルを開くダイアログを表示して絶対パスを返す
	static std::string OpenFileDialog(const std::string& default_dir = "Data\\Json", const std::string& filter = "JSON Files (*.json)\0*.json\0");

	//ファイルを保存ダイアログを表示して絶対パスを返す
	static std::string SaveFileDialog(const std::string& default_dir = "Data\\Json", const std::string& filter = "JSON Files (*.json)\0*.json\0", const std::string& default_ext = "json");
};

