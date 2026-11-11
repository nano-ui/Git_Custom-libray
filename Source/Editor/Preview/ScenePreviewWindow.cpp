#include "ScenePreviewWindow.h"
#include "Engine\Graphics\Device\framebuffer.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Utility\ConvertUtillty.h"
#include "Editor\EditorMediator.h"

#include <imgui.h>
#include <DirectXMath.h>

//コンストラクタ
ScenePreviewWindow::ScenePreviewWindow() = default;

//デストラクタ
ScenePreviewWindow::~ScenePreviewWindow() = default;

//初期化
void ScenePreviewWindow::Initialize()
{
	//デバイスの取得
	ID3D11Device* device = Graphics::Instance().GetDevice();
	if (!device)
	{
		LOG_ERROR({ u8"設定" }, u8"取得したデバイスがnullです");
		return;
	}

	//ウィンドウサイズの取得
	UINT window_x = Graphics::Instance().GetCurrentWidth();
	UINT window_y = Graphics::Instance().GetCurrentHeight();
	if (window_x == 0.0f || window_y == 0.0f)
	{
		LOG_ERROR({ u8"設定" }, u8"ウィンドウのサイズが0になっています(幅: %u,高さ: %u)", window_x, window_y);
		return;
	}

	//フレームバッファの生成
	frame_buffer = std::make_unique<framebuffer>(
		device,
		window_x, window_y
	);
	LOG_SUCCESS({ u8"生成" }, u8"フレームバッファが生成できました");
}

//描画先を設定
void ScenePreviewWindow::BeginRender(ID3D11DeviceContext* context)
{
	//ガード処理
	if (!context)
	{
		LOG_ERROR({ u8"設定" }, u8"渡されたcontextがnullです");
		return;
	}

	if (!frame_buffer)
	{
		LOG_ERROR({ u8"設定" }, u8"frame_bufferが生成されていません");
		return;
	}

	//描画先の設定
	static constexpr DirectX::XMFLOAT4 default_color = { 0.2f,0.2f,0.2f,1.0f };
	frame_buffer->clear(context, default_color.x, default_color.y, default_color.z, default_color.w);
	frame_buffer->activate(context);
}

//描画先を解除
void ScenePreviewWindow::EndRender(ID3D11DeviceContext* context)
{
	//ガード処理
	if (!context)
	{
		LOG_ERROR({ u8"設定" }, u8"渡されたcontextがnullです");
		return;
	}

	if (!frame_buffer)
	{
		LOG_ERROR({ u8"設定" }, u8"frame_bufferが生成されていません");
		return;
	}

	//描画先の解除
	frame_buffer->deactivate(context);
}

//Gui描画
void ScenePreviewWindow::RenderGui()
{
	//------------------------------------------------
	//ImGuiウィンドウの定義と領域計算
	//------------------------------------------------

	//ウィンドウの開始
	ImGui::Begin(u8"シーンプレビュー");

	//表示可能領域の取得
	ImVec2 avail_size = ImGui::GetContentRegionAvail();

	//------------------------------------------------
	//描画対象リソースの取得と画像表示
	//------------------------------------------------

	//シェーダーリソースビュー（SRV）の取得
	ID3D11ShaderResourceView* srv = frame_buffer->shader_resource_views[0].Get();

	//リソースの妥当性確認
	if (!srv)
	{
		LOG_ERROR({ u8"取得" }, u8"取得したシェーダーリソースビューがnullです");
		ImGui::Text(u8"取得したシェーダーリソースビューがnullです");
		ImGui::End();
		return;
	}

	//画像の描画命令
	ImGui::Image(static_cast<ImTextureRef>(srv), avail_size);

	DragAndDropReceipt();

	//ウィンドウの終了
	ImGui::End();
}

//ドラッグ&ドロップ受け取り
void ScenePreviewWindow::DragAndDropReceipt()
{
	//画像領域の取得とドロップターゲットの開始
	ImVec2 left_top = ImGui::GetItemRectMin();
	ImVec2 screen_size = ImGui::GetItemRectSize();

	//ドロップターゲットの判定
	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_FILE_PATH");

		if (!payload)
		{
			LOG_ERROR({ u8"取得" }, u8"受け取ったpayloadがnullです");
			ImGui::EndDragDropTarget();
			return;
		}
		std::string model_path = static_cast<const char*>(payload->Data);

		//画像ローカル座標の計算
		ImVec2 mouse_pos = ImGui::GetIO().MousePos;

		DirectX::XMFLOAT2 target_pos = {
			mouse_pos.x - left_top.x,
			mouse_pos.y - left_top.y
		};

		DirectX::XMFLOAT2 ndc = ConvertUtillty::ConvertNDC(screen_size.x, screen_size.y, target_pos);

		EditorMediator::Instance().OnModelDropped(model_path, ndc);

		ImGui::EndDragDropTarget();
	}
}
