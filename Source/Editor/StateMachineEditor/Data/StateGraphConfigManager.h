#pragma once

#include <string>

class StateGraphConfigManager
{
public:
	//コンストラクタ
	StateGraphConfigManager();

	//デストラクタ
	~StateGraphConfigManager() = default;

	//最後に使用したファイルパスを設定ファイルへ保存
	void SaveEditorConfig(const std::string& current_path);

	//設定ファイルから最後に使用したファイルパスを読み込む
	void LoadEditorConfig();

	//ファイルを開くダイアログを実行してパスを更新・取得
	std::string OpenGraphFileDialog();

	//ファイルを保存ダイアログを実行してパスを更新・取得
	std::string SaveGraphFileDialog();

	//ファイルパスを取得
	const std::string& GetCurrentLoadedFilePath()const { return current_loaded_file_path; }

	//ファイルパスを設定
	void SetCurrentLoadedFilePath(const std::string& path) { current_loaded_file_path = path; }

private:
	std::string current_loaded_file_path = "";
};

