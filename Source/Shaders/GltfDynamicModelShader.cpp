#include "GltfDynamicModelShader.h"
#include "../Graphics/shader.h"

//==================
//コンストラクタ
//==================
GltfDynamicModelShader::GltfDynamicModelShader()
{
}

//=========
//初期化
//=========
HRESULT GltfDynamicModelShader::Initialize(ID3D11Device* device)
{
	//----------------------
	//入力レイアウトの定義
	//----------------------

	D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 頂点座標
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 法線ベクトル
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 接線ベクトル
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // テクスチャ座標
		{ "WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // スキニング重み
		{ "JOINTS",   0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ボーン番号
	};

	//-------------------------------
	//シェーダーオブジェクトの作成
	//-------------------------------

	//頂点シェーダーと入力レイアウトを作成
	HRESULT hr = create_vs_from_cso(
		device,
		"gltf_dynamic_model_vsr.cso",
		vertex_shader.GetAddressOf(),
		input_layout.GetAddressOf(),
		layout_desc,
		_countof(layout_desc)
	);

	if (FAILED(hr)) return hr;

	hr = create_ps_from_cso(
		device,
		"gltf_dynamic_model_ps.cso",
		pixel_shader.GetAddressOf()
	);

	if (FAILED(hr)) return hr;

	//サンプラーステートの作成
	D3D11_SAMPLER_DESC sampler_desc = {}; //サンプラーの設定用構造体
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; //線形補間を使用
	sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; //テクスチャを繰り返して描画
	//設定を元にサンプラーオブジェクトを作成
	hr = device->CreateSamplerState(&sampler_desc, common_sampler.GetAddressOf());
	if (FAILED(hr)) return hr;

	//------------------------------
	//シーン用定数バッファの作成
	//------------------------------

	D3D11_BUFFER_DESC cb_desc = {}; // バッファの設定用構造体
	cb_desc.ByteWidth = sizeof(SceneConstantBuffer); // ビュー行列などを含む構造体のサイズを指定
	cb_desc.Usage = D3D11_USAGE_DYNAMIC; // 高速な書き換えを行うために動的として設定
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // 定数バッファとして作成
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPUからの書き込みを許可
	//設定を元にバッファを作成
	hr = device->CreateBuffer(&cb_desc, nullptr, scene_cb.GetAddressOf());

	return hr;
}

//==========================
//シーン定数バッファの更新
//==========================
void GltfDynamicModelShader::UpdateSceneBuffer(ID3D11DeviceContext* dc, const SceneConstantBuffer& scene_data)
{
	//書き込み先のメモリ情報を保持する構造体
	D3D11_MAPPED_SUBRESOURCE mapped_res;
	//書き込みのためにバッファをロック
	if (SUCCEEDED(dc->Map(scene_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res)))
	{
		//渡されたシーンデータをGPUメモリへ直接コピー
		memcpy(mapped_res.pData, &scene_data, sizeof(SceneConstantBuffer));
		//書き込みが終わったのでロックを解除し、GPUに反映
		dc->Unmap(scene_cb.Get(), 0);
	}
}

//=======================
//シェーダーのバインド
//=======================
void GltfDynamicModelShader::Bind(ID3D11DeviceContext* dc)
{
	//--------------------------------------
	//シェーダー本体とレイアウトのセット
	//--------------------------------------

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);	//頂点シェーダーを適用
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);	//ピクセルシェーダーを適用
	dc->IASetInputLayout(input_layout.Get());

	//-------------------------
	//定数バッファのセット
	//-------------------------

	ID3D11Buffer* v_cbs[] = { scene_cb.Get() };	//ビュー投影行列などが入ったバッファ
	dc->VSSetConstantBuffers(1, 1, v_cbs);		//頂点シェーダーのレジスタb1にセット
	dc->PSSetConstantBuffers(1, 1, v_cbs);		//ピクセルシェーダー側でもライト情報を使うためb1にセット

	//---------------------
	//サンプラーのセット
	//---------------------

	ID3D11SamplerState* samplers[] = { common_sampler.Get() };	//作成済みのサンプラー
	dc->PSSetSamplers(0, 1, samplers);							//ピクセルシェーダーのレジスタs0にセット
}
