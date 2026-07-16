#include "ContentBrowserEditor.h"
#include "Engine\Graphics\Graphics.h"
#include "Engine\Graphics\GpuResourceUtils.h"
#include "Editor\EditorMediator.h"

#include <windows.h>
#include <imgui.h>

//コンストラクタ
ContentBrowserEditor::ContentBrowserEditor()
{
}

//デストラクタ
ContentBrowserEditor::~ContentBrowserEditor()
{
}

//初期化処理
void ContentBrowserEditor::Initialize()
{
	//ルートパスの初期化
	root_path = "Data";
	current_path = root_path;
	should_sync_tree = true;
	
	//ディレクトリの存在確認
	if (!std::filesystem::exists(root_path))
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: 'Data' directory not found!\n");
	}
}

//Gui描画
void ContentBrowserEditor::RenderGui()
{
	//ウィンドウサイズの設定
	const float default_window_width = 500.0f;	
	const float default_window_height = 400.0f;	
	ImGui::SetNextWindowSize(ImVec2(default_window_width, default_window_height), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(u8"コンテンツブラウザ"))
	{
		const float left_panel_width = 200.0f;
		const float panel_child_id = 1;

		//左パネル：フォルダツリーの描画
		ImGui::BeginChild(u8"フォルダ階層", ImVec2(left_panel_width, 0.0f), true);
		DrawFolderTree(root_path);
		ImGui::EndChild();
		should_sync_tree = false;

		ImGui::SameLine();

		//右パネル：ファイル一覧の描画
		ImGui::BeginChild(u8"フォルダの中身", ImVec2(0.0f, 0.0f), true);
		DrawFolderContents(current_path);
		ImGui::EndChild();
	}
	ImGui::End();
}

//フォルダ階層を表示
void ContentBrowserEditor::DrawFolderTree(const std::filesystem::path& path)
{
	try
	{
		//指定ディレクトリ内のイテレーション
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				std::string folder_name = entry.path().filename().string();	//フォルダ名
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;	//挙動制御フラグ

				//選択状態の更新
				if (current_path == entry.path())
				{
					node_flags |= ImGuiTreeNodeFlags_Selected;
				}

				//フォルダ階層の自動開閉ロジック
				bool is_ancestor = false;
				for (auto p = current_path; p != root_path.parent_path(); p = p.parent_path())
				{
					if (p == entry.path())
					{
						is_ancestor = true;
						break;
					}
				}

				if (should_sync_tree && is_ancestor)
				{
					ImGui::SetNextItemOpen(true);
				}

				//ツリーノードの描画と再帰処理
				if (ImGui::TreeNodeEx(folder_name.c_str(), node_flags))
				{
					if (ImGui::IsItemClicked())
					{
						current_path = entry.path();
						selected_path = entry.path();
					}
					DrawFolderTree(entry.path());
					ImGui::TreePop();
				}
				else
				{
					if (ImGui::IsItemClicked())
					{
						current_path = entry.path();
						selected_path = entry.path();
					}
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& error)
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: Failed to iterate directory in tree view.\n");
	}
}

//フォルダ内容を表示
void ContentBrowserEditor::DrawFolderContents(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		return;
	}

	try
	{
		//グリッドレイアウト計算
		float panel_width = ImGui::GetContentRegionAvail().x;	
		float cell_size = icon_size + grid_padding;				
		int column_count = static_cast<int>(panel_width / cell_size);	

		if (column_count < 1)
		{
			column_count = 1;
		}

		//テーブルによるアイコン描画
		if (ImGui::BeginTable("FolderContentsGrid", column_count, ImGuiTableFlags_None))
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				ImGui::TableNextColumn();

				std::string name = entry.path().filename().string();	
				bool is_selected = (selected_path == entry.path());		

				ImGui::PushID(name.c_str());

				//アイコンリソースの取得
				ID3D11ShaderResourceView* srv = GetOrCreateSystemIcon(entry.path());	
				ImVec4 bg_color = is_selected ? ImVec4(0.2f, 0.4f, 0.8f, 0.6f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

				ImGui::BeginGroup();

				//アイコンボタンの描画
				if (ImGui::ImageButton(name.c_str(), reinterpret_cast<ImTextureID>(srv), ImVec2(icon_size, icon_size), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), bg_color)) 
				{
					selected_path = entry.path();
				}

				//ダブルクリック判定（ディレクトリ遷移）
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.is_directory())
					{
						current_path = entry.path();
						selected_path = entry.path();
						should_sync_tree = true;
					}
					else if (entry.is_regular_file())
					{
						std::string extension = entry.path().extension().string();
						if (extension == ".gltf" || extension == ".glb")
						{
							std::string relative_path = entry.path().string();
							EditorMediator::Instance().OnModelDubleClied(relative_path);
						}
					}
				}

				//テキストの中央寄せ処理
				float text_offset_x = (icon_size - ImGui::CalcTextSize(name.c_str()).x) * 0.5f;
				if (text_offset_x > 0.0f)
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_offset_x);
				}
				ImGui::TextWrapped(name.c_str());

				ImGui::EndGroup();
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
	catch (const std::filesystem::filesystem_error& error)
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: Failed to iterate directory contents.\n");
	}
}

