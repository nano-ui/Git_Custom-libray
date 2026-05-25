#pragma once

#include <memory>
#include <vector>
#include <d3d11.h>
#include "PostProcess.h"
#include "../Graphics/framebuffer.h"

class ShaderManager
{
public:
	//内部バッファの初期化
	void Initalize(ID3D11Device* device, int width, int height);

	//3Dシーンに描画するための初期バッファを設定
	void BeginRender(ID3D11DeviceContext* context);

	//登録されたエフェクトを順に処理し、最終画面に出力
	void EndRender(ID3D11DeviceContext* context, ID3D11RenderTargetView* back_buffer);

	//適用したいシェーダーをリストに追加
	void AddShader(std::unique_ptr<PostProcess> shader) { shaders.push_back(std::move(shader)); }

	//エフェクト適用前の元のシーン画像を取得
	ID3D11ShaderResourceView* GetSceneSRV()const { return scene_buffer->shader_resource_views[0].Get(); }

private:
	std::unique_ptr<framebuffer> scene_buffer;			//3Dシーンに最初に書き込むメインバッファ
	std::unique_ptr<framebuffer> work_buffer;			//シェーダーを交互に書き込むための一時作業用バッファ
	std::unique_ptr<fullscreen_quad> quad;				//シェーダー描画に使いまわす板ポリゴン
	std::vector<std::unique_ptr<PostProcess>> shaders;	//追加されたシェーダー配列
};

