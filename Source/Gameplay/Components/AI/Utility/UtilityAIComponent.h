#pragma once
#include "Gameplay\Components\Base\Component.h"
#include "UtilityTypes.h"
#include <unordered_map>
#include <functional>
#include <string>

class JsonSerializer;
class GuiInspector;
class ShapeRenderer;
struct ID3D11DeviceContext;

class UtilityAIComponent : public Component
{
public:
	//コンストラクタ
	UtilityAIComponent();

	//デストラクタ
	~UtilityAIComponent()override;

	//初期化処理
	void Initialize()override;

	//更新処理
	void Update(float elapsed_time)override;

	//描画処理
	void Render(ID3D11DeviceContext* context)override {}

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer)override {}

	//Gui処理
	void RenderGui()override;

	//JSON保存用変数登録
	void SetupSerialization(JsonSerializer* serializer)override;

	//インスペクター項目登録
	void SetupInspector(GuiInspector* inspector)override;

public:
	//行動の登録
	size_t RegisterAction(const std::string& action_name);

	//必須条件の追加
	void AddRequirements(size_t action_id, const std::function<float()>& evaluation_function);

	//ボーナス条件の追加
	void AddBonus(size_t action_id, const std::function<float()>& evaluation_function);

	//候補の全削除
	void Clear();

	//コールバック関数の登録
	void RegisterCallback(const std::function<void(size_t)>& callback_function);

	//行動の重み設定
	void SetWeight(size_t action_id, float weight);

public:
	//設計情報の設定
	void SetUtilitySettings(const UtilitySettings& settings);

	//設定情報の取得
	UtilitySettings GetUtilitySettings()const;

	//現在の行動名の取得
	std::string GetCurrentActionName()const;

	//現在の行動IDの取得
	size_t GetCurrentActionId()const;

private:
	float time_spent_thinking;							//思考時間
	size_t current_action_id;							//現在の行動ID
	std::vector<size_t> hash_id_list;					//ハッシュIDリスト
	std::function<void(size_t)> select_action_function;	//選択された行動関数
	UtilitySettings utility_settings;					//設計情報
	std::unordered_map<size_t, UtilityAction> utility_action_map;	//行動候補の辞書
};