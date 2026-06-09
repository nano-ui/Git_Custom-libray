#pragma once

#include <vector>
#include <string>
#include <unordered_map>

class GameObject;
class Camera;
class CollisionManager;

class ObjectEditor
{
public:
	//コンストラクタ
	ObjectEditor();

	//デストラクタ
	~ObjectEditor();

	//初期化
	void Initialize();

	//更新
	void Update(Camera* camera, CollisionManager* collision_manager);

	//描画
	void RenderUi(Camera* camera, CollisionManager* collision_manager);

private:
	//オブジェクト生成UI描画
	void DrawLeftPane(Camera* camera, CollisionManager* collision_manager);

	//オブジェクトパラメータ編集UI描画
	void DrawRightPane();

private:
	int selected_class_index;				//選択されているクラスのインデックス
	GameObject* current_selected_object;	//選択されているゲームオブジェクト
	std::vector<std::string> cached_class_names;					//クラス名リストを毎フレーム取得しないためのキャッシュ
	std::unordered_map<std::string, int> frame_class_counters;		//毎フレームのメモリ確保を避けるための連番カウント用マップ
	bool is_placement_mode = false;									//配置モード切り替えフラグ
};

