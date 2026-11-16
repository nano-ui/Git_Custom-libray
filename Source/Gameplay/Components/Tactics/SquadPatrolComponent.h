#pragma once
#include <DirectXMath.h>

#include "Gameplay\Components\Base\Component.h"

class JsonSerializer;
class GuiInspector;

class SquadPatrolComponent :public Component
{
public:
	//コンストラクタ
	SquadPatrolComponent();

	//デストラクタ
	~SquadPatrolComponent();

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//ImGuiデバッグ描画処理
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//GuiInspector専用のインスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;


private:
	DirectX::XMFLOAT3 base_position = { 0.0f,0.0f,0.0f };

};

