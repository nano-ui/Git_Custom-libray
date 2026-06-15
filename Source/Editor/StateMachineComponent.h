#pragma once

#include "EditorConponents.h"

#include <memory>
#include <string>

class StateMachine;

class StateMachineComponent :public EditorConponents
{
public:
	//コンストラクタ
	StateMachineComponent(GameObject* owner_object);

	//デストラクタ
	~StateMachineComponent()override;

	//初期化
	void Initialize()override;

	//更新
	void Update(float elapsed_time)override;

	//シリアライズの窓口
	void SetupSerialization(class JsonSerializer* serializer)override;

	//シリアライズ読み込み完了後のフック
	void OnPostLoad()override;

	//エディタから直接パスを流し込まれる窓口
	void LoadStateMachineConfig(const std::string& file_path);

	//ステートマシン取得
	StateMachine* GetStateMachine()const { return state_machine.get(); }

	//ファイルパス取得
	const std::string& GetStateMachineCnfigPath()const { return state_machine_config_path; }

	//アニメーション名を取得
	std::string GetCurrentStateAnimationClip()const;

	//ループフラグを取得
	bool IsCurrentStateAnimLoop()const;

private:
	//JSON解析
	void BuildStateMachineFronJson(const std::string& file_path);

private:
	std::unique_ptr<StateMachine> state_machine;	//ステートマシンクラス
	std::string state_machine_config_path;			//設定ファイルの保存パス
};

