#include "SceneLoading.h"
#include "SceneManager.h"
#include "Engine\Graphics\Renderers\sprite_batch.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Camera\StandCamera.h"
#include "Engine\Core\MacroTracy.h"
#include <wrl.h>
#include <objbase.h>
#include <Windows.h>

namespace
{
	constexpr float full_circle_degrees = 360.0f;
	constexpr float background_clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	constexpr UINT default_sample_mask = 0xFFFFFFFF;
	constexpr int blend_state_alpha = 0;
	constexpr wchar_t loading_icon_path[] = L"Data/Sprite/UI/LoadingIcon.png";
}

SceneLoading::SceneLoading(std::unique_ptr<Scene> next_scene)
	: next_scene(std::move(next_scene))
	, is_load_completed(false)
	, icon_size{ default_icon_size, default_icon_size }
	, icon_color{ 1.0f, 1.0f, 1.0f, 1.0f }
	, angle(0.0f)
	, rotation_speed(default_rotation_speed)
{
	BS_PROFILE_FUNCTION();

	// 画面右下に余白を持たせた初期位置を計算
	icon_pos.x = logical_screen_width - icon_size.x - icon_margin;
	icon_pos.y = logical_screen_height - icon_size.y - icon_margin;
}

SceneLoading::~SceneLoading()
{
	// スレッドが未終了のまま破棄されるのを防ぐため確実に待機
	JoinLoadingThread();
}

void SceneLoading::Initialize()
{
	BS_PROFILE_FUNCTION();

	// 1. 描画用リソース（スプライトとカメラ）の初期化
	ID3D11Device* device = Graphics::Instance().GetDevice();
	loading_sprite = std::make_unique<sprite_batch>(device, loading_icon_path, 1);

	camera = std::make_unique<StandCamera>();
	camera->Initialize();

	// 2. 次シーンの有効性チェックと非同期読み込みスレッドの起動
	if (!next_scene)
	{
		OutputDebugStringA("[エラー] SceneLoading::Initialize - 遷移先のnext_sceneがnullです。\n");
		is_load_completed.store(true);
		return;
	}

	is_load_completed.store(false);
	loading_thread = std::make_unique<std::thread>(&SceneLoading::LoadingThread, this);
}

void SceneLoading::Finalize()
{
	// 1. バックグラウンドスレッドの安全な終了待機
	JoinLoadingThread();

	// 2. 描画リソースの解放
	loading_sprite.reset();
	camera.reset();
}

void SceneLoading::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	// 1. ImGuiの画面スケール補正とカメラ更新
	Scene::ImGuiScaleCorrection();
	if (camera)
	{
		camera->Update(elapsed_time);
	}

	// 2. ローディングアイコンの回転角度更新
	angle += rotation_speed * elapsed_time;
	if (angle >= full_circle_degrees)
	{
		angle -= full_circle_degrees;
	}

	// 3. 読み込み完了を検知したら次シーンへ遷移予約
	if (is_load_completed.load() && next_scene)
	{
		JoinLoadingThread();
		SceneManager::Instance().ChangeScene(std::move(next_scene));
	}
}

void SceneLoading::Render(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	// 1. パイプラインステートと定数バッファの更新
	ID3D11DeviceContext * context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	context->OMGetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
	if (render_target_view)
	{
		context->ClearRenderTargetView(render_target_view.Get(), background_clear_color);
	}
	else
	{
		OutputDebugStringA("[警告] SceneLoading::Render - RenderTargetViewの取得に失敗しました。\n");
	}

	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);
	context->OMSetBlendState(states->GetBlendState(blend_state_alpha).Get(), blend_factor, default_sample_mask);
	context->RSSetState(states->GetRasterizerState(2).Get());
	if (camera)
	{
		Graphics::Instance().UpdateSceneConstantBuffer({
			camera->GetViewProjectionMatrix(),
			{ 0.0f, -1.0f, 1.0f, 0.0f },
			{ camera->GetEye().x, camera->GetEye().y, camera->GetEye().z, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 0.2f, 0.2f, 0.2f, 0.2f }
			});
	}

	context->RSSetState(states->GetRasterizerState(0).Get());

	// 2. 現在のビューポートサイズから論理解像度に対するスケールを計算
	D3D11_VIEWPORT viewport{};
	UINT num_viewports = 1;
	context->RSGetViewports(&num_viewports, &viewport);

	float scale_x = 1.0f;
	float scale_y = 1.0f;
	if (viewport.Width > 0.0f && viewport.Height > 0.0f)
	{
		scale_x = viewport.Width / logical_screen_width;
		scale_y = viewport.Height / logical_screen_height;
	}

	// 縦横比が崩れないよう小さい方のスケールを基準にアイコンサイズと余白を算出
	float uniform_scale = (scale_x < scale_y) ? scale_x : scale_y;
	float draw_width = icon_size.x * uniform_scale;
	float draw_height = icon_size.y * uniform_scale;
	float draw_x = viewport.Width - draw_width - (icon_margin * uniform_scale);
	float draw_y = viewport.Height - draw_height - (icon_margin * uniform_scale);

	// 3. ローディングアイコンの2D描画
	if (loading_sprite)
	{
		loading_sprite->begin(context);
		loading_sprite->render(
			context,
			draw_x, draw_y,
			draw_width, draw_height,
			icon_color.x, icon_color.y, icon_color.z, icon_color.w,
			angle
		);
		loading_sprite->end(context);
	}
#ifdef USE_IMGUI
	//RenderGui();
#endif
}

void SceneLoading::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::Begin("Loading Debug"))
	{
		if (ImGui::CollapsingHeader("SceneLoading", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat2("IconPos", &icon_pos.x, 1.0f);
			ImGui::DragFloat2("IconSize", &icon_size.x, 1.0f);
			ImGui::ColorEdit4("IconColor", &icon_color.x);
			ImGui::SliderFloat("Angle", &angle, 0.0f, full_circle_degrees);
			ImGui::DragFloat("RotationSpeed", &rotation_speed, 1.0f, 0.0f, 720.0f);
			ImGui::Text("Load Completed: %s", is_load_completed.load() ? "True" : "False");
		}
	}
	if (camera)
	{
		camera->RenderGui();
	}
	ImGui::End();
#endif
}

void SceneLoading::LoadingThread()
{
	BS_PROFILE_FUNCTION();

	// 1. スレッド固有のCOMライブラリ初期化
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		OutputDebugStringA("[警告] SceneLoading::LoadingThread - CoInitializeExに失敗しました。\n");
	}

	// 2. 次シーンの重いリソース読み込み処理を実行
	if (next_scene)
	{
		next_scene->Load();
	}

	// 3. COMライブラリの終了化と完了フラグの通知
	if (SUCCEEDED(hr))
	{
		CoUninitialize();
	}
	is_load_completed.store(true);
}

void SceneLoading::JoinLoadingThread()
{
	BS_PROFILE_FUNCTION();

	// 1. スレッドが存在し合流可能であれば完了まで待機
	if (loading_thread)
	{
		if (loading_thread->joinable())
		{
			loading_thread->join();
		}
		loading_thread.reset();
	}
}