#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>

#include "Engine\Collision\Collider.h"
#include "Serialization\JsonSerializer.h"

class ModelPreviewWindow;
class ShapeRenderer;
class Model;
class Camera;
class CollisionLogic;

class ColliderAttachmentItem
{
public:
	//JsonSerializerへの登録処理
	void SetupSerialization(JsonSerializer* serializer)
	{
		if (!serializer)return;
		serializer->RegisterVariable(u8"コライダー名", &name);
		serializer->RegisterVariable(u8"基準ボーン名", &start_bone_name);
		serializer->RegisterVariable(u8"終点ボーン名", &end_bone_name);
		serializer->RegisterVariable(u8"半径", &radius);
		serializer->RegisterVariable(u8"長さ", &height);
		serializer->RegisterVariable(u8"オフセット", &offset);
		serializer->RegisterVariable(u8"２ボーン連携フラグ", &is_two_bone_link);
		serializer->RegisterVariable(u8"有効フラグ", &is_active);
	}
public:
	std::string name = "NewCollider";
	std::string start_bone_name = "";
	std::string end_bone_name = "";
	float radius = 0.2f;
	float height = 0.5f;
	DirectX::XMFLOAT3 offset = { 0.0f,0.0f,0.0f };
	bool is_two_bone_link = false;
	bool is_active = true;
};

class ColliderAttachmentEditor
{
public:
	//コンストラクタ
	ColliderAttachmentEditor();

	//デストラクタ
	~ColliderAttachmentEditor();

	//初期化処理
	void Initialize();

	//ImGui描画及びレイキャストによる選択処理
	void RenderGui(ModelPreviewWindow* preview_window);

	//プレビュー用コライダー描画
	void RenderDebug(ShapeRenderer* renderer, ModelPreviewWindow* preview_window);

	//JSON保存
	void SaveToJson(const std::string& file_path);

	//JSON読み込み
	void LoadFromJson(const std::string& file_path);

private:
	//ビューポートクリック時のレイキャスト判定
	void HandleRaycastSelection(ModelPreviewWindow* preview_window, const ImVec2& image_pos, const ImVec2& image_size);

	//ヒットしたノードから親階層の最も上のノードを探す
	int FindTopHierarchyNodeIndex(int hit_node_index, Model* model);

	//カプセル座標の計算
	bool CalculateCapsuleWorld(
		const ColliderAttachmentItem& item,
		Model* model,
		const DirectX::XMFLOAT4X4& model_world,
		DirectX::XMFLOAT3& out_center,
		DirectX::XMFLOAT4& out_rotation,
		float& out_total_height
	);

private:
	std::unique_ptr<CollisionLogic> collision_logic;						//当たり判定計算
	std::vector<std::unique_ptr<ColliderAttachmentItem>> collider_items;	//設定データリスト
	int selected_item_index = -1;											//選択インデックス
	std::string save_file_path = "";										//保存先のパス
};

