#pragma once

#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

struct KeyValues {
	std::string key;
	std::vector<std::string> values;
	
	KeyValues(const std::string & aKey);
};

class Codable {
public:
	
	static glm::vec3 decodeVec3(const KeyValues & param, unsigned int start = 0);
	
	static glm::mat4 decodeTransformation(const std::vector<KeyValues> & params);
	
	static TextureInfos * decodeTexture(const KeyValues & param);
	
};
