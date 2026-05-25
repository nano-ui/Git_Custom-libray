#include "ShaderManager.h"

//=======================
//内部バッファの初期化
//=======================
void ShaderManager::Initalize(ID3D11Device* device, int width, int height)
{
	scene_buffer = std::make_unique<framebuffer>(device, width, height);	//ピンポン処理のためのシーンバッファを画面サイズで作成
	work_buffer = std::make_unique<framebuffer>(device, width, height);		//もう一枚の作業用バッファを同じサイズで作成
	quad = std::make_unique<fullscreen_quad>(device);						//板ポリゴンを生成
}

//============================================
//3Dシーンに描画するための初期バッファを設定
//============================================
void ShaderManager::BeginRender(ID3D11DeviceContext* context)
{
	scene_buffer->clear(context);
	scene_buffer->activate(context);
}

//====================================================
//登録されたエフェクトを順に処理し、最終画面に出力
//====================================================
void ShaderManager::EndRender(ID3D11DeviceContext* context, ID3D11RenderTargetView* back_buffer)
{
	//------------------------
	//シェーダーの連鎖処理
	//------------------------
	scene_buffer->deactivate(context);	//シーンの書き込みを終了するためバッファを非アクティブ
	if (shaders.empty())	//シェーダーが設定されていない場合
	{
		context->OMSetRenderTargets(1, &back_buffer, nullptr);										//そのままバックバッファをターゲット
		quad->blit(context, scene_buffer->shader_resource_views[0].GetAddressOf(), 0, 1, nullptr);	//シーンバッファをそのまま画面にコピーして終了
		return;
	}
	ID3D11ShaderResourceView** current_source = scene_buffer->shader_resource_views[0].GetAddressOf();	//最初の入力元をシーンバッファのテクスチャに設定
	ID3D11RenderTargetView* current_dest = work_buffer->render_target_view.Get();	//最初の出力先を作業用バッファのレンダーターゲットに設定
	for (size_t i = 0; i < shaders.size(); i++)	//登録された全てのシェーダーを順番に処理
	{
		if (i == shaders.size() - 1)	//現在のシェーダーが最後か判定
		{
			current_dest = back_buffer;	//最後の出力先をバックバッファに設定
		}
		shaders[i]->Render(context, current_source, current_dest, quad.get());	//シェーダーの描画関数を呼び出し
		if (i != shaders.size() - 1)	//まだ次のエフェクトがある場合は、入出力を入れ替えるピンポン処理を行う
		{
			if (i % 2 == 0)	//偶数回か奇数回かで入出力をフリップ（入れ替え）
			{
				current_source = work_buffer->shader_resource_views[0].GetAddressOf();
				current_dest = scene_buffer->render_target_view.Get();
			}
			else
			{
				current_source = scene_buffer->shader_resource_views[0].GetAddressOf();
				current_dest = work_buffer->render_target_view.Get();
			}
		}
	}
	ID3D11ShaderResourceView* null_srv[2] = { nullptr,nullptr };
	context->PSSetShaderResources(0, 2, null_srv);
}
