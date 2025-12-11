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
	for (Model* model : draw_list_)
	{
		//モデルのワールド行列を計算
		DirectX::XMFLOAT4X4 world_matrix = model->GetWorldMatrix();

		//メッシュの種類に応じて描画関数を呼び出し
		if (model->IsSkinned())
		{
			skinned_mesh* mesh = asset_manager_ptr_->GetSkinnedMesh(model->GetMeshIndex());
			if (mesh)
			{
				DrawSkinnedMesh(
					mesh,
					world_matrix,
					rc.material_color,
					rc.animation_keyframe_ptr,
					rc.immediate_context_ptr
				);
			}
			else
			{
				static_mesh* mesh = asset_manager_ptr_->GetStaticMesh(model->GetMeshIndex());
				if (mesh)
				{
					DrawStaticMesh(
						mesh,
						world_matrix,
						rc.material_color,
						rc.immediate_context_ptr
					);
				}
			}
		}
	}

	//描画が完了したら、リストをクリア
	draw_list_.clear();
}

//静的メッシュを描画するための内部ヘルパー関数
void ModelRenderer::DrawStaticMesh(
	static_mesh* mesh,
	const DirectX::XMFLOAT4X4& world_matrix,
	const DirectX::XMFLOAT4& material_color,
	ID3D11DeviceContext* context)
{
	mesh->render(context, world_matrix, material_color);
}

//スキニングメッシュを描画するための内部ヘルパー関数
void ModelRenderer::DrawSkinnedMesh(
	skinned_mesh* mesh,
	const DirectX::XMFLOAT4X4& world_matrix,
	const DirectX::XMFLOAT4& material_color,
	const void* keyframe_ptr,
	ID3D11DeviceContext* context)
{
	mesh->render(context, world_matrix, material_color, (const skinned_mesh::animation::keyframe*)keyframe_ptr);
}
