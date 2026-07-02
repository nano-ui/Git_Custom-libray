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
	void Update(float elapsed_time);

	//アニメーション名で直接再生命令を出
	void PlayAnimationByName(const std::string& anim_name, uint32_t state_key, bool is_loop);

	//アニメーションマップの設定
	void SetAnimationMap(const std::unordered_map<uint32_t, std::string>& new_map);

	//再生時間を取得
	float GetCurrentAnimationTime()const;

	//再生中のアニメーション名を取得
	const std::string& GetCurrentAnimationName()const { return current_animation_name; }

private:
	std::unordered_map<uint32_t, std::string> animaton_map;	//ハッシュキーとアニメーション名の対応表
	std::weak_ptr<Model> target_model;						//対象のモデル
	std::weak_ptr<IAnimationListener> event_listener;		//イベント通知先
	uint32_t current_state_key;								//現在再生中の状態ハッシュキー
	std::string current_animation_name;						//再生中のアニメーション名
	bool current_animation_loop = true;						//アニメーションループフラグ
};

