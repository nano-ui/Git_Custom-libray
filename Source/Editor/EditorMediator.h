#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>

class GameObject;
class ObjectEditor;
class StateMachineGraphEditor;
class ColliderAttachmentEditor;
class ColliderAttachmentItem; 
class ModelPreviewWindow;
class ScenePreviewWindow;
class Camera;
class CollisionManager;

class EditorMediator
{
public:
	//インスタンス取得
	static EditorMediator& Instance();

	//ステートマシンエディタのポインタを登録
	void RegisterStateMachineGraphEditor(StateMachineGraphEditor* editor);

	//モデルプレビューウィンドウのポインタを中継用に登録
	void RegisterModelPreviewWindow(ModelPreviewWindow* window);

	//シーンプレビューウィンドウのポインタを仲介用に登録
	void RegisterScenePreviewWindow(ScenePreviewWindow* window);

	//オブジェクトエディタのポインタを仲介用に登録
	void RegisterObjectEditor(ObjectEditor* editor);

	//アタッチメントエディタの登録
	void RegisterCollisionAttachmentEditor(ColliderAttachmentEditor* editor);

	//カメラの登録
	void RegisterActiveCamera(Camera* camera);

	//カメラの取得
	Camera* GetActiveCamera()const;

	//当たり判定マネージャーの登録
	void RegisterCollisionManager(CollisionManager* collision);

	//当たり判定マネージャーの取得
	CollisionManager* GetCollisionManager()const;

	//アタッチメントエディタコライダー一覧の参照取得
	const std::vector<std::unique_ptr<ColliderAttachmentItem>>* GetAttachmentColliderItems()const;

	//コライダー名を指定して有効状態を中継設定
	void SetColliderActiveByName(const std::string& collider_name, bool is_active);

	//モデルファイルがドロップした際のイベント
	void OnModelDropped(const std::string& file_path, DirectX::XMFLOAT2 ndc);

	//中継通知イベント
	void OnObjectSelected(GameObject* object);

	//コンテンツブラウザ等でモデルファイルがダブルクリックされたときに呼び出し
	void OnModelDubleClied(const std::string& file_path);

	//再生速度を仲介
	void SetModelAnimationSpeed(float speed);

	//再生状態を仲介
	void SetModelAnimationPlaying(bool playing);

	//現在の再生時間を仲介
	void SetModelAnimationTime(float time);

	//モデルの現在の再生時間を取得
	float GetModelAnimationCurrentTime() const;

	//モデルのアニメーション総時間を取得
	float GetModelAnimationDuration() const;

	//モデルのファイルパスを取得
	std::string GetModelName()const;

	//現在再生されているアニメーション名を取得
	std::string GetModelAnimationName()const;

	//モデルのアニメーション再生を中継
	void PlayModelAnimation(const std::string& anim_name, bool is_loop);

	//アクティブノードIDをエディタに同期
	void UpdateViewerSynchronization(GameObject* object);

	//対象コンポーネントへリロード命令を仲介
	void NotifyGraphChanged(const std::string& file_path);

	//ウィンドウサイズの設定
	void SetWindowSize(const DirectX::XMFLOAT2& size){ window_size = size; }

	//ウィンドウサイズの取得
	DirectX::XMFLOAT2 GetWindowSize()const { return window_size; }

	//ウィンドウ座標の設定
	void SetWindowPos(const DirectX::XMFLOAT2 pos) { window_pos = pos; }

	//ウィンドウ座標の取得
	DirectX::XMFLOAT2 GetWindowPos()const { return window_pos; }

	//ギズモ表示
	void RenderScenePreviewOverlay();

private:
	//外部での生成を禁止
	EditorMediator();

	//デストラクタ
	~EditorMediator();

private:
	Camera* active_camera = nullptr;								//カメラのポインタ
	CollisionManager* collision_manager = nullptr;					//コリジョンマネージャーのポインタ
	StateMachineGraphEditor* state_machine_graph_editor = nullptr;	//対象エディタのポインタ
	GameObject* last_selected_object = nullptr;						//前回選択オブジェクト
	ModelPreviewWindow* model_preview_window = nullptr;				//モデルプレビュー
	ScenePreviewWindow* scene_preview_window = nullptr;				//シーンプレビュー
	ObjectEditor* object_editor = nullptr;							//オブジェクトエディタのポインタ
	ColliderAttachmentEditor* collider_attachment_editor = nullptr;	//コライダーアタッチメントエディタのポインタ

private:
	DirectX::XMFLOAT2 window_size = { 0.0f,0.0f };					//ウィンドウサイズ
	DirectX::XMFLOAT2 window_pos = { 0.0f,0.0f };					//ウィンドウ座標
};

