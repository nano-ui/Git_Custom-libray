#include "GltfDynamicModel.h"

//=================================================
//コンストラクタ
//=================================================
GltfDynamicModel::GltfDynamicModel(std::shared_ptr<GltfDynamicModelResource> resource_)
{
}

//=================================
//描画処理
//=================================
void GltfDynamicModel::Draw(ID3D11DeviceContext* dc)
{
}

//=================================================
//定数バッファの更新
//=================================================
void GltfDynamicModel::UpdateConstantBuffer(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& color)
{
}
