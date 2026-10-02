#pragma once

#include <string>
#include <cstdint>

class StateGraphDataManager;
class StateGraphConfigManager;
class AssetLoader;
class StateMachineComponent;
class StateBlackboard;
class StateGraphNavigator;

//ツールバー描画パラメータ
struct ToolbarContext
{
	StateGraphDataManager* data_manager;			//グラフデータ管理クラス
	StateGraphConfigManager* config_manager;		//環境設定管理クラス
	AssetLoader* asset_loader;						//アセット読み込みクラス
	StateMachineComponent* state_machine_component;	//シミュレーションコンポーネントクラス
	StateBlackboard* blackboard;					//ブラックボード
	StateGraphNavigator* navigator;					//階層ナビゲーションクラス
	std::string& current_loaded_file_path;			//読み込まれているファイルパス
	uint32_t& current_graph_id;						//現在の階層ID
	uint32_t& target_model_hash;					//対象モデルのハッシュ値
	bool& is_tracking_active_node;					//追尾フラグ
	bool& is_simulation_active;						//シミュレーションフラグ
	uint32_t& last_synced_node_id;					//同期したノードID
};

//上部ツールバー及びファイル・アセット操作クラス
class StateGraphToolbar
{
public:
	//コンストラクタ
	StateGraphToolbar() = default;

	//デストラクタ
	~StateGraphToolbar() = default;

	//ツールバー描画
	bool DrawToolbar(
		ToolbarContext& context	//ツールバー描画コンテキスト
	);

private:
	//グラフデータの保存処理
	void ExecuteSave(
		ToolbarContext& context	//ツールバー描画コンテキスト
	);

	//モデルファイルの読み込み・関連付け処理
	bool ExecuteLoadModel(
		ToolbarContext& context	//ツールバー描画コンテキスト
	);

};

