#pragma once

#include <string>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <filesystem>
#include <Windows.h>

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
	//コンストラクタ
	ColliderAttachmentItem()
	{
		name = "NewCollider";
		start_bone_name = "";
		end_bone_name = "";
		radius = 0.2f;
		height = 0.5f;
		offset = { 0.0f,0.0f,0.0f };
		rotation = { 0.0f,0.0f,0.0f };
		attribute_bits = static_cast<uint32_t>(ColliderAttribute::None);
		target_mask = DEFAULT_TARGET_MASK;
		is_two_bone_link = false;
		is_active = true;
	}

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
		serializer->RegisterVariable(u8"角度", &rotation);
		serializer->RegisterVariable(u8"属性ビット", &attribute_bits);
		serializer->RegisterVariable(u8"対象マスク", &target_mask);
		serializer->RegisterVariable(u8"２ボーン連携フラグ", &is_two_bone_link);
		serializer->RegisterVariable(u8"有効フラグ", &is_active);
	}

	//復元後にintからenumに反映
	void OnDeserialized()
	{
		if (target_mask == 0)
		{
			OutputDebugStringA("[ColliderAttachmentItem 警告] target_mask が 0 のため DEFAULT_TARGET_MASK を設定しました。\n");
			target_mask = DEFAULT_TARGET_MASK;
		}
	}

public:
	std::string name ;				//コライダー名
	std::string start_bone_name;	//基準ボーン名
	std::string end_bone_name;		//終点ボーン名
	float radius;					//半径
	float height;					//長さ
	DirectX::XMFLOAT3 offset;		//オフセット
	DirectX::XMFLOAT3 rotation;		//角度
	uint32_t attribute_bits;		//属性ビット
	uint32_t target_mask;			//対象マスク
	bool is_two_bone_link;			//２ボーン連携フラグ
	bool is_active;					//有効フラグ
	bool is_sequencer_active = true;
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

	//更新処理
	void Update(ModelPreviewWindow* preview_wnidow);

	//ImGui描画及びレイキャストによる選択処理
	void RenderGui(ModelPreviewWindow* preview_window);

	//プレビュー用コライダー描画
	void RenderDebug(ShapeRenderer* renderer, ModelPreviewWindow* preview_window);

	//シーケンサプレビュー用コライダー描画
	void RenderDebugForSequencer(ShapeRenderer* renderer, ModelPreviewWindow* prevew_window);

	//JSON保存
	void SaveToJson(const std::string& file_path);

	//JSON読み込み
	void LoadFromJson(const std::string& file_path);

	//登録されている全コライダーアイテムのリストを取得
	const std::vector<std::unique_ptr<ColliderAttachmentItem>>& GetColliderItems()const { return collider_items; }

	//コライダー名を指定してON/OFF切り替え
	void SetColliderActiveByName(const std::string& target_name, bool is_active);

private:
	//ビューポートクリック時のレイキャスト判定
	void HandleRaycastSelection(ModelPreviewWindow* preview_window, const ImVec2& image_pos, const ImVec2& image_size);

	//ヒットしたノード群の中から最も親側（最上位）にあるノードを探す
	int FindHighestAmongHitNodes(const std::vector<int>& hit_node_indices, Model* model);

	//カプセル座標の計算
	bool CalculateCapsuleWorld(
		const ColliderAttachmentItem& item,
		Model* model,
		const DirectX::XMFLOAT4X4& model_world,
		DirectX::XMFLOAT3& out_center,
		DirectX::XMFLOAT4& out_rotation,
		float& out_total_height
	);

	//モデル名に基づいて保存/読み込みファイルパスを管理
	std::string GetDefaultFilePath(ModelPreviewWindow* preview_window)const;

private:
	//アタッチ対象のボーンスロット種別
	enum class BoneSelectSlot
	{
		Start,	//基準ボーン
		End		//終点ボーン
	};

private:
	std::unique_ptr<CollisionLogic> collision_logic;						//当たり判定計算
	std::vector<std::unique_ptr<ColliderAttachmentItem>> collider_items;	//設定データリスト
	int selected_item_index = -1;											//選択インデックス
	std::string save_file_path = "";										//保存先のパス

	bool is_draw_colliders = true;											//アタッチ済みコライダーの描画フラグ
	bool is_draw_node_spheres = false;										//ボーンノード当たり判定球の描画フラグ

	float node_radio = 0.3f;

	BoneSelectSlot current_select_slot = BoneSelectSlot::Start;				//現在クリックで設定する対象スロット
};

