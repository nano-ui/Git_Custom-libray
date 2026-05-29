#pragma once

#include <DirectXMath.h>
#include <string>
#include <d3d11.h>

class GameObject
{
public:
	//コンストラクタ
	GameObject();

	//デストラクタ
	~GameObject();

	//初期化処理
	 virtual void Initialize() = 0;

	 //更新処理
	 virtual void Update(float elapsed_time) = 0;

	 //描画処理
	 virtual void Render(ID3D11DeviceContext* context) = 0;

	 //ImGuiデバッグ描画
	 virtual void RenderGui();

	 //削除要求
	 void Destory() { is_active = false; }

	 //生存フラグを取得
	 bool IsActive()const { return is_active; }

protected:
	DirectX::XMFLOAT3 position;	//位置
	DirectX::XMFLOAT4 rotation;	//角度
	DirectX::XMFLOAT3 scale;	//スケール倍率
	DirectX::XMFLOAT4 color;	//色
	bool is_active;				//生存フラグ
};

