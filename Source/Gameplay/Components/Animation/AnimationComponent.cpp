#include "AnimationComponent.h"

#include "../Engine/Graphics/Model.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
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
	current_animation_loop = true;
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
	std::shared_ptr<Model> shared_model = target_model.lock(); // スマートポインタの昇格確認

	if (shared_model)
	{
		shared_model->Update(elapsed_time);

		if (shared_model->IsAnimationFinished())
		{
			std::shared_ptr<IAnimationListener> shared_listener = event_listener.lock(); // リスナーの昇格確認

			if (shared_listener)
			{
				shared_listener->OnAnimationEnd(current_state_key);
			}
		}
	}
}

//アニメーション名で直接再生命令を出
void AnimationComponent::PlayAnimationByName(const std::string& anim_name, uint32_t state_key, bool is_loop)
{
	if (anim_name.empty() || (current_animation_name == anim_name && current_animation_loop == is_loop))
	{
		return;
	}

	std::shared_ptr<Model> shared_model = target_model.lock(); // スマートポインタの昇格確認

	if (shared_model)
	{
		shared_model->PlayAnimation(anim_name, is_loop);
		current_animation_name = anim_name;
		current_state_key = state_key;
		current_animation_loop = is_loop;
	}
}

//再生時間を取得
float AnimationComponent::GetCurrentAnimationTime()const
{
	std::shared_ptr<Model> shared_model = target_model.lock();
	if (shared_model)
	{
		return shared_model->GetAnimationTime();
	}
	OutputDebugStringA("[AnimationComponent Error] GetCurrentAnimationTime: shared_model is null!\n");
	return 0.0f;
}