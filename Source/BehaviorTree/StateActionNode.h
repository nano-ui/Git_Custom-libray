#pragma once
#include "ActionNode.h"
#include "BehaviorTree.h"

template<typename TargetType>
class StateActionNode :
    public ActionNode<TargetType>
{
public:
	StateActionNode(int node_id, std::weak_ptr<TargetType> owner_character,int index = 0, float action_duration_time = -1.0f, std::shared_ptr<CooldownJudgment> cooldown_judgment = nullptr)
		: ActionNode<TargetType>(node_id, owner_character), action_index(index), action_duration(action_duration_time), cooldown(cooldown_judgment)
	{

	}

	bool CanExecute()const override
	{
		std::shared_ptr<TargetType> chara = this->owner.lock();	//weak_ptrからshared_ptrを生成しキャラクターが存在するか確認
		if (!chara)return false;
		if (cooldown && !cooldown->Check() && chara->GetCurrentComboCount() == 0)
		{
			return false;
		}
		return BehaviorNode::CanExecute();
	}

	//アニメーション実行
	NodeState Execute(float elapsed_time)override
	{
		//----------------------
		//オーナーの有効性確認
		//----------------------
		std::shared_ptr<TargetType> chara = this->owner.lock();	//weak_ptrからshared_ptrを生成しキャラクターが存在するか確認
		if (!chara)	//キャラクターが存在しない場合
		{
			return NodeState::Failure;	//失敗状態を返す
		}

		auto anim_editor = chara->GetAnimationEditor();
		if (anim_editor)
		{
			anim_editor->SetFloat("ActionIndex", static_cast<float>(action_index));
		}

		BehaviorNode* previous_node_ptr = chara->GetBehaviorTree()->GetCurrentNodePtr();

		if (previous_node_ptr != this)	//前回のノードIDがこのノードのIDと一致しないか判定
		{
			this->current_step = 0;	//アニメーションが違うなら、必ずStep 0（初回処理）に戻す
		}
		chara->GetBehaviorTree()->SetCurrentNodeId(static_cast<ActionNodeID>(this->GetID()));  // ツリーの実行中ノードIDをこのノードのIDに更新
		chara->GetBehaviorTree()->SetCurrentNodePtr(this);			//実行中のノードポインタを登録する
		//----------------------
		//アニメーションの再生
		//----------------------
		switch (this->current_step)	//現在のステップに応じて処理を分岐
		{
		case 0:// 初回実行時の処理
		{		
			current_timer = 0.0f;	// タイマーを0にリセットする
			if (cooldown)
			{
				cooldown->NotifyExecution(TimeManager::Instance().GetTotalTime());
			}
			std::string current_anim = chara->GetModel()->GetCurrentAnimationName();
			this->current_step++;	//次のフレームからは待機判定を行うためステップを進める
			return NodeState::Running;	//現在は実行中であるとして返す
		}
		case 1:	//継続実行時の処理
			current_timer += elapsed_time;	//経過時間をタイマーに加算
			if (action_duration >= 0.0f)	//行動時間が指定されている場合
			{
				if (current_timer >= action_duration)//タイマーが指定時間を超えたか確認する
				{
					this->ResetState();	//次回のために状態をリセットする
					return NodeState::Success;	//待機完了として成功を返す
				}
			}
			else //行動時間がマイナスに指定されている（アニメーションの終了を待つ）場合
			{
				if (chara->GetModel()->IsAnimationEnd())	//アニメーションの再生が完了しているか確認する
				{
					this->ResetState();	//次回のために状態をリセットする

					if (anim_editor)
					{
						anim_editor->SetFloat("ActionIndex", 0.0f);
					}
					//アクション終了時に、ツリーの記憶を「無し(None)」に戻す
					chara->GetBehaviorTree()->SetCurrentNodeId(ActionNodeID::None);
					chara->GetBehaviorTree()->SetCurrentNodePtr(nullptr);

					return NodeState::Success;	//再生完了として成功を返す
				}
				else if (current_timer > 5.0f)
				{
					this->ResetState();
				}
			}
			return NodeState::Running;	//終了条件をまだ満たしていないため実行中として返す
		}
		return NodeState::Failure;	//予期せぬステップ値の場合はエラーとして失敗を返す
	}

private:
	int action_index;			//エディタと同期させるための汎用番号
	float action_duration;		//行動を継続する時間
	float current_timer = 0.0f;	//経過時間タイマー
	std::shared_ptr<CooldownJudgment> cooldown;//クールタイム判定
};