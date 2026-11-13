#pragma once

#include <memory>
#include <DirectXMath.h>

class framebuffer;
struct ID3D11DeviceContext;

class ScenePreviewWindow
{
public:
	//コンストラクタ
	ScenePreviewWindow();

	//デストラクタ
	~ScenePreviewWindow();

	//初期化
	void Initialize();

	//描画先を設定
	void BeginRender(ID3D11DeviceContext* context);

	//描画先を解除
	void EndRender(ID3D11DeviceContext* context);

	//Gui描画
	void RenderGui();

private:
	//ドラッグ&ドロップ受け取り
	void DragAndDropReceipt();

private:
	std::unique_ptr<framebuffer> frame_buffer;	//フレームバッファ
};

