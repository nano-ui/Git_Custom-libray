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
	current_animation_name = "";
}

//デストラクタ
AnimationComponent::~AnimationComponent()
{

}

//初期化
void AnimationComponent::Initialize()
{
	current_animation_name = "";
	current_state_key = UINT32_MAX;
}

//更新
void AnimationComponent::Update(float elapsed_time)
{
	std::shared_ptr<Model> shared_model = target_model.lock(); // スマートポインタの昇格確認変数

	if (shared_model)
	{
		shared_model->Update(elapsed_time);

		if (shared_model->IsAnimationFinished())
		{
			std::shared_ptr<IAnimationListener> shared_listener = event_listener.lock(); // リスナーの昇格確認変数

			if (shared_listener)
			{
				shared_listener->OnAnimationEnd(current_state_key);
			}
		}
	}
}

//アニメーション名で直接再生命令を出
void AnimationComponent::PlayAnimationByName(const std::string& anim_name, uint32_t state_key)
{
	if (anim_name.empty() || current_animation_name == anim_name)
	{
		return;
	}

	std::shared_ptr<Model> shared_model = target_model.lock(); // スマートポインタの昇格確認変数

	if (shared_model)
	{
		bool is_loop = true; // ループ再生フラグ変数
		shared_model->PlayAnimation(anim_name, is_loop);
		current_animation_name = anim_name;
		current_state_key = state_key;
	}
}

//アニメーションマップの設定
void AnimationComponent::SetAnimationMap(const std::unordered_map<uint32_t, std::string>& new_map)
{
}
