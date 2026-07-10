#include "StateGraphConfigManager.h"
#include "../Editor/FileDialogHelper.h"
#include "../ThiedParty/json.hpp"

#include <fstream>
#include <iomanip>

//コンストラクタ
StateGraphConfigManager::StateGraphConfigManager()
{

}

//最後に使用したファイルパスを設定ファイルへ保存
void StateGraphConfigManager::SaveEditorConfig(const std::string& current_path)
{
	nlohmann::json config_json;
	config_json["LastOpenedFilePath"] = current_loaded_file_path;
	const std::string config_file_path = "Data/Json/StateEditorConfig.json";
	std::ofstream file_out(config_file_path);
	if (file_out.is_open())
	{
		const int indent_space_size = 4;
		file_out << std::setw(indent_space_size) << config_json << std::endl;
		printf("StateMachineGraphEditor: 環境設定ファイルへ最後に開いたパスを記憶しました。\n");
	}
	else
	{
		printf("Error: SaveEditorConfig - 環境設定ファイル「%s」を開けませんでした。\n", config_file_path.c_str());
	}
}

//設定ファイルから最後に使用したファイルパスを読み込む
void StateGraphConfigManager::LoadEditorConfig()
{
	const std::string config_file_path = "Data/Json/StateEditorConfig.json";
	std::ifstream file_in(config_file_path);

	if (!file_in.is_open())
	{
		printf("StateMachineGraphEditor: 環境設定ファイルがないため、初回デフォルト設定で起動します。\n");
		current_loaded_file_path = "";
		return;
	}
	nlohmann::json config_json;
	file_in >> config_json;

	if (config_json.find("LastOpenedFilePath") != config_json.end())
	{
		current_loaded_file_path = config_json["LastOpenedFilePath"].get<std::string>();
		printf("StateMachineGraphEditor: 前回の終了ファイルパス「%s」を自動検出しました。\n", current_loaded_file_path.c_str());
	}
	else
	{
		printf("Warning: LoadEditorConfig - 設定ファイルのキー構造が不正です。パスを初期化します。\n");
		current_loaded_file_path = "";
	}
}

//ファイルを開くダイアログを実行してパスを更新・取得
std::string StateGraphConfigManager::OpenGraphFileDialog()
{
	PathResult path_result = FileDialogHelper::OpenGenericFileDialog();

	//ファイルが選択されたか判定
	if (!path_result.absolute_path.empty())
	{
		current_loaded_file_path = path_result.relative_path;
	}

	return current_loaded_file_path;
}

//ファイルを保存ダイアログを実行してパスを更新・取得
std::string StateGraphConfigManager::SaveGraphFileDialog()
{
	PathResult path_result = FileDialogHelper::OpenGenericFileDialog();

	//ファイルが選択されたか判定
	if (!path_result.absolute_path.empty())
	{
		current_loaded_file_path = path_result.relative_path;
	}

	return current_loaded_file_path;
}
