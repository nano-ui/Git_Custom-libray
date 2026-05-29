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
