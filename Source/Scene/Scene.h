#pragma once

#include "../Graphics/Graphics.h"

#include <imgui.h> 
#include <imgui_impl_dx11.h> 
#include <imgui_impl_win32.h>

class Scene
{
public:
	virtual ~Scene() = default;
	virtual void Initialize() = 0;
	virtual void Update(float elapsed_time) = 0;
	virtual void Render(float elapsed_time) = 0;
	virtual void RenderGui() = 0;
	virtual void Finalize() = 0;

protected:
	//ImGuiスケール補正
	void ImGuiScaleCorrection()
	{
		#ifdef USE_IMGUI
			ImGui_ImplDX11_NewFrame(); // DirectX11用のImGuiフレームを新規開始
			ImGui_ImplWin32_NewFrame(); // Win32用のImGuiフレームを新規開始

			//任意のウィンドウサイズに対応するマウス座標とUIスケールの補正
			ImGuiIO& io = ImGui::GetIO();
			HWND hwnd = GetForegroundWindow();
			if (hwnd)
			{
				RECT rect = {};
				GetClientRect(hwnd, &rect);
				float actual_width = static_cast<float>(rect.right - rect.left);
				float actual_height = static_cast<float>(rect.bottom - rect.top);
				if (actual_width > 0.0f && actual_height > 0.0f)
				{
					io.DisplaySize = ImVec2(logical_screen_width, logical_screen_height);
					io.MousePos.x *= (logical_screen_width / actual_width);
					io.MousePos.y *= (logical_screen_height / actual_height);
				}
			}

			ImGui::NewFrame(); // ImGuiのコアロジックのフレームを新規開始
		#endif
	}
private:
	float logical_screen_width = 1280.0f; //ゲームの設計上の基本横幅
	float logical_screen_height = 720.0f; //ゲームの設計上の基本縦幅

};

