#include "ShapeRenderer.h"
#include "../Shaders/CustomShader.h"
#include "../Graphics/Graphics.h"

#include <cmath>

//コンストラクタ
ShapeRenderer::ShapeRenderer(ID3D11Device* device)
{
	//シェーダーの初期化
	custom_shader = std::make_unique<CustomShader>();
	bool success = custom_shader->Initialize("shape_renderer_vs.cso", "shape_renderer_ps.cso");
	_ASSERT_EXPR(success, L"Failed to initialize CustomShader in ShapeRenderer.");

	//定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(constant_buffer_data);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());

	//各基本メッシュを生成
	CreateBoxMesh(device);
	CreateSphereMesh(device, 16);
	CreateCylinderMesh(device, 16);
}

//デストラクタ
ShapeRenderer::~ShapeRenderer()
{
	instances.clear();
	if (custom_shader)
	{
		custom_shader.reset();
	}
	constant_buffer.Reset();
	box_mesh.vertex_buffer.Reset();
	sphere_mesh.vertex_buffer.Reset();
	cylinder_mesh.vertex_buffer.Reset();
}

//箱の描画登録
void ShapeRenderer::DrawBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, const DirectX::XMFLOAT3& size, const DirectX::XMFLOAT4& color, ShapeDrawMode mode)
{
	//行列合成
	DirectX::XMMATRIX scale_matrix = DirectX::XMMatrixScaling(size.x, size.y, size.z);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));
	DirectX::XMMATRIX translation_matrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX world = scale_matrix * rotation_matrix * translation_matrix;

	//描画リストに登録
	instance new_instance = {};
	new_instance.target_mesh = &box_mesh;
	DirectX::XMStoreFloat4x4(&new_instance.world_matrix, world);
	new_instance.color = color;
	new_instance.mode = mode;
	instances.push_back(new_instance);
}

//球の描画登録
void ShapeRenderer::DrawSphere(const DirectX::XMFLOAT3& position, float radius, const DirectX::XMFLOAT4& color, ShapeDrawMode mode)
{
	//行列合成
	DirectX::XMMATRIX world = DirectX::XMMatrixScaling(radius, radius, radius) * DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//描画リストに登録
	instance new_instance = {};
	new_instance.target_mesh = &sphere_mesh;
	DirectX::XMStoreFloat4x4(&new_instance.world_matrix, world);
	new_instance.color = color;
	new_instance.mode = mode;
	instances.push_back(new_instance);
}

//円柱の描画登録
void ShapeRenderer::DrawCylinder(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, float radius, float height, const DirectX::XMFLOAT4& color, ShapeDrawMode mode)
{
	//行列合成
	DirectX::XMMATRIX scale_matrix = DirectX::XMMatrixScaling(radius, height, radius);
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));
	DirectX::XMMATRIX translation_matrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX world = scale_matrix * rotation_matrix * translation_matrix;

	//描画リストに登録
	instance new_instance = {};
	new_instance.target_mesh = &cylinder_mesh;
	DirectX::XMStoreFloat4x4(&new_instance.world_matrix, world);
	new_instance.color = color;
	new_instance.mode = mode;
	instances.push_back(new_instance);
}

//カプセルの描画登録
void ShapeRenderer::DrawCapsule(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& rotation, float radius, float height, const DirectX::XMFLOAT4& color, ShapeDrawMode mode)
{
	//中心の円柱作成
	float cylinder_height = height - (radius * 2.0f);
	if (cylinder_height < 0.0f)cylinder_height = 0.0f;
	DrawCylinder(position, rotation, radius, cylinder_height, color, mode);

	//クォータニオンから回転行列を生成
	DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));

	//上下の球のローカルオフセットを計算
	DirectX::XMVECTOR offset_vec = DirectX::XMVectorSet(0.0f, cylinder_height * 0.5f, 0.0f, 0.0f);

	//回転行列を掛けてワールド空間でのオフセット方向ベクトルを算出
	DirectX::XMVECTOR top_offset = DirectX::XMVector3TransformNormal(offset_vec, rotation_matrix);
	DirectX::XMVECTOR bottom_offset = DirectX::XMVector3TransformNormal(DirectX::XMVectorNegate(offset_vec), rotation_matrix);

	DirectX::XMVECTOR pos_vec = DirectX::XMLoadFloat3(&position);
	DirectX::XMFLOAT3 top_pos, bottom_pos;
	DirectX::XMStoreFloat3(&top_pos, DirectX::XMVectorAdd(pos_vec, top_offset));
	DirectX::XMStoreFloat3(&bottom_pos, DirectX::XMVectorAdd(pos_vec, bottom_offset));

	//算出した座標で上下の球を登録
	DrawSphere(top_pos, radius, color, mode);
	DrawSphere(bottom_pos, radius, color, mode);
}

