#pragma once
#include "GraphNode.h"
#include "Editor\StateMachineEditor\Data\AnimationData.h"
#include "Gameplay\StateMachine\StateBlackboard.h"

#include <DirectXMath.h>

class JsonSerializer;

class StateGraphNode : public GraphNode
{
public:
	//コンストラクタ
	StateGraphNode();

	//デストラクタ
	~StateGraphNode()override;

	//ピンの登録処理
	void SetupPins()override;

	//保存変数登録処理
	void SetupSerializer(JsonSerializer* serializer)override;

	//アニメーション情報を取得
	AnimationData GetAnimationData()const { return animation_data; }

	//アニメーション情報を設定
	void SetAnimationData(AnimationData anim_data) { animation_data = anim_data; }

	//リンク色を取得
	DirectX::XMFLOAT3 GetLinkColor()const { return link_color; }

	//リンク色設定
	void SetLinkColor(DirectX::XMFLOAT3 color) { link_color = color; }

	//アクションカテゴリーを取得
	ActionCategory GetActionCategory()const { return action_category; }

	//アクションカテゴリーを設定
	void SetActionCategory(ActionCategory action) { action_category = action; }

private:
	AnimationData animation_data;	//アニメーション情報
	DirectX::XMFLOAT3 link_color;	//リンク色
	ActionCategory action_category;	//アクションカテゴリー
};

