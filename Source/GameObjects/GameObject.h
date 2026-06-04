#pragma once

#include <DirectXMath.h>
#include <string>
#include <d3d11.h>

#include "../Graphics/ShapeRenderer.h"
#include "../ObjectsRender/Model.h"

struct Collider;

class GameObject
{
public:
	//コンストラクタ
	GameObject();

	//デストラクタ
	virtual ~GameObject();

	//初期化処理
	 virtual void Initialize() = 0;

	 //更新処理
	 virtual void Update(float elapsed_time) = 0;

	 //描画処理
	 virtual void Render(ID3D11DeviceContext* context) = 0;

	 //デバッグ描画処理
	 virtual void RenderDebug(ShapeRenderer* renderer) = 0;

	 //ImGuiデバッグ描画
	 virtual void RenderGui() {};

	 //削除要求
	 void Destory() { is_active = false; }

	 //生存フラグを取得
	 bool IsActive()const { return is_active; }

	 //ワールド変換行列の合成、取得
	 DirectX::XMMATRIX GetWorldMatrix()const;

	 //座標取得
	 DirectX::XMFLOAT3 GetPosition()const { return position; }

	 //コライダーを取得
	 const std::vector<Collider*>GetColliders()const { return collideres; }

protected:
	//コライダーを登録
	void AddCollider(Collider* collider) { collideres.push_back(collider); }

protected:
	DirectX::XMFLOAT3 position;	//位置
	DirectX::XMFLOAT4 rotation;	//角度
	DirectX::XMFLOAT3 scale;	//スケール倍率
	DirectX::XMFLOAT4 color;	//色
	bool is_active;				//生存フラグ
	std::vector<Collider*> collideres;	//コライダーのリスト

};

