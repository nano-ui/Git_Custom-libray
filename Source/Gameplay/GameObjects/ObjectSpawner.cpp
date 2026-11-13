#include "ObjectSpawner.h"
#include "GameObject.h"
#include "ObjectFactory.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Engine\Core\Log\Log.h"
#include "Utility\JsonFileUtility.h"

#include <filesystem>
#include <fstream>
#include <memory>

//オブジェクト出現
GameObject* ObjectSpawner::ObjectSpawn(const std::string& model_path, const DirectX::XMFLOAT3& spawn_positon)
{
    //ガード処理
    if (model_path.empty())
    {
        LOG_WARNING({ u8"設定" }, u8"モデルパスが空です(モデルパス: %s)", model_path.c_str());
        return nullptr;
    }

    //ファイルとJSONの検証
    std::string model_name = std::filesystem::path(model_path).stem().string();
    std::string detail_file_path = "Data/Json/" + model_name + "/" + model_name + ".json";
    if (!std::filesystem::exists(model_path))
    {
        LOG_ERROR({ u8"存在" }, u8"モデルファイルが存在しません(ファイルパス: %s)", model_name.c_str());
        return nullptr;
    }
    if (!std::filesystem::exists(detail_file_path))
    {
        LOG_ERROR({ u8"存在" }, u8"JSONパスが存在しません(JSONパス: %s)", detail_file_path.c_str());
        return nullptr;
    }

    //JSONの読込とオブジェクト生成
    nlohmann::json json_data;
    std::string class_name = "";
    if (!JsonFileUtility::Load(detail_file_path, json_data))return nullptr;
    if (!JsonFileUtility::TryGetValue(json_data, u8"クラス名", class_name))return nullptr;
    GameObject* new_object = ObjectFactory::CreateAndRegister(class_name);
    if (!new_object)return nullptr;
    new_object->Initialize();
    std::shared_ptr<ModelComponent> model = new_object->GetComponent<ModelComponent>();
    if (!model)
    {
        new_object->Destory();
        return nullptr;
    }
    if (!model->LoadModel(model_path))
    {
        new_object->Destory();
        return nullptr;
    }
    new_object->LoadFromJObject(json_data, SerializeTarget::Detail);
    std::shared_ptr<TransformComponent> transform = new_object->GetComponent<TransformComponent>();
    if (!transform)
    {
        new_object->Destory();
        return nullptr;
    }
    transform->SetPosition(spawn_positon);

    return new_object;
}
