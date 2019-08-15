#include "system/Codable.hpp"
#include "system/TextUtilities.hpp"

bool Codable::decodeBool(const KeyValues & param, unsigned int position){
	if(param.values.size() < position + 1){
		return false;
	}
	const std::string boolString = param.values[position];
	
	const std::vector<std::string> allowedTrues = {"true", "True", "yes", "Yes", "1", "y", "Y"};
	for(const auto & term : allowedTrues){
		if(boolString == term){
			return true;
		}
	}
	return false;
}

glm::vec3 Codable::decodeVec3(const KeyValues & param, unsigned int position){
	// Filter erroneous case.
	if(param.values.size() < position + 3){
		Log::Error() << "Unable to decode vec3 from string." << std::endl;
		return glm::vec3(0.0f);
	}
	glm::vec3 vec(0.0f);
	vec[0] = std::stof(param.values[position + 0]);
	vec[1] = std::stof(param.values[position + 1]);
	vec[2] = std::stof(param.values[position + 2]);
	return vec;
}

glm::vec2 Codable::decodeVec2(const KeyValues & param, unsigned int position){
	// Filter erroneous case.
	if(param.values.size() < position + 2){
		Log::Error() << "Unable to decode vec2 from string." << std::endl;
		return glm::vec2(0.0f);
	}
	glm::vec2 vec(0.0f);
	vec[0] = std::stof(param.values[position + 0]);
	vec[1] = std::stof(param.values[position + 1]);
	return vec;
}

glm::mat4 Codable::decodeTransformation(const std::vector<KeyValues> & params){
	glm::vec3 rotationAxis(0.0f);
	glm::vec3 translation(0.0f);
	float rotationAngle = 0.0f;
	float scaling = 1.0f;
	// Parse parameters, only keeping the three needed.
	for(const auto & param : params){
		if(param.key == "orientation" ){
			rotationAxis = Codable::decodeVec3(param);
			rotationAxis = glm::normalize(rotationAxis);
			if(param.values.size() >= 4){
				rotationAngle = std::stof(param.values[3]);
			}
			
		} else if(param.key == "translation"){
			translation = Codable::decodeVec3(param);
			
		} else if(param.key == "scaling" && param.values.size() > 0){
			scaling = std::stof(param.values[0]);
			
		}
	}
	const glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translation);
	const glm::mat4 rotationMat = rotationAngle != 0.0f ? glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis) : glm::mat4(1.0f);
	const glm::mat4 scalingMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaling));
	return translationMat * rotationMat * scalingMat;
}

const Texture * Codable::decodeTexture(const KeyValues & param, const Storage mode){
	// Subest of descriptors supported by the scene serialization model.
	const std::map<std::string, Descriptor> descriptors = {
		{"srgb", {GL_SRGB8_ALPHA8, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT}},
		{"rgb", {GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT}},
		{"rgb32", {GL_RGB32F, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT}},
		
		{"srgbcube", {GL_SRGB8_ALPHA8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}},
		{"rgbcube", {GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}},
		{"rgb32cube", {GL_RGB32F, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}},
	};
	// Check if the required format exists.
	if(descriptors.count(param.key) == 0 || param.values.empty()){
		return nullptr;
	}
	// This is indeed a texture.
	const std::string textureString = param.values[0];
	return Resources::manager().getTexture(textureString, descriptors.at(param.key), mode);
}


std::vector<KeyValues> Codable::parse(const std::string & codableFile){
	std::vector<KeyValues> rawTokens;
	std::stringstream sstr(codableFile);
	std::string line;
	
	// First, get a list of flat tokens, cleaned up, splitting them when there are multiple on the same line.
	while(std::getline(sstr, line)){
		// Check if the line contains a comment, remove everything after.
		const std::string::size_type hashPos = line.find("#");
		if(hashPos != std::string::npos){
			line = line.substr(0, hashPos);
		}
		// Cleanup.
		line = TextUtilities::trim(line, " \t\r");
		if(line.empty()){
			continue;
		}
		
		// Find the first colon.
		const std::string::size_type firstColon = line.find(":");
		// If no colon, ignore the line.
		if(firstColon == std::string::npos){
			Log::Warning() << "Line with no colon encountered while parsing file. Skipping line." << std::endl;
			continue;
		}
		
		// We can have multiple colons on the same line, when nesting (a texture for a specific attribute for instance). In that case, store the next element as a child of the current one, recursively.
		// Create the base token.
		std::string key = line.substr(0, firstColon);
		key = TextUtilities::trim(key, " \t");
		rawTokens.emplace_back(key);
		KeyValues * tok = &rawTokens.back();
		
		// Then iterate while we are find sub-tokens, denoted by colons.
		std::string::size_type previousColon = firstColon+1;
		std::string::size_type nextColon = line.find(":", previousColon);
		while (nextColon != std::string::npos) {
			std::string keySub = line.substr(previousColon, nextColon-previousColon);
			keySub = TextUtilities::trim(keySub, " \t");
			// Store the token as a child of the previous one, and recurse.
			if(!keySub.empty()){
				tok->elements.emplace_back(keySub);
				tok = &(tok->elements.back());
			}
			previousColon = nextColon+1;
			nextColon = line.find(":", previousColon);
		}
		
		// Everything after the last colon are values, separated by either spaces or commas.
		// Those values belong to the last token created.
		std::string values = line.substr(previousColon);
		TextUtilities::replace(values, ",", " ");
		values = TextUtilities::trim(values, " \t");
		// Split in value tokens.
		std::stringstream valuesSstr(values);
		std::string value;
		while(std::getline(valuesSstr, value, ' ')){
			if(!value.empty()){
				tok->values.push_back(value);
			}
		}
	}
	
	// Parse the raw tokens and recreate the hierarchy, looking at objets (*) and array elements (-).
	std::vector<KeyValues> tokens;
	for(size_t tid = 0; tid < rawTokens.size(); ++tid){
		const auto & token = rawTokens[tid];
		// Tokens here are guaranteed to be non-empty.
		// If the token start with a '*' it is a root object.
		if(token.key[0] == '*'){
			std::string name = token.key.substr(1);
			name = TextUtilities::trim(name, "\t ");
			tokens.emplace_back(name);
			tokens.back().values = token.values;
			continue;
		}
		// Can't add tokens if there are no objects.
		if(tokens.empty()){
			continue;
		}
		// Else, we append to the current object elements list.
		auto & object = tokens.back();
		object.elements.push_back(token);
		
		// Array handling: search for tokens with a '-' following the current one.
		auto & array = object.elements.back();
		++tid;
		while(tid < rawTokens.size() && rawTokens[tid].key[0] == '-'){
			// Store the element in the array.
			array.elements.push_back(rawTokens[tid]);
			// Update the name.
			std::string name = array.elements.back().key;
			name = TextUtilities::trim(name.substr(1), "\t ");
			array.elements.back().key = name;
			++tid;
		}
		--tid;
		
	}
	return tokens;
}
