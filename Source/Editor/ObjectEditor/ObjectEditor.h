#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>
#include <memory>

class GameObject;
class Camera;
class CollisionManager;
class SceneSerializer;
class ObjectGizmo;

class ObjectEditor
{
public:
	//コンストラクタ
	ObjectEditor();

	//デストラクタ
	~ObjectEditor();

	//初期化
	void Initialize();

	//描画
	void RenderUi(Camera* camera, CollisionManager* collision_manager);

	//選択中のオブジェクトを取得
	GameObject* GetCurrentSelectObject()const { return current_selected_object; }

	//シーン保存
	void SaveSceneWithDialog();

	//シーン読み込み
	void LoadSceneWithDialog();

	//仮オブジェクト生成
	void CreateTempModelObject(const std::string& model_path, DirectX::XMFLOAT2 ndc);

private:
	//オブジェクト生成UI描画
	void DrawLeftPane(Camera* camera, CollisionManager* collision_manager);

	//クラス適用UI描画
	void DrawClassApplySection();

	//選択されたクラスへの置き換え処理
	void ApplySelectedClassToObject();

	//オブジェクトパラメータ編集UI描画
	void DrawRightPane();

	//保存先のファイルパスをダイアログから選択取得
	std::string SelectSavePath();

	//読み込み元のファイルパスをダイアログから選択取得
	std::string SelectOpenPath();

private:
	std::unique_ptr<SceneSerializer> scene_serializer;	//シーンシリアル
	std::unique_ptr<ObjectGizmo> object_gizmo;			//ギズモ
private:
	int inspector_selected_class_index = 0;	//選択されている変更用クラスのインデックス
	GameObject* current_selected_object;	//選択されているゲームオブジェクト
	std::vector<std::string> cached_class_names;						//クラス名リストを毎フレーム取得しないためのキャッシュ
	std::unordered_map<std::string, int> frame_class_counters;			//毎フレームのメモリ確保を避けるための連番カウント用マップ
	int current_gizmo_operation;										//ギズモの現在の操作モード
	std::string current_model_path = "";								//ドロップされたモデルのパス
};

