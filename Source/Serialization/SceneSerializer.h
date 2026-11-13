#pragma once

#include <string>

class JsonSeializer;

class SceneSerializer
{
public:
	//シーン保存
	bool SaveScene(const std::string& file_path);

	//シーン読み込み
	bool LoadScene(const std::string& file_path);

	//設定ファイルへ保存
	bool SaveEditorConfig(const std::string& last_scene_path);

	//設定ファイルを取得
	std::string LoadEditorConfig();

private:
	static const std::string editor_config_path;	//設定ファイルパス
	static const std::string config_key_scene_path;	//前回開いたファイルパス
	static const std::string json_root_key_objects;	//全オブジェクトを保持するルートキー名
	static const std::string detail_json_directory;	//個別フォルダ群の各階層パス
};

