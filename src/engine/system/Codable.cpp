#include "system/Codable.hpp"
#include "system/TextUtilities.hpp"
#include "resources/Texture.hpp"

#include <map>
#include <sstream>

bool Codable::decodeBool(const KeyValues & param, unsigned int position) {
	if(param.values.size() < position + 1) {
		return false;
	}
	const std::string boolString = param.values[position];

	const std::vector<std::string> allowedTrues = {"true", "True", "yes", "Yes", "1", "y", "Y"};
	for(const auto & term : allowedTrues) {
		if(boolString == term) {
			return true;
		}
	}
	return false;
}

std::string Codable::encode(bool b){
	return b ? "true" : "false";
}

glm::vec3 Codable::decodeVec3(const KeyValues & param, unsigned int position) {
	// Filter erroneous case.
	if(param.values.size() < position + 3) {
		Log::Error() << "Unable to decode vec3 from string." << std::endl;
		return glm::vec3(0.0f);
	}
	glm::vec3 vec(0.0f);
	vec[0] = std::stof(param.values[position + 0]);
	vec[1] = std::stof(param.values[position + 1]);
	vec[2] = std::stof(param.values[position + 2]);
	return vec;
}

std::string Codable::encode(const glm::vec3 & v){
	return std::to_string(v[0]) + "," + std::to_string(v[1]) + "," + std::to_string(v[2]);
}

glm::vec2 Codable::decodeVec2(const KeyValues & param, unsigned int position) {
	// Filter erroneous case.
	if(param.values.size() < position + 2) {
		Log::Error() << "Unable to decode vec2 from string." << std::endl;
		return glm::vec2(0.0f);
	}
	glm::vec2 vec(0.0f);
	vec[0] = std::stof(param.values[position + 0]);
	vec[1] = std::stof(param.values[position + 1]);
	return vec;
}

std::string Codable::encode(const glm::vec2 & v){
	return std::to_string(v[0]) + "," + std::to_string(v[1]);
}

glm::mat4 Codable::decodeTransformation(const std::vector<KeyValues> & params) {
	glm::vec3 rotationAxis(0.0f);
	glm::vec3 translation(0.0f);
	float rotationAngle = 0.0f;
	float scaling		= 1.0f;
	// Parse parameters, only keeping the three needed.
	for(const auto & param : params) {
		if(param.key == "orientation") {
			rotationAxis = Codable::decodeVec3(param);
			rotationAxis = glm::normalize(rotationAxis);
			if(param.values.size() >= 4) {
				rotationAngle = std::stof(param.values[3]);
			}

		} else if(param.key == "translation") {
			translation = Codable::decodeVec3(param);

		} else if(param.key == "scaling" && !param.values.empty()) {
			scaling = std::stof(param.values[0]);
		}
	}
	const glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translation);
	const glm::mat4 rotationMat	= rotationAngle != 0.0f ? glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis) : glm::mat4(1.0f);
	const glm::mat4 scalingMat	 = glm::scale(glm::mat4(1.0f), glm::vec3(scaling));
	return translationMat * rotationMat * scalingMat;
}


std::vector<KeyValues> Codable::encode(const glm::mat4 & transfo) {

	KeyValues scaling("scaling");
	KeyValues translation("translation");
	KeyValues orientation("orientation");
	glm::vec3 scale, trans, skew;
	glm::vec4 persp;
	glm::quat orient;
	glm::decompose(transfo, scale, orient, trans, skew, persp);
	orient = glm::normalize(orient);
	
	if(scale[0] != scale[1] || scale[0] != scale[2] || scale[1] != scale[2]){
		Log::Error() << "Encoding a non uniform scale is unsupported. (" << scale << "), using average." << std::endl;
		scale[0] = (scale[0] + scale[1] + scale[2])/3.0f;
	}
	scaling.values = {std::to_string(scale[0])};
	translation.values = {encode(trans)};
	
	const glm::vec3 axis = glm::axis(orient);
	const float angle = glm::angle(orient);
	orientation.values = {encode(axis), std::to_string(angle)};
	return {scaling, translation, orientation};
	
}

