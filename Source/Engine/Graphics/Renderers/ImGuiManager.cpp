#include "ImGuiManager.h"
#include <iostream>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

//ImGuiのWin32メッセージハンドラ宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//コンストラクタ
ImGuiManager::ImGuiManager()
	:is_initialized(false)
{
}

//デストラクタ
ImGuiManager::~ImGuiManager()
{
	Shutdown();
}

//初期化処理
bool ImGuiManager::Initialize(HWND hwnd_window, ID3D11Device* device_ptr, ID3D11DeviceContext* context_ptr)
{
	if (is_initialized)
	{
		std::cout << "[ImGuiManager] 警告: すでに初期化されています。" << std::endl;
		return true;
	}

	if (!hwnd_window || !device_ptr || !context_ptr)
	{
		std::cout << "[ImGuiManager] エラー: ウィンドウハンドルまたはDirectX11デバイスのポインタが不正です。" << std::endl;
		return false;
	}

	//ImGuiコンテキスト生成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	//スタイル設定呼び出し
	SetupStyle();

	//Win32バックエンド初期化
	if (!ImGui_ImplWin32_Init(hwnd_window))
	{
		std::cout << "[ImGuiManager] エラー: Win32用ImGuiバックエンドの初期化に失敗しました。" << std::endl;
		ImGui::DestroyContext();
		return false;
	}

	//DirectX11バックエンド初期化
	if (!ImGui_ImplDX11_Init(device_ptr, context_ptr))
	{
		std::cout << "[ImGuiManager] エラー: DirectX11用ImGuiバックエンドの初期化に失敗しました。" << std::endl;
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	is_initialized = true;
	std::cout << "[ImGuiManager] ImGuiの初期化が完了しました。" << std::endl;
	return true;
}

//フレーム開始処理
void ImGuiManager::BeginFrame()
{
	if (!is_initialized)
	{
		std::cout << "[ImGuiManager] エラー: 未初期化状態で BeginFrame が呼び出されました。" << std::endl;
		return;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

//フレーム描画処理
void ImGuiManager::EndFrame()
{
	if (!is_initialized)
	{
		std::cout << "[ImGuiManager] エラー: 未初期化状態で EndFrame が呼び出されました。" << std::endl;
		return;
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//解放処理
void ImGuiManager::Shutdown()
{
	if (!is_initialized)return;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	is_initialized = false;
	std::cout << "[ImGuiManager] ImGuiの解放処理が完了しました。" << std::endl;
}

//Win32ウィンドウメッセージ処理
LRESULT ImGuiManager::ProcessMessage(HWND hwnd_window, UINT message_type, WPARAM w_param, LPARAM l_param)
{
	if (is_initialized)return ImGui_ImplWin32_WndProcHandler(hwnd_window, message_type, w_param, l_param);
	return 0;
}

//UIの初期スタイル設定
void ImGuiManager::SetupStyle()
{
	ImGuiIO& io = ImGui::GetIO();

	//ドッキング有効化
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	//日本語フォントの読み込み
	const ImWchar* glyph_ranges = io.Fonts->GetGlyphRangesJapanese();
	const float font_size = 18.0f; //フォントの大きさ
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", font_size, nullptr, glyph_ranges);

	ImGui::StyleColorsDark();
}