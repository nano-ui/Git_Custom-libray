#pragma once

#include <memory>
#include <filesystem>

class ContentBrowserEditor
{
public:
	//コンストラクタ
	ContentBrowserEditor();

	//デストラクタ
	~ContentBrowserEditor();

	//初期化処理
	void Initialize();

	//Gui描画
	void RenderGui();

private:
	//フォルダ階層を表示
	void DrawFolderTree(const std::filesystem::path& path);

	//フォルダ内容を表示
	void DrawFolderContents(const std::filesystem::path& path);

private:
	std::filesystem::path root_path;	//ルートディレクトリパス
	std::filesystem::path current_path;	//現在のディレクトリパス
	std::filesystem::path selected_path;//選択中のパス
};

