#include "scene/Scene.hpp"
#include "Common.hpp"
#include <sstream>

Scene::Scene(const std::string & name){
	
	std::string fullName = name;
	if(!TextUtilities::hasSuffix(name, ".scene")){
		fullName += ".scene";
	}
	_name = fullName;
}

std::vector<KeyValues> Scene::parse(const std::string & sceneFile){
	std::vector<KeyValues> tokens;
	std::stringstream sstr(sceneFile);
	std::string line;
	
	while(std::getline(sstr, line)){
		if(line.empty()){
			continue;
		}
		// Check if the line contains a comment.
		const std::string::size_type hashPos =  line.find("#");
		if(hashPos != std::string::npos){
			line = line.substr(0, hashPos);
		}
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
		
		std::string::size_type previousColon = 0;
		std::string::size_type nextColon = firstColon;
		while (nextColon != std::string::npos) {
			std::string key = line.substr(previousColon, nextColon-previousColon);
			key =  TextUtilities::trim(key, " \t");
			tokens.emplace_back(key);
			previousColon = nextColon+1;
			nextColon = line.find(":", previousColon);
		}
		
		// Everything after the last colon are values, separated by either spaces or commas.
		std::string values = line.substr(previousColon);
		TextUtilities::replace(values, ",", " ");
		values = TextUtilities::trim(values, " \t");
		// Split in value tokens.
		std::stringstream valuesSstr(values);
		std::string value;
		while(std::getline(valuesSstr, value, ' ')){
			tokens.back().values.push_back(value);
		}
	}
	return tokens;
}
	
void Scene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	const std::string sceneFile = Resources::manager().getString(_name);
	
	const std::vector<KeyValues> allKeyVals = parse(sceneFile);
	
	// Find the main tokens.
	const std::vector<std::string> mainKeys = {"object", "scene", "background", "point", "directional", "spot" };
	std::vector<int> mainKeysLocations;
	for(int tid = 0; tid < allKeyVals.size();++tid){
		const std::string & key = allKeyVals[tid].key;
		if(std::find(mainKeys.begin(), mainKeys.end(), key) != mainKeys.end()){
			mainKeysLocations.push_back(tid);
		}
	}
	// Add a final end key.
	mainKeysLocations.push_back(allKeyVals.size());
	
	
	std::map<std::string, void (Scene::*)(const std::vector<KeyValues> &)> loaders = {{"object", &Scene::loadObject}, { "background", &Scene::loadBackground}, {"point", &Scene::loadPointLight}, {"directional", &Scene::loadDirectionalLight}, {"spot", &Scene::loadSpotLight}};
	
	glm::mat4 sceneModel(1.0f);
	
	// Process each group of keyvalues.
	for(int mkid = 0; mkid < mainKeysLocations.size()-1; ++mkid){
		const int startId = mainKeysLocations[mkid];
		const int endId = mainKeysLocations[mkid+1];
		// Extract the corresponding subset.
		std::vector<KeyValues> subset(allKeyVals.begin() + startId, allKeyVals.begin() + endId);
		const std::string key = subset[0].key;
		if(loaders.count(key) > 0){
			(this->*loaders[key])(subset);
		} else if(key == "scene"){
			Log::Info() << "Loading scene data." << std::endl;
			
			const auto & params = subset;
			for(int pid = 0; pid < params.size();){
				const auto & param = params[pid];
				if(param.key == "irradiance" && !param.values.empty()){
					loadSphericalHarmonics(param.values[0]);
				} else if(param.key == "probe"){
					// Move to the next parameter.
					++pid;
					const TextureInfos * tex = Codable::decodeTexture(params[pid]);
					if(tex == nullptr){
						--pid;
						break;
					}
					backgroundReflection = tex;
					
				}
				++pid;
			}
			// Update matrix.
			sceneModel = Codable::decodeTransformation(params);
		} else {
			Log::Warning() << "Unknown key, skipping." << std::endl;
		}
		
	}
	
	for(auto & object : objects){
		const glm::mat4 newModel = sceneModel * object.model();
		object.set(newModel);
	}
	
	const BoundingBox sceneBox = computeBoundingBox(true);
	for(auto & light : directionalLights){
		light.setScene(sceneBox);
	}
	for(auto & light : spotLights){
		light.setScene(sceneBox);
	}
	for(auto & light : pointLights){
		light.setScene(sceneBox);
	}
};

void Scene::loadObject(const std::vector<KeyValues> & params){
	objects.emplace_back();
	objects.back().decode(params);
}

void Scene::loadPointLight(const std::vector<KeyValues> & params){
	pointLights.emplace_back();
	pointLights.back().decode(params);
}

void Scene::loadDirectionalLight(const std::vector<KeyValues> & params){
	directionalLights.emplace_back();
	directionalLights.back().decode(params);
}

void Scene::loadSpotLight(const std::vector<KeyValues> & params){
	spotLights.emplace_back();
	spotLights.back().decode(params);
}

void Scene::loadBackground(const std::vector<KeyValues> & params){
	
	background = Object(Object::Type::Skybox, Resources::manager().getMesh("skybox"), false);
	
	for(int pid = 0; pid < params.size();){
		const auto & param = params[pid];
		if(param.key == "texture"){
			// Move to the next parameter.
			++pid;
			const TextureInfos * tex = Codable::decodeTexture(params[pid]);
			if(tex == nullptr){
				--pid;
				break;
			}
			background.addTexture(tex);
		}
		++pid;
	}
}

Scene::~Scene(){};

void Scene::loadSphericalHarmonics(const std::string & name){
	backgroundIrradiance.clear();
	backgroundIrradiance.resize(9);
	
	const std::string coeffsRaw = Resources::manager().getString(name);
	std::stringstream coeffsStream(coeffsRaw);
	
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	
	for(int i = 0; i < 9; ++i){
		coeffsStream >> x >> y >> z;
		backgroundIrradiance[i] = glm::vec3(x,y,z);
	}
	
}

BoundingBox Scene::computeBoundingBox(bool onlyShadowCasters){
	BoundingBox bbox;
	if(objects.empty()){
		return bbox;
	}
	bool first = true;
	for(size_t oid = 0; oid < objects.size(); ++oid){
		if(onlyShadowCasters && !objects[oid].castsShadow()){
			continue;
		}
		if(first){
			first = false;
			bbox = objects[oid].getBoundingBox();
			continue;
		}
		bbox.merge(objects[oid].getBoundingBox());
	}
	Log::Info() << Log::Resources << "Scene bounding box: [" << bbox.minis << ", " << bbox.maxis << "]." << std::endl;
	return bbox;
}

void Scene::update(double fullTime, double frameTime){
	
	for(auto & light : pointLights){
		light.update(fullTime, frameTime);
	}
	for(auto & light : spotLights){
		light.update(fullTime, frameTime);
	}
	for(auto & light : directionalLights){
		light.update(fullTime, frameTime);
	}
	for(auto & object : objects){
		object.update(fullTime, frameTime);
	}
	
}

void Scene::clean() {
	
	for(auto& dirLight : directionalLights){
		dirLight.clean();
	}
	for(auto& pointLight : pointLights){
		pointLight.clean();
	}
	for(auto& spotLight : spotLights){
		spotLight.clean();
	}
};
