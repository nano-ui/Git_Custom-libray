#include "ObjectFactory.h"

//«‘‚Ö‚Ì“o˜^ˆ—
void ObjectFactory::RegisterClass(const std::string& class_name, CreateFunc func)
{
	GetRegistry()[class_name] = func;
}

//•¶š—ñ‚©‚ç‚Ì¶¬‚Æ‘S©“®“o˜^
GameObject* ObjectFactory::CreateAndRegister(const std::string& class_name)
{
	auto& registry = GetRegistry();

	ObjectManager* manager_ptr = &ObjectManager::Instance();

	if (manager_ptr == nullptr)
	{
		return nullptr;
	}

	if (registry.find(class_name) != registry.end())
	{
		std::unique_ptr<GameObject> new_object = registry[class_name]();
		GameObject* raw_pointer = new_object.get();
		ObjectManager::Instance().Register(std::move(new_object));
		return raw_pointer;
	}
	return nullptr;
}

//“o˜^Ï‚İƒNƒ‰ƒX–¼ƒŠƒXƒg‚Ìæ“¾
std::vector<std::string> ObjectFactory::GetClassNames()
{
	std::vector<std::string> names;
	for (const auto& pair : GetRegistry())
	{
		names.push_back(pair.first);
	}
	return names;
}