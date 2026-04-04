#include "GltfDynamicMesh.h"

//=================================================
//コンストラクタ
//=================================================
GltfDynamicMesh::GltfDynamicMesh(
	ID3D11Device* device, 
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices)
{
	//インデックス数を保持
	index_count = static_cast<uint32_t>(indices.size());

	//頂点バッファの作成
	D3D11_BUFFER_DESC vertex_buffer_desc = {};
	vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;//高速化のため読み取り専用に設定
	vertex_buffer_desc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(vertices.size());
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertex_data = {};
	vertex_data.pSysMem = vertices.data();
	device->CreateBuffer(&vertex_buffer_desc, &vertex_data, vertex_buffer.GetAddressOf());

	//インデックスバッファの作成
	D3D11_BUFFER_DESC index_buffer_desc = {};
	index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;//高速化のため読み取り専用に設定
	index_buffer_desc.ByteWidth = sizeof(uint32_t) * index_count;
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA index_data = {};
	index_data.pSysMem = indices.data();
	device->CreateBuffer(&index_buffer_desc, &index_data, index_buffer.GetAddressOf());
}

//=================================================
//メッシュ描画
//=================================================
void GltfDynamicMesh::Render(ID3D11DeviceContext* dc)
{
	//頂点バッファの設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//インデックスバッファの設定
	dc->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	//プリミティブトポロジーの設定
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//マテリアルの設定
	if (material)
	{
		material->Bind(dc);
	}

	//描画処理
	dc->DrawIndexed(index_count, 0, 0);
}
