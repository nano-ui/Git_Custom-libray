#pragma once

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <wrl.h>

class GltfModelData;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
struct ID3D11Buffer;
class CustomShader;

class GltfInstancingRenderer
{
public:
	static constexpr UINT MAX_INSTANCE_COUNT = 10000;	//描画できる最大インスタンス数
	static constexpr UINT INSTANCE_SLOT = 1;			//インスタンスバッファを割り当てるスロット番号
	static constexpr UINT MAX_TOTAL_BONES = 100000;		//確保するボーン行列の最大数
	static constexpr UINT BONE_BUFFER_SLOT = 10;		//ボーンバッファをセットするスロット番号

	static constexpr UINT SHADER_SLOT_0 = 0;			//シェーダースロット0番
	static constexpr UINT SHADER_SLOT_1 = 1;			//シェーダースロット1番
	static constexpr UINT RESOURCE_COUNT_1 = 1;			//一度にバインドするリソースの数
	static constexpr UINT OFFSET_ZERO = 0;				//バッファ等のオフセット初期値

	//インスタンスごとにGPUへ送るデータ構造体
	struct instance_data
	{
		DirectX::XMFLOAT4X4 world_matrix;	//各インスタンスのワールド行列
		UINT bone_offset;					//構造化バッファ内のボーン開始インデックス
		UINT pad[3];						//16バイト境界に揃えるためのアライメント用変数
	};

public:
	//コンストラクタ
	GltfInstancingRenderer(ID3D11Device* device);

	//デストラクタ
	~GltfInstancingRenderer(){}

	//複数のインスタンスを一括で描画
	void RenderInstanced(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<instance_data>& instances);

	//ボーン情報を格納する大容量バッファの作成
	void InitializeBoneBuffer(ID3D11Device* device);

private:
	//ボーン配列を一括更新してGPUにバインド
	void UpdateAndBindBoneBuffer(ID3D11DeviceContext* immediate_context, const std::vector<DirectX::XMFLOAT4X4>& all_bone_matrices);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> instance_buffer;				//各インスタンスの行列を転送するための動的バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;				//プリミティブ情報を送る定数バッファ
	std::unique_ptr<CustomShader> custom_shader;						//インスタンシング対応のシェーダー
	Microsoft::WRL::ComPtr<ID3D11Buffer> bone_matrix_buffer;			//全インスタンスのボーン行列を格納する構造化バッファ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bone_matrix_srv;	//シェーダーからバッファを読むためのビュー
};

