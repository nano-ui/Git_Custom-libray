#pragma once

#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <memory>
#include <DirectXMath.h>

class CustomShader;

//描画モード
enum class ShapeDrawMode
{
	Wireframe,			//枠線のみを描画
	Solid,				//メッシュのみを描画
	SolidAndWireframe	//メッシュと枠線、両方を描画
};

class ShapeRenderer
{
public:
	//コンストラクタ
	ShapeRenderer(ID3D11Device* device);

	//デストラクタ
	~ShapeRenderer();

	//箱の描画登録
	void DrawBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color, ShapeDrawMode mode = ShapeDrawMode::Wireframe);

	//球の描画登録
	void DrawSphere(const DirectX::XMFLOAT3& position, float radius, const DirectX::XMFLOAT4& color, ShapeDrawMode mode = ShapeDrawMode::Wireframe);

	//円柱の描画登録
	void DrawCylinder(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, float radius, float height, const DirectX::XMFLOAT4& color, ShapeDrawMode mode = ShapeDrawMode::Wireframe);

	//カプセルの描画登録
	void DrawCapsule(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, float radius, float height, const DirectX::XMFLOAT4& color, ShapeDrawMode mode = ShapeDrawMode::Wireframe);

	//蓄積された全図形の描画実行とリストクリア
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

private:
	//メッシュデータ
	struct mesh
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//頂点バッファ
		UINT vertex_count;									//頂点数
	};

	//描画リクエスト
	struct instance
	{
		mesh* target_mesh;					//描画対象
		DirectX::XMFLOAT4X4 world_matrix;	//ワールド行列
		DirectX::XMFLOAT4 color;			//描画色
		ShapeDrawMode mode;					//描画モード
	};

	//シェーダーへ送る定数バッファ
	struct constant_buffer_data
	{
		DirectX::XMFLOAT4X4 world_view_projection;	//合成行列
		DirectX::XMFLOAT4 color;					//描画色
	};

	//メッシュ生成の共通関数
	void CreateMesh(ID3D11Device* device, const std::vector<DirectX::XMFLOAT3>& vertices, mesh& out_mesh);

	//箱メッシュ生成
	void CreateBoxMesh(ID3D11Device* device);

	//球メッシュ生成
	void CreateSphereMesh(ID3D11Device* device, int subdivisions);

	//円柱メッシュ生成
	void CreateCylinderMesh(ID3D11Device* device, int subdivisions);

private:
	std::unique_ptr<CustomShader> custom_shader;			//シェーダー管理クラス
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;	//定数バッファ

	mesh box_mesh;		//箱用メッシュ
	mesh sphere_mesh;	//球用メッシュ
	mesh cylinder_mesh;	//円柱用メッシュ

	std::vector<instance> instances;	//描画リクエストされた図形の蓄積リスト
};

