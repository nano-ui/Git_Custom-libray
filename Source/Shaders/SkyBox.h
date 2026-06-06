#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <memory>
#include <DirectXMath.h>

class CustomShader;

class SkyBox
{
public:
	static constexpr UINT IBL_DIFFUSE_SLOT = 33;					//IBL拡散反射テクスチャのバインドスロット
	static constexpr UINT IBL_SPECULAR_SLOT = 34;					//IBL鏡面反射テクスチャのバインドスロット
	static constexpr UINT IBL_LUT_SLOT = 35;						//IBLルックアップテーブルのバインドスロット
	static constexpr UINT RESOURCE_COUNT_1 = 1;						//一度にバインドするリソースの数
	static constexpr UINT OFFSET_ZERO = 0;							//バッファ等のオフセット初期値
	static constexpr UINT STRIDE_SIZE = sizeof(DirectX::XMFLOAT3);	//頂点データのストライドサイズ
	static constexpr UINT STENCIL_REF_ZERO = 0;						//ステンシル参照用の初期値定数

public:
	//コンストラクタ
	SkyBox();

	//デストラクタ
	~SkyBox();

	//スカイボックスとIBL環境色テクスチャを読み込む
	bool Initialize(
		ID3D11Device* device,
		const std::wstring& skybox_tex,
		const std::wstring& diffuse_tex,
		const std::wstring& specular_tex,
		const std::wstring& lut_tex);

	//スカイボックスをシーンの背景として描画
	void Render(ID3D11DeviceContext* immediate_context);

	//全モデル向けにIBL色情報を一括バインド
	void BindIblTextures(ID3D11DeviceContext* immediate_context)const;

	ID3D11ShaderResourceView* GetSkyboxSrv() { return skybox_srv.Get(); }

private:
	std::unique_ptr<CustomShader> skybox_shader;						//スカイボックス描画用シェーダーオブジェクト
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;					//立方体の頂点データを保持するバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;					//立方体の面を構成するインデックスバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;				//行列情報を送るための定数バッファ
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state;		//専用のラスタライザステート
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_state;//専用の深度ステンシルステート

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skybox_srv;		//背景描画用のキューブマップテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuse_iem_srv;	//IBL用拡散反射テクスチャ（色情報）
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specular_pmrem_srv;//IBL用鏡面反射テクスチャ（色情報）
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lut_ggx_srv;		//IBL用ルックアップテーブルテクスチャ
};

