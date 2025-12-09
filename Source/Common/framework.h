#pragma once
#include <d3d11.h>

#include <windows.h>
#include <tchar.h>
#include <sstream>
#include <wrl.h>

#include "geometric_primitive.h"
#include "misc.h"
#include "high_resolution_timer.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];
#endif
#include "sprite.h"
#include "sprite_batch.h"
#include "static_mesh.h"
#include "skinned_mesh.h"
#include "framebuffer.h"
#include "fullscreen_quad.h"
#include "shader.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FULLSCREEN FALSE
#define APPLICATION_NAME L"X3DGP"

//Unit32の５の②の※から


class framework
{
public:


	CONST HWND hwnd;


	framework(HWND hwnd);
	~framework();

	framework(const framework&) = delete;
	framework& operator=(const framework&) = delete;
	framework(framework&&) noexcept = delete;
	framework& operator=(framework&&) noexcept = delete;

	int run()
	{
		MSG msg{};

		if (!initialize())
		{
			return 0;
		}

#ifdef USE_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f, nullptr, glyphRangesJapanese);
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(device.Get(), immediate_context.Get());
		ImGui::StyleColorsDark();
#endif

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
				update(tictoc.time_interval());
				render(tictoc.time_interval());
			}
		}

#ifdef USE_IMGUI
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
#endif

#if 1
		BOOL fullscreen = 0;
		swap_chain->GetFullscreenState(&fullscreen, 0);
		if (fullscreen)
		{
			swap_chain->SetFullscreenState(FALSE, 0);
		}
#endif

		return uninitialize() ? static_cast<int>(msg.wParam) : 0;
	}

	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
#ifdef USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
#endif
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			BeginPaint(hwnd, &ps);

			EndPaint(hwnd, &ps);
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		case WM_ENTERSIZEMOVE:
			tictoc.stop();
			break;
		case WM_EXITSIZEMOVE:
			tictoc.start();
			break;
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	}

	// 定数バッファ構造体
	struct ThresholdBuffer
	{
		float brightness_threshold = 1.0f;
		float padding[3];
	};

	struct BloomParams
	{
		float gaussian_sigma = 1.0f;
		float bloom_intensity = 1.0f;
		DirectX::XMFLOAT2 paddings; // 16バイト合わせ
	};

private:
	bool initialize();
	void update(float elapsed_time/*Elapsed seconds from last frame*/);
	void render(float elapsed_time/*Elapsed seconds from last frame*/);

	void render(
		ID3D11DeviceContext* immediate_context,
		float dx, float dy, float dw, float dh,
		float r, float g, float b, float a,
		float angle/*degree*/);
	bool uninitialize();

private:
	
	struct Transform
	{
		XMFLOAT3 position{ 0,0,0 };//位置
		XMFLOAT3 scale{ 1,1,1 };//スケール
		XMFLOAT3 rotation{ 0,0,0 };//回転
		float angleDeg = 0.0f;//2D用回転
		XMFLOAT4 color{ 1,1,1,1 };//色
	};

	Transform cube;
	Transform w_cube;
	Transform camera;

	ThresholdBuffer tb;
	BloomParams init;

	void editInformation
	(
		std::string label,
		Transform& t,
		float posRange = 1000.0f,
		float scaleRange = 10.0f
	);

	//ID3D11Device* device;
	//ID3D11DeviceContext* immediate_context;
	//IDXGISwapChain* swap_chain;
	//ID3D11RenderTargetView* render_target_view;
	//ID3D11DepthStencilView* depth_stencil_view;
	//ID3D11SamplerState* sampler_state[3];
	//ID3D11DepthStencilState* depth_stencil_state[4];
	//ID3D11BlendState* blend_states[4];

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state[3];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_state[4];
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_states[4];
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> reasterizer_states[3];
	Microsoft::WRL::ComPtr<ID3D11Buffer>constnt_buffer[8];
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];

	Microsoft::WRL::ComPtr<ID3D11Buffer> thresholdBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> bloomParamBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> blurDirectionBuffer;//ブラー方向をシェーダーに渡すための定数バッファ

	BloomParams bloomParams = { 1.0f, 1.0f, {0.0f, 0.0f} };

	float Long = 300.0f;
	float Rotation = 1.5f;
	float supplementation = 0.5f;//補完率


	high_resolution_timer tictoc;
	uint32_t frames_per_second{ 0 };

	std::unique_ptr<sprite>sprites[8];
	std::unique_ptr<sprite_batch>sprute_batches[8];
	std::unique_ptr<static_mesh>static_meshes[8];

	std::unique_ptr<geometric_primitive>geometric_primitives[8];

	std::unique_ptr<skinned_mesh> skinned_meshes[8];

	std::unique_ptr<framebuffer> framebuffers[8];

	struct BlurDirection
	{
		DirectX::XMFLOAT2 direction;
		DirectX::XMFLOAT2 padding;
	};

	std::unique_ptr<fullscreen_quad> bit_block_transfer;


	DirectX::XMFLOAT3 Cubuposition{ -3,0,0 };
	DirectX::XMFLOAT3 Cubuposcale{0.4,0.4, 0.4};
	DirectX::XMFLOAT3 Cuburotation;

	DirectX::XMFLOAT3 W_Cubuposition{ 3,0,0 };
	DirectX::XMFLOAT3 W_Cubuposcale{ 1,1, 1 };
	DirectX::XMFLOAT3 W_Cuburotation;

	DirectX::XMFLOAT2 position = { 0,1280 };
	DirectX::XMFLOAT2 size = { 0,720 };
	DirectX::XMFLOAT4 render_color = { 1,1,1,1 };
	DirectX::XMFLOAT4 camera_positon = { 0.0f,0.0f,10.0f,1.0f };

	float angle = 0.0;
	float count_by_seconds{ 0.0f };
	DirectX::XMFLOAT4 material_color = { 1,1,1,1 };



	void calculate_frame_stats()
	{
		if (++frames_per_second, (tictoc.time_stamp() - count_by_seconds) >= 1.0f)
		{
			float fps = static_cast<float>(frames_per_second);
			std::wostringstream outs;
			outs.precision(6);
			outs << L"X3DGP" << L" : FPS : " << fps << L" / " << L"Frame Time : " << 1000.0f / fps << L" (ms)";
			SetWindowTextW(hwnd, outs.str().c_str());

			frames_per_second = 0;
			count_by_seconds += 1.0f;
		}
	}

	struct scene_constants
	{
		//ビュー・プロジェクション変換行列
		DirectX::XMFLOAT4X4 view_projection;

		//ライトの向き
		DirectX::XMFLOAT4 light_direction;

		DirectX::XMFLOAT4 camera_position;
	};
	scene_constants data{};

};

