#pragma once

#include <memory>
#include <filesystem>
#include <wrl.h>
#include <d3d11.h>
#include <unordered_map>
#include <string>

#ifndef HWND_DEFINED
struct HICON__;
typedef struct HICON__* HICON;
#endif

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

	//システムアイコンを取得、新規生成
	ID3D11ShaderResourceView* GetOrCreateSystemIcon(const std::filesystem::path& path);

	//Win32のHICONからDirect3D11のシェーダーリソースビューを作成
	HRESULT CreateSrvFromHIcon(ID3D11Device* device, HICON h_icon, ID3D11ShaderResourceView** pp_srv);

private:
	std::filesystem::path root_path;	//ルートディレクトリパス
	std::filesystem::path current_path;	//現在のディレクトリパス
	std::filesystem::path selected_path;//選択中のパス
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> icon_cache;	//アイコンのキャッシュ
	static constexpr float icon_size = 64.0f;		//アイコンの大きさ
	static constexpr float grid_padding = 16.0f;	//周囲の余白
	bool should_sync_tree = false;					//フォルダ遷移時のツリー同期フラグ
};