//システムアイコンを取得、新規生成
ID3D11ShaderResourceView* ContentBrowserEditor::GetOrCreateSystemIcon(const std::filesystem::path& path)
{
	//キャッシュキーの生成
	std::wstring key = path.extension().wstring();
	if (std::filesystem::is_directory(path))
	{
		key = L"[Folder]";
	}

	//キャッシュ確認
	auto it = icon_cache.find(key);
	if (it != icon_cache.end())
	{
		return it->second.Get();
	}

	//Windows APIによるシステムアイコン取得
	ID3D11Device* device = Graphics::Instance().GetDevice();
	SHFILEINFOA sfi = {};
	UINT flags = SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES;
	DWORD attributes = std::filesystem::is_directory(path) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	std::string path_str = std::filesystem::is_directory(path) ? "folder" : ("file" + path.extension().string());

	if (SHGetFileInfoA(path_str.c_str(), attributes, &sfi, sizeof(sfi), flags))
	{
		HICON h_icon = sfi.hIcon;

		if (h_icon)
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

			//SRVへの変換
			HRESULT hr = CreateSrvFromHIcon(device, h_icon, srv.GetAddressOf());
			DestroyIcon(h_icon);

			if (SUCCEEDED(hr))
			{
				icon_cache[key] = srv;
				return srv.Get();
			}
			else
			{
				OutputDebugStringA("[Error] ContentBrowserEditor: Failed to create SRV from HICON.\n");
			}
		}
	}
	else
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: Failed to get system icon file info.\n");
	}
	return nullptr;
}

//Win32のHICONからDirect3D11のシェーダーリソースビューを作成
HRESULT ContentBrowserEditor::CreateSrvFromHIcon(ID3D11Device* device, HICON h_icon, ID3D11ShaderResourceView** pp_srv)
{
	//アイコン情報の取得
	ICONINFO icon_info = {};
	if (!GetIconInfo(h_icon, &icon_info))
	{
		return E_FAIL;
	}

	BITMAP bitmap = {};
	GetObject(icon_info.hbmColor, sizeof(BITMAP), &bitmap);
	int width = bitmap.bmWidth;
	int height = bitmap.bmHeight;

	//ピクセルデータの取得
	std::vector<DWORD> pixels(width * height);
	HDC hdc = GetDC(nullptr);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; 
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	int result = GetDIBits(hdc, icon_info.hbmColor, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

	ReleaseDC(nullptr, hdc);

	//リソース解放
	if (icon_info.hbmColor)
	{
		DeleteObject(icon_info.hbmColor);
	}
	if (icon_info.hbmMask)
	{
		DeleteObject(icon_info.hbmMask);
	}

	if (result == 0)
	{
		return E_FAIL;
	}

	//アルファチャンネルの確認と補完
	bool has_alpha = false;
	for (int i = 0; i < width * height; ++i)
	{
		if ((pixels[i] & 0xFF000000) != 0)
		{
			has_alpha = true;
			break;
		}
	}

	if (!has_alpha)
	{
		for (int i = 0; i < width * height; ++i)
		{
			pixels[i] |= 0xFF000000;
		}
	}

	//D3D11テクスチャ生成
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA init_data = {};
	init_data.pSysMem = pixels.data();
	init_data.SysMemPitch = width * sizeof(DWORD);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = device->CreateTexture2D(&desc, &init_data, texture.GetAddressOf());

	if (FAILED(hr))
	{
		return hr;
	}

	//SRV生成
	hr = device->CreateShaderResourceView(texture.Get(), nullptr, pp_srv);
	return hr;
}
