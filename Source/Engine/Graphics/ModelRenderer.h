#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "Model.h"
#include "AssetManager.h"

//描画に必要な情報をまとめた構造体
struct RenderContext
{
	ID3D11DeviceContext* immediate_context_ptr = nullptr;		//描画命令の発行に使用するID3D11DeviceContextへのポインタ
	DirectX::XMFLOAT4X4 view_projection_matrix;					//ビュー行列と射影行列を掛け合わせた行列
	DirectX::XMFLOAT4 material_color = { 1.0f,1.0f,1.0f,1.0f };	//モデルに適応する基本色
	const void* animation_keyframe_ptr = nullptr;				//スキニングアニメーションで使用するキーフレームデータへのポインタ
};

class ModelRenderer
{
public:
	//デバイスコンテキストとアセットマネージャーのポインタを受け取る
	ModelRenderer(ID3D11DeviceContext* context, AssetManager* asset_manager);

	//描画したいModelインスタンスをリストに追加
	void Draw(Model* model);

	//登録された全てのModelを描画
	void Render(const RenderContext& rc);
private:
	//静的メッシュを描画するための内部ヘルパー関数
	void DrawStaticMesh(static_mesh* mesh, const DirectX::XMFLOAT4X4& world_matrix, const DirectX::XMFLOAT4& material_color, ID3D11DeviceContext* context);

	//スキニングメッシュを描画するための内部ヘルパー関数
	void DrawSkinnedMesh(skinned_mesh* mesh, const DirectX::XMFLOAT4X4& world_matrix, const DirectX::XMFLOAT4& material_color, const void* keyframe_ptr, ID3D11DeviceContext* context);
private:
	ID3D11DeviceContext* immediate_ptr_;	//描画処理の実行に使用するデバイスコンテキスト
	AssetManager* asset_manager_ptr_;		//メッシュの実体を取得するために使用するアセットマネージャー
	std::vector<Model*> draw_list_;			//1フレームごとの描画が必要なModelインスタンスへのポインタリスト

};

