#pragma once

#include "GameObject.h"
#include "ObjectManager.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

class ObjectFactory
{
public:
	//ŠÖ”‚ÌŒ^’è‹`
	using CreateFunc = std::function<std::unique_ptr<GameObject>()>;

	//«‘‚Ö‚Ì“o˜^ˆ—
	static void RegisterClass(const std::string& class_name, CreateFunc func);

	//•¶š—ñ‚©‚ç‚Ì¶¬‚Æ‘S©“®“o˜^
	static GameObject* CreateAndRegister(const std::string& class_name);

	//“o˜^Ï‚İƒNƒ‰ƒX–¼ƒŠƒXƒg‚Ìæ“¾
	static std::vector<std::string> GetClassNames();

private:
	//Ã“I‰Šú‰»‡˜–â‘è‚ğ‰ñ”ğ‚·‚é«‘æ“¾ˆ—
	static std::unordered_map<std::string, CreateFunc>& GetRegistry()
	{
		static std::unordered_map<std::string, CreateFunc> registry;
		return registry;
	}
};

//©“®“o˜^—pƒwƒ‹ƒp[\‘¢‘Ì
template<class T>
struct AutoRegister
{
	AutoRegister(const std::string& class_name)
	{
		ObjectFactory::RegisterClass(class_name, []()->std::unique_ptr<GameObject> {
			return std::make_unique<T>();
			});
	}
};

