#include "framework.h"
#include "../Graphics/Graphics.h"
#include "../Scene/SceneTitle.h"

#include <sstream>

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

framework::framework(HWND hwnd):hwnd(hwnd)
{
}

framework::~framework()
{
}

int framework::run()
{
	MSG msg{};

	//グラフィックの初期化
	if (!Graphics::Instance().Initialize(hwnd))
	{
		return 0;
	}

#ifdef USE_IMGUI
	//ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(Graphics::Instance().GetDevice(), Graphics::Instance().GetContext());
	ImGui::StyleColorsDark();
#endif
	//MainSceneを作成・初期化
	scene = std::make_unique<SceneTitle>();
	scene->Initialize();

	//メインループ
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			tictoc.tick();
			calculate_frame_stats();

			//シーンの更新と描画
			if (scene)
			{
				scene->Update(tictoc.time_interval());
				scene->Render(tictoc.time_interval());
			}
		}
	}

#ifdef USE_IMGUI
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif

	//シーンの終了処理
	if (scene)
	{
		scene->Finalize();
	}

	Graphics::Instance().Finalize();
	return static_cast<int>(msg.wParam);
}

LRESULT framework::handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef  USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true; 
#endif //  USE_IMGUI

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, msg, wparam, lparam);
		}
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return LRESULT();
}

void framework::calculate_frame_stats()
{
	if (++frames_per_second, (tictoc.time_stamp() - count_by_seconds) >= 1.0f)
	{
		//FPSとフレーム時間を計算
		float fps = static_cast<float>(frames_per_second);
		float mspf = 1000.0f / fps;

		//表示する文字列を作成
		std::wostringstream outs;
		outs.precision(6);
		outs << L"Game Framework" << L" : FPS : " << fps << L" / " << L"Frame Time : " << mspf << L" (ms)";

		//ウィンドウのタイトルを変更
		SetWindowText(hwnd, outs.str().c_str());

		//カウンタをリセット
		frames_per_second = 0;
		count_by_seconds += 1.0f;
	}
}
