#pragma once
#include "GraphNode.h"
#include "Editor\StateMachineEditor\Data\AnimationData.h"

#include <DirectXMath.h>

class StateGraphNode : public GraphNode
{
public:
	//コンストラクタ
	StateGraphNode();

	//デストラクタ
	~StateGraphNode()override;

	//ピンの登録処理
	void SetupPins()override;

private:
	AnimationData animation_data;	//アニメーション情報
	DirectX::XMFLOAT3 link_color;	//リンク色
	int action_category;			//アクションカテゴリー
};