//蓄積された全図形の描画実行とリストクリア
void ShapeRenderer::Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	if (instances.empty())return;

	//パイプライン設定
	custom_shader->Apply();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	auto states = Graphics::Instance().GetPipelineStates();

	DirectX::XMMATRIX view_matrix = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX proj_matrix = DirectX::XMLoadFloat4x4(&projection);
	DirectX::XMMATRIX vp_matrix = view_matrix * proj_matrix;

	//登録したインスタンスを全てループ
	for (const instance& inst : instances)
	{
		if (!inst.target_mesh->vertex_buffer)continue;

		UINT stride = sizeof(DirectX::XMFLOAT3);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, inst.target_mesh->vertex_buffer.GetAddressOf(), &stride, &offset);
		DirectX::XMMATRIX world_matrix = DirectX::XMLoadFloat4x4(&inst.world_matrix);
		constant_buffer_data cb_data = {};
		DirectX::XMStoreFloat4x4(&cb_data.world_view_projection, world_matrix * vp_matrix);

		//メッシュの生成
		if (inst.mode == ShapeDrawMode::Solid || inst.mode == ShapeDrawMode::SolidAndWireframe)
		{
			context->RSSetState(states->GetRasterizerState(0).Get());
			cb_data.color = inst.color;
			if (inst.mode == ShapeDrawMode::SolidAndWireframe)cb_data.color.w *= 0.5f;
			context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &cb_data, 0, 0);
			context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
			context->PSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
			context->Draw(inst.target_mesh->vertex_count, 0);
		}

		//枠線の描画
		if (inst.mode == ShapeDrawMode::Wireframe || inst.mode == ShapeDrawMode::SolidAndWireframe)
		{
			context->RSSetState(states->GetRasterizerState(1).Get());
			cb_data.color = inst.color;
			if (inst.mode == ShapeDrawMode::SolidAndWireframe) cb_data.color = { 0.0f,0.0f,0.0f,1.0f };
			context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &cb_data, 0, 0);
			context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
			context->PSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());
			context->Draw(inst.target_mesh->vertex_count, 0);
		}
	}
	instances.clear();
}

//メッシュ生成の共通関数
void ShapeRenderer::CreateMesh(ID3D11Device* device, const std::vector<DirectX::XMFLOAT3>& vertices, mesh& out_mesh)
{
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT3) * vertices.size());
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA init_data{};
	init_data.pSysMem = vertices.data();
	device->CreateBuffer(&buffer_desc, &init_data, out_mesh.vertex_buffer.GetAddressOf());
	out_mesh.vertex_count = static_cast<UINT>(vertices.size());
}

//箱メッシュ生成
void ShapeRenderer::CreateBoxMesh(ID3D11Device* device)
{
	std::vector<DirectX::XMFLOAT3> vertecies;

	//8つの頂点
	DirectX::XMFLOAT3 p[8] = {
		{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f},
		{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}
	};

	//6つの面をそれぞれ2つの三角形で構成
	int indices[] = {
		0,1,2,0,2,3,	//正面
		1,5,6,1,6,2,	//右側
		5,4,7,5,7,6,	//背面
		4,0,3,4,3,7,	//左面
		3,2,6,3,6,7,	//上面
		4,5,1,4,1,0		//下面
	};
	for (int i = 0; i < 36; i++)vertecies.push_back(p[indices[i]]);
	CreateMesh(device, vertecies, box_mesh);
}

//球メッシュ生成
void ShapeRenderer::CreateSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> vertices;
	int slices = subdivisions;
	int stacks = subdivisions / 2;

	//緯度と経度に基づいて球体を三角形で分割生成
	for (int i = 0; i < stacks; i++)
	{
		float phi1 = DirectX::XM_PI * i / stacks - DirectX::XM_PIDIV2;
		float phi2 = DirectX::XM_PI * (i + 1) / stacks - DirectX::XM_PIDIV2;
		float y1 = std::sinf(phi1);
		float r1 = std::cosf(phi1);
		float y2 = std::sinf(phi2);
		float r2 = std::cosf(phi2);
		for (int j = 0; j < slices; j++)
		{
			float theta1 = DirectX::XM_2PI * j / slices;
			float theta2 = DirectX::XM_2PI * (j + 1) / slices;

			DirectX::XMFLOAT3 p1 = { r1 * std::cosf(theta1),y1,r1 * std::sinf(theta1) };
			DirectX::XMFLOAT3 p2 = { r1 * std::cosf(theta2),y1,r1 * std::sinf(theta2) };
			DirectX::XMFLOAT3 p3 = { r2 * std::cosf(theta1),y2,r2 * std::sinf(theta1) };
			DirectX::XMFLOAT3 p4 = { r2 * std::cosf(theta2),y2,r2 * std::sinf(theta2) };

			//1マスの四角形を2つの三角形に分割して登録
			vertices.push_back(p1); vertices.push_back(p3); vertices.push_back(p2);
			vertices.push_back(p2); vertices.push_back(p3); vertices.push_back(p4);
		}
	}
	CreateMesh(device, vertices, sphere_mesh);
}

//円柱メッシュ生成
void ShapeRenderer::CreateCylinderMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> vertices;
	float step = DirectX::XM_2PI / subdivisions;
	DirectX::XMFLOAT3 top_center = { 0.0f,0.5f,0.0f };
	DirectX::XMFLOAT3 bottom_center = { 0.0f,-0.5f,0.0f };

	//上下面と側面を三角形で分割生成
	for (int i = 0; i < subdivisions; i++)
	{
		float a1 = i * step;
		float a2 = ((i + 1) % subdivisions) * step;

		DirectX::XMFLOAT3 t1 = { std::cosf(a1),0.5f,std::sinf(a1) };
		DirectX::XMFLOAT3 t2 = { std::cosf(a2),0.5f,std::sinf(a2) };
		DirectX::XMFLOAT3 b1 = { std::cosf(a1),-0.5f,std::sinf(a1) };
		DirectX::XMFLOAT3 b2 = { std::cosf(a2),-0.5f,std::sinf(a2) };

		//上面
		vertices.push_back(top_center); vertices.push_back(t1); vertices.push_back(t2);
		//下面
		vertices.push_back(bottom_center); vertices.push_back(b2); vertices.push_back(b1);
		//側面
		vertices.push_back(t1); vertices.push_back(b1); vertices.push_back(b2);
		vertices.push_back(t1); vertices.push_back(b2); vertices.push_back(t2);
	}
	CreateMesh(device, vertices, cylinder_mesh);
}

