#include "framework.h"
#include "../Graphics/Graphics.h"
#include "../Scene/SceneTitle.h"
#include "../Scene/SceneManager.h"
#include "../Input/Input.h"

#include <sstream>
#include <memory>

namespace
{
	struct GraphicsScopeDeleter
	{
		void operator()(Graphics* graphics) const noexcept
		{
			if (graphics)
			{
				graphics->Finalize();
			}
		}
	};

	struct SceneManagerScopeDeleter
	{
		void operator()(SceneManager* scene_manager) const noexcept
		{
			if (scene_manager)
			{
				scene_manager->Finalize();
			}
		}
	};
}

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	struct ImGuiScopeDeleter
	{
		void operator()(ImGuiContext* context) const noexcept
		{
			if (context)
			{
				ImGui_ImplDX11_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(context);
			}
		}
	};
}
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
	std::unique_ptr<Graphics, GraphicsScopeDeleter> graphics_scope(&Graphics::Instance());
	std::unique_ptr<SceneManager, SceneManagerScopeDeleter> scene_manager_scope(&SceneManager::Instance());

	//グラフィックの初期化
	if (!graphics_scope->Initialize(hwnd))
	{
		return 0;
	}

#ifdef USE_IMGUI
	std::unique_ptr<ImGuiContext, ImGuiScopeDeleter> imgui_scope;
	//ImGuiの初期化
	IMGUI_CHECKVERSION();
	imgui_scope.reset(ImGui::CreateContext());
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	if (!ImGui_ImplWin32_Init(hwnd))
	{
		return 0;
	}
	if (!ImGui_ImplDX11_Init(graphics_scope->GetDevice(), graphics_scope->GetContext()))
	{
		return 0;
	}
	ImGui::StyleColorsDark();
#endif
	//初期シーンの登録
	std::unique_ptr<SceneTitle> title_scene = std::make_unique<SceneTitle>();
	scene_manager_scope->ChangeScene(std::move(title_scene));
	Input::Instance().Initialize();

	//メインループ
	while (WM_QUIT != msg.message)
	{
		//現在のフレームに溜まっているWindowsメッセージを全て消化
		while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WM_QUIT == msg.message)
			{
				break;
			}

		}
		if (WM_QUIT != msg.message)
		{
#ifdef USE_IMGUI
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
#endif // USE_IMGUI

			tictoc.tick();
			calculate_frame_stats();
			Input::Instance().Update();

			//シーンの更新と描画
			scene_manager_scope->Update(tictoc.time_interval());
			graphics_scope->BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);
			scene_manager_scope->Render(tictoc.time_interval());
#ifdef USE_IMGUI
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			//if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			//{
			//	ImGui::UpdatePlatformWindows();
			//	ImGui::RenderPlatformWindowsDefault();
			//}
#endif // USE_IMGUI
			graphics_scope->EndFrame();
		}
	}
	scene_manager_scope.reset();

	return static_cast<int>(msg.wParam);
}

LRESULT framework::handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef  USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return 1; 
#endif //  USE_IMGUI

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		if (wparam == VK_F11)
		{
			toggle_fullscreen(); // フルスクリーン切り替え関数を呼び出し
			return 0;
		}
		break;
	case WM_SIZE:
		//ウィンドウサイズ変更検知
		if (wparam != SIZE_MINIMIZED)
		{
			UINT width = LOWORD(lparam);
			UINT height = HIWORD(lparam);
			Graphics::Instance().Resize(width, height);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void framework::calculate_frame_stats()
{
	if (++frames_per_second, (tictoc.time_stamp() - count_by_seconds) >= 1.0f)
	{
		//FPSとフレーム時間を計算
		float fps = static_cast<float>(frames_per_second);
		float mspf = 1000.0f / fps;

		//表示する文字列を作成
		const size_t buffer_size = 256;
		wchar_t buffer[buffer_size] = {};
		swprintf_s(buffer, buffer_size, L"Game Framework : FPS : %.6f / Frame Time : %6f(ms)", fps, mspf);

		//ウィンドウのタイトルを変更
		SetWindowText(hwnd, buffer);

		//カウンタをリセット
		frames_per_second = 0;
		count_by_seconds += 1.0f;
	}
}

//フルスクリーンとウィンドウモードの切り替え
void framework::toggle_fullscreen()
{
	static bool is_fullscreen = false;
	static RECT window_rect = {};
	static DWORD window_style = 0;

	//現在のモード判定とウィンドウスタイルの変更処理
	if (!is_fullscreen)
	{
		GetWindowRect(hwnd, &window_rect);
		window_style = GetWindowLong(hwnd, GWL_STYLE);
		SetWindowLong(hwnd, GWL_STYLE, (window_style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME)) | WS_POPUP);

		//モニター情報の取得とウィンドウの全画面拡大
		MONITORINFO monitor_info = {};
		monitor_info.cbSize = sizeof(monitor_info);
		GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor_info);

		SetWindowPos(hwnd, HWND_TOP,
			monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOZORDER);

		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}
	else
	{
		//ウィンドウモードへの復帰処理
		SetWindowLong(hwnd, GWL_STYLE, window_style);

		SetWindowPos(hwnd, HWND_TOP,
			window_rect.left,
			window_rect.top,
			window_rect.right - window_rect.left,
			window_rect.bottom - window_rect.top,
			SWP_FRAMECHANGED | SWP_NOZORDER);

		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}
	is_fullscreen = !is_fullscreen;
}
