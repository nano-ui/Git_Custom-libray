#include "GameObject.h"

//コンストラクタ
GameObject::GameObject()
{
	//基本情報の設定
	position = { 0.0f,0.0f,0.0f };
	rotation = { 0.0f,0.0f,0.0f,1.0f };
	scale = { 1.0f,1.0f,1.0f };
	color = { 1.0f,1.0f,1.0f,1.0f };
	is_active = true;
}

//デストラクタ
GameObject::~GameObject()
{
}

//ワールド変換行列の合成、取得
DirectX::XMMATRIX GameObject::GetWorldMatrix() const
{
	//各トランスフォーム成分の独立した行列生成
	DirectX::XMMATRIX matrix_scale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX matrix_rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));
	DirectX::XMMATRIX matrix_translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//ワールド行列の作成
	DirectX::XMMATRIX matrix_world = matrix_scale * matrix_rotation * matrix_translation;

	return matrix_world;
}