std::pair<std::string, Descriptor> Codable::decodeTexture(const KeyValues & param) {
	// Subest of descriptors supported by the scene serialization model.
	const std::map<std::string, Descriptor> descriptors = {
		{"srgb", {Layout::SRGB8_ALPHA8, Filter::LINEAR_LINEAR, Wrap::REPEAT}},
		{"rgb", {Layout::RGBA8, Filter::LINEAR_LINEAR, Wrap::REPEAT}},
		{"rgb32", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::REPEAT}},

		{"srgbcube", {Layout::SRGB8_ALPHA8, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
		{"rgbcube", {Layout::RGBA8, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
		{"rgb32cube", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
	};
	// Check if the required format exists.
	if(descriptors.count(param.key) == 0 || param.values.empty()) {
		return {"", Descriptor()};
	}
	// This is indeed a texture.
	const std::string textureString = param.values[0];
	return {textureString, descriptors.at(param.key)};
}


KeyValues Codable::encode(const Texture * texture){
	const std::vector<std::pair<std::string, Descriptor>> descriptors = {
		{"srgb", {Layout::SRGB8_ALPHA8, Filter::LINEAR_LINEAR, Wrap::REPEAT}},
		{"rgb", {Layout::RGBA8, Filter::LINEAR_LINEAR, Wrap::REPEAT}},
		{"rgb32", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::REPEAT}},

		{"srgbcube", {Layout::SRGB8_ALPHA8, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
		{"rgbcube", {Layout::RGBA8, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
		{"rgb32cube", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}},
	};
	KeyValues token("rgb");
	for(const auto & desc : descriptors){
		if(texture->gpu->descriptor() == desc.second){
			token.key = desc.first;
			break;
		}
	}
	token.values = {texture->name()};
	return token;
}

std::vector<KeyValues> Codable::decode(const std::string & codableFile) {
	std::vector<KeyValues> rawTokens;
	std::stringstream sstr(codableFile);
	std::string line;

	// First, get a list of flat tokens, cleaned up, splitting them when there are multiple on the same line.
	while(std::getline(sstr, line)) {
		// Check if the line contains a comment, remove everything after.
		const std::string::size_type hashPos = line.find('#');
		if(hashPos != std::string::npos) {
			line = line.substr(0, hashPos);
		}
		// Cleanup.
		line = TextUtilities::trim(line, " \t\r");
		if(line.empty()) {
			continue;
		}

		// Find the first colon.
		const std::string::size_type firstColon = line.find(':');
		// If no colon, ignore the line.
		if(firstColon == std::string::npos) {
			Log::Warning() << "Line with no colon encountered while parsing file. Skipping line." << std::endl;
			continue;
		}

		// We can have multiple colons on the same line, when nesting (a texture for a specific attribute for instance). In that case, store the next element as a child of the current one, recursively.
		// Create the base token.
		std::string key = line.substr(0, firstColon);
		key				= TextUtilities::trim(key, " \t");
		rawTokens.emplace_back(key);
		KeyValues * tok = &rawTokens.back();

		// Then iterate while we are find sub-tokens, denoted by colons.
		std::string::size_type previousColon = firstColon + 1;
		std::string::size_type nextColon	 = line.find(':', previousColon);
		while(nextColon != std::string::npos) {
			std::string keySub = line.substr(previousColon, nextColon - previousColon);
			keySub			   = TextUtilities::trim(keySub, " \t");
			// Store the token as a child of the previous one, and recurse.
			if(!keySub.empty()) {
				tok->elements.emplace_back(keySub);
				tok = &(tok->elements.back());
			}
			previousColon = nextColon + 1;
			nextColon	 = line.find(':', previousColon);
		}

		// Everything after the last colon are values, separated by either spaces or commas.
		// Those values belong to the last token created.
		std::string values = line.substr(previousColon);
		TextUtilities::replace(values, ",", " ");
		values = TextUtilities::trim(values, " \t");
		// Split in value tokens.
		tok->values = TextUtilities::split(values, " ", true);
		
	}

	// Parse the raw tokens and recreate the hierarchy, looking at objets (*) and array elements (-).
	std::vector<KeyValues> tokens;
	for(size_t tid = 0; tid < rawTokens.size(); ++tid) {
		const auto & token = rawTokens[tid];
		// Tokens here are guaranteed to be non-empty.
		// If the token start with a '*' it is a root object.
		if(token.key[0] == '*') {
			std::string name = token.key.substr(1);
			name			 = TextUtilities::trim(name, "\t ");
			tokens.emplace_back(name);
			tokens.back().values = token.values;
			continue;
		}
		// Can't add tokens if there are no objects.
		if(tokens.empty()) {
			continue;
		}
		// Else, we append to the current object elements list.
		auto & object = tokens.back();
		object.elements.push_back(token);

		// Array handling: search for tokens with a '-' following the current one.
		auto & array = object.elements.back();
		++tid;
		while(tid < rawTokens.size() && rawTokens[tid].key[0] == '-') {
			// Store the element in the array.
			array.elements.push_back(rawTokens[tid]);
			// Update the name.
			std::string name		  = array.elements.back().key;
			name					  = TextUtilities::trim(name.substr(1), "\t ");
			array.elements.back().key = name;
			++tid;
		}
		--tid;
	}
	return tokens;
}

std::string Codable::encode(const std::vector<KeyValues> & params, Prefix prefix, uint level) {
	std::stringstream str;
	static const std::map<Prefix, std::string> prefixes = {
		{Prefix::ROOT, "* "}, {Prefix::LIST, "- "}, {Prefix::NONE, ""}
	};
	const std::string padChar(level, '\t');
	
	for(const auto & param : params){
		str << padChar << prefixes.at(prefix) << param.key << ": ";
		for(const auto & val : param.values){
			str << val << " ";
		}
		// If no sub-elements, done.
		if(param.elements.empty()){
			str << std::endl;
			continue;
		}
		// If only one sub-element, pack it on the same line.
		if(param.elements.size() == 1){
			str << encode(param.elements, Prefix::NONE, 0);
			continue;
		}
		// Else move to the next one and generate the list.
		str << std::endl;
		const Prefix elemPrefix = (level == 0) ? Prefix::NONE : Prefix::LIST;
		str << encode(param.elements, elemPrefix, level+1);
	}
	return str.str();
}

std::string Codable::encode(const std::vector<KeyValues> & params) {
	return encode(params, Prefix::ROOT, 0);
}

std::string Codable::encode(const KeyValues & params) {
	return encode({ params }, Prefix::ROOT, 0);
}

