#include "ModelRenderer.h"
#include "static_mesh.h"
#include "skinned_mesh.h"
#include "misc.h"
#include <DirectXMath.h>

//デバイスコンテキストとアセットマネージャーのポインタを保持
ModelRenderer::ModelRenderer(ID3D11DeviceContext* context, AssetManager* asset_manager)
	:immediate_ptr_(context), asset_manager_ptr_(asset_manager)
{
}

//描画したいModelインスタンスをリストに追加
void ModelRenderer::Draw(Model* model)
{
	//描画リストにモデルインスタンスへのポインタを追加
	draw_list_.push_back(model);
}

//登録された全てのModelを描画
void ModelRenderer::Render(const RenderContext& rc)
{
}

//静的メッシュを描画するための内部ヘルパー関数
void ModelRenderer::DrawStaticMesh(
	static_mesh* mesh,
	const DirectX::XMFLOAT4X4& world_matrix,
	const DirectX::XMFLOAT4& material_color,
	ID3D11DeviceContext* context)
{
}

//スキニングメッシュを描画するための内部ヘルパー関数
void ModelRenderer::DrawSkinnedMesh(
	skinned_mesh* mesh,
	const DirectX::XMFLOAT4X4& world_matrix,
	const DirectX::XMFLOAT4& material_color,
	const void* keyframe_ptr,
	ID3D11DeviceContext* context)
{
}
