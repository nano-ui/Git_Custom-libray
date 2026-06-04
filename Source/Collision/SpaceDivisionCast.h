#pragma once
#include <vector>
#include <DirectXMath.h>
#include <DirectXCollision.h>

class SpaceDivisionCast
{
public:
	//コンストラクタ
	SpaceDivisionCast() = default;

	//デストラクタ
	~SpaceDivisionCast() = default;

	//空間分割データを構築
	void Build(const std::vector<DirectX::XMFLOAT3>& vertices_data, const std::vector<uint32_t>& indices_data);

	//レイキャストを実行
	bool Raycast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal);

	//疑似スフィアキャストを実行
	bool PsseudoSpheraCast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, float radius, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal);
	
	//静的球交差判定
	bool StaticSpheraCast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, float radius, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal);

	//複数球を用いた疑似カプセルキャスト
	bool MultiSpheraCast(const DirectX::XMFLOAT3& bottom_pos, const DirectX::XMFLOAT3& top_pos, float radius, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal);

	//静的OBBキャスト
	bool StaticObbCast(const DirectX::XMFLOAT3& center_pos, const DirectX::XMFLOAT3& extents, const DirectX::XMFLOAT4& orientation, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal);

	//全ての境界線のリストを取得
	std::vector<DirectX::BoundingBox>GetAreaBoundingBoxes()const;

	//エリアの総数を取得
	size_t GetAreaCount();

private:
	//エリアを作成
	void CreateAreas(const DirectX::XMFLOAT3& volume_min, const DirectX::XMFLOAT3& volume_max);

	//カプセルの隙間を埋めるために必要な球の数を計算
	int CalculateSpheraCount(float capsule_length, float radius)const;

	//2点間を指定した割合で線形補間した座標を計算
	DirectX::XMFLOAT3 GetLerpPosition(const DirectX::XMVECTOR& start_vec, const DirectX::XMVECTOR& end_vec, float ratio)const;

private:
	//三角形データを保持
	struct Traiangle
	{
		DirectX::XMFLOAT3 position[3];	//頂点座標3つを保持
		DirectX::XMFLOAT3 normal;		//面の法線ベクトルを保持
	};

	//空間分割された1つのエリアを保持
	struct Area
	{
		DirectX::BoundingBox bounding_box;	//エリアの境界ボックス
		std::vector<int> triangle_indices;	//エリアに含まれる三角形のインデックスリスト
	};

private:
	std::vector<Traiangle> triangles_list;		//全三角形データを保持する動的配列
	std::vector<Area> areas_list;				//分割されたエリア情報を保持する動的配列
	bool is_built = false;						//空間分割が構築済みかどうかを示すフラグ
	static constexpr float CELL_SIZE = 4.0f;	//分割するセルの1辺のサイズ
	static constexpr float EPSILON_NORMAL = 0.0001f;
	static constexpr float SPHERE_INTERVAL_RATIO = 1.0f;	//球を配置する間隔の密度比率
};

