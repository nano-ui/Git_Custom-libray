#pragma once

#include <string>

class JsonSeializer;

class SceneSerializer
{
public:
	//JSONï€ë∂
	bool SaveScene(const std::string& file_path);

	//JSONì«Ç›çûÇ›
	bool LoadScene(const std::string& file_path);

private:

};

