#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include <vector>

#include "static_mesh.h"
#include "skinned_mesh.h"
#include "framebuffer.h"
#include "sprite.h"
#include "sprite_batch.h"
#include "shader.h"

//class static_mesh;
//class skinned_mesh;
//class framebuffer;
//class shader;
//class sprite;

struct MeshReference
{
	size_t mesh_index;
	bool is_skinned;
};

class AssetManager
{
public:
	AssetManager(ID3D11Device* device);

	//全てのアセット（メッシュ、フレームバッファ、シェーダーなど）の読み込みと作成を実行
	void Initializa();

	//FBXファイルを読み込み、アニメーションの有無に応じて適切なメッシュとして登録し、参照情報を返す
	MeshReference LoadModelAsset(const wchar_t* fbx_filename);

private:
	//静的メッシュとスキニングメッシュのデータをファイルから読み込み、GPUリソースを作成
	void LoadMeshes();

	//レンダーターゲットビューや深度バッファを持つフレームバッファをGPU上に作成
	void CreateFramebuffers();

	//シェーダーファイル（.cso）を読み込み、ID3D11...Shaderオブジェクトを作成
	void LoadShaders();

private:
	ID3D11Device* device_ptr_;//リソース作成に使用するID3D11Deviceへのポインタ
	std::vector<std::unique_ptr<static_mesh>> static_meshes_;	//静的メッシュの配列
	std::vector<std::unique_ptr<skinned_mesh>> skinned_meshs_;	//ボーンの存在するメッシュの配列
	std::vector<std::unique_ptr<framebuffer>> framebuffers_;	//中間描画結果を格納する配列
};

