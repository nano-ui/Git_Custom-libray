#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

class Model;
class StateBlackboard;

class IAnimationListener
{
public:
	//デストラクタ
	virtual ~IAnimationListener() = default;

	//アニメーション終了時のコールバック
	virtual void OnAnimationEnd(uint32_t state_key) = 0;
};

class AnimationComponent
{
public:
	//コンストラクタ
	AnimationComponent(std::weak_ptr<Model> target_model, std::weak_ptr<IAnimationListener> listener);

	//デストラクタ
	~AnimationComponent();

	//初期化
	void Initialize();

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard);

	//アニメーションマップ読み込み
	bool LoadFromJson(const std::string& file_path);

private:
	std::unordered_map<uint32_t, std::string> animaton_map;	//ハッシュキーとアニメーション名の対応表
	std::weak_ptr<Model> target_model;						//対象のモデル
	std::weak_ptr<IAnimationListener> event_listener;		//イベント通知先
	uint32_t current_state_key;								//現在再生中の状態ハッシュキー
	uint32_t active_anim_key;								//検索用のハッシュキー
};

