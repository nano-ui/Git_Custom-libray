#include "AnimationComponent.h"

#include "../Engine/Graphics/Model.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "../ThiedParty/json.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>

//コンストラクタ
AnimationComponent::AnimationComponent(
	std::weak_ptr<Model> target_model,
	std::weak_ptr<IAnimationListener> listener)
	:target_model(target_model)
	,event_listener(listener)
{
	current_state_key = 0;
	active_anim_key = StateBlackboard::CalculateHash("ActionAnimation");
}

//デストラクタ
AnimationComponent::~AnimationComponent()
{

}

//初期化
void AnimationComponent::Initialize()
{
	current_state_key = 0;
}

//更新
void AnimationComponent::Update(float elapsed_time, StateBlackboard* blackboard)
{
	//ブラックボードのポインタチェック
	if (!blackboard)
	{
		return;
	}

	uint32_t fallback_hash = 0;	//取得失敗時のデフォルト値
	uint32_t next_state_key = blackboard->GetValue<uint32_t>(active_anim_key, fallback_hash);	//次の状態ハッシュ

	//状態が変化したか確認
	if (current_state_key != next_state_key)
	{
		auto iterator = animaton_map.find(next_state_key);	//アニメーション名検索

		//マップ内に存在するか確認
		if (iterator != animaton_map.end())
		{
			std::shared_ptr<Model> shared_model = target_model.lock();	//モデルのポインタ

			//モデルが有効か確認
			if (shared_model)
			{
				bool is_loop = true;	//ループ再生フラグ
				shared_model->PlayAnimation(iterator->second, is_loop);
				current_state_key = next_state_key;
			}
			else
			{
				std::cerr << "Error: AnimationComponent::Update - ターゲットモデルの参照が失われています。\n";
			}
		}
		else
		{
			std::cerr << "Warning: AnimationComponent::Update - 指定されたハッシュキーのアニメーションが見つかりません。Key: " << next_state_key << "\n";
		}
	}
	//アニメーションの終了コードバック処理
	std::shared_ptr<Model> shared_model = target_model.lock();	//モデルのポインタ

	//モデルが有効か確認
	if (shared_model)
	{
		//アニメーションの終了判定
		if (shared_model->IsAnimationFinished())
		{
			std::shared_ptr<IAnimationListener> shared_listener = event_listener.lock();	//インベントのポインタ

			//イベントがあるか確認
			if (shared_listener)
			{
				shared_listener->OnAnimationEnd(current_state_key);
			}
		}
	}
}

//アニメーションマップ読み込み
bool AnimationComponent::LoadFromJson(const std::string& file_path)
{
	std::ifstream file_in(file_path);	//ファイル読み込みストリーム

	//ファイルが開けるか確認
	if (!file_in.is_open())
	{
		std::cerr << "Error: AnimationComponent::LoadFromJson - ファイルが開けません: " << file_path << "\n";
		return false;
	}

	nlohmann::json json_data;	//JSON情報格納先
	file_in >> json_data;

	//JSON内の要素をイテレーターで走査
	for (auto iterator = json_data.begin(); iterator != json_data.end(); iterator++)
	{
		std::string state_name = iterator.key();		//状態キー
		std::string animation_name = iterator.value();	//アニメーション名

		uint32_t state_hash = StateBlackboard::CalculateHash(state_name);	//状態キーのハッシュ
		animaton_map[state_hash] = animation_name;
	}
	return true;
}
