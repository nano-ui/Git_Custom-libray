#pragma once
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\Components\Tactics\TacticalTypes.h"

#include <memory>
#include <unordered_map>

class TargetSelectorComponent;
class TacticalCommandController;
class UtilityAIComponent;
class SituationEvaluator;

//指揮官
class Commander : public GameObject
{
public:
	//コンストラクタ
	Commander();

	//デストラクタ
	~Commander()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	 void Update(float elapsed_time)override;

	//描画処理
	 void Render(ID3D11DeviceContext* context)override;

	//デバッグ描画処理
	 void RenderDebug(ShapeRenderer* renderer)override;

	//ImGuiデバッグ描画
	 void RenderGui()override;

	//コンポーネントの設定
	 void SetupComponent()override;

	//シリアライズ登録
	 void SetupSerialization()override;

private:
	//戦術の登録
	size_t RegisterTactic(const std::string& tactical_name, TacticalType tactical);

	//戦術の設定
	void SetupTactics();

private:
	std::shared_ptr<TacticalCommandController> tactical_command_controller;	//戦術
	std::shared_ptr<TargetSelectorComponent> target_selector;				//対象選択
	std::shared_ptr<UtilityAIComponent> utility_ai;							//UtilityAI
	std::shared_ptr<SituationEvaluator> situaltion_evaluator;				//状況判断
private:
	std::unordered_map<size_t, TacticalType> tactical_map;	//戦術の辞書

};

