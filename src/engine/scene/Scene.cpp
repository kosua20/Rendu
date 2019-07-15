#include "scene/Scene.hpp"
#include "scene/Sky.hpp"
#include "Common.hpp"
#include "helpers/TextUtilities.hpp"

Scene::Scene(const std::string & name){
	// Append the extension if needed.
	std::string fullName = name;
	if(!TextUtilities::hasSuffix(name, ".scene")){
		fullName += ".scene";
	}
	_name = fullName;
}
	
void Scene::init(const Storage mode){
	if(_loaded){
		return;
	}
	
	// Define loaders for each keyword.
	std::map<std::string, void (Scene::*)(const std::vector<KeyValues> &, const Storage)> loaders = {
		{"scene", &Scene::loadScene}, {"object", &Scene::loadObject}, {"point", &Scene::loadLight}, {"directional", &Scene::loadLight}, {"spot", &Scene::loadLight}, {"camera", &Scene::loadCamera}
	};
	
	// Parse the file.
	const std::string sceneFile = Resources::manager().getString(_name);
	const std::vector<KeyValues> allKeyVals = Codable::parse(sceneFile);
	
	// Find the main tokens positions. Everything between two keyword belongs to the first one.
	std::vector<int> mainKeysLocations;
	for(size_t tid = 0; tid < allKeyVals.size();++tid){
		const std::string & key = allKeyVals[tid].key;
		if(loaders.count(key) > 0){
			mainKeysLocations.push_back(tid);
		}
	}
	// Add a final end key.
	mainKeysLocations.push_back(int(allKeyVals.size()));
	
	// Process each group of keyvalues.
	for(size_t mkid = 0; mkid < mainKeysLocations.size()-1; ++mkid){
		const int startId = mainKeysLocations[mkid];
		const int endId = mainKeysLocations[mkid+1];
		// Extract the corresponding subset.
		std::vector<KeyValues> subset(allKeyVals.begin() + startId, allKeyVals.begin() + endId);
		const std::string key = subset[0].key;
		// By construction (see above), all keys should have a loader.
		(this->*loaders[key])(subset, mode);
	}
	
	// Update all objects poses.
	for(auto & object : objects){
		const glm::mat4 newModel = _sceneModel * object.model();
		object.set(newModel);
	}
	// The scene model matrix has been applied to all objects, we can reset it.
	_sceneModel = glm::mat4(1.0f);
	// Update all lights bounding box infos.
	_bbox = computeBoundingBox(true);
	for(auto & light : lights){
		light->setScene(_bbox);
	}
	_loaded = true;
};

void Scene::loadObject(const std::vector<KeyValues> & params, const Storage mode){
	objects.emplace_back();
	objects.back().decode(params, mode);
}

void Scene::loadLight(const std::vector<KeyValues> & params, const Storage){
	auto light = Light::decode(params);
	if(light){
		lights.push_back(light);
	}
}

void Scene::loadCamera(const std::vector<KeyValues> & params, const Storage){
	_camera.decode(params);
}

void Scene::loadScene(const std::vector<KeyValues> & params, const Storage mode){
	background = std::unique_ptr<Object>(new Object(Object::Type::Common, Resources::manager().getMesh("plane", mode), false));
	backgroundIrradiance = std::vector<glm::vec3>(9, glm::vec3(0.0f));
	
	for(size_t pid = 0; pid < params.size(); ++pid){
		const auto & param = params[pid];
		if(param.key == "irradiance" && !param.values.empty()){
			loadSphericalHarmonics(param.values[0]);
		} else if(param.key == "probe"){
			// Move to the next parameter and try to load the texture.
			backgroundReflection = Codable::decodeTexture(params[++pid], mode);
			if(backgroundReflection == nullptr){
				--pid;
			}
		} else if(param.key == "bgcolor"){
			backgroundMode = Background::COLOR;
			// Background is a plane, store the color.
			backgroundColor = Codable::decodeVec3(param);
		} else if(param.key == "bgimage"){
			backgroundMode = Background::IMAGE;
			// Background is a textured plane.
			// Move to the next parameter.
			const TextureInfos * tex = Codable::decodeTexture(params[++pid], mode);
			if(tex){
				background->addTexture(tex);
			} else {
				--pid;
			}
		} else if(param.key == "bgsky"){
			// In that case the background is a sky object.
			backgroundMode = Background::ATMOSPHERE;
			background = std::unique_ptr<Sky>(new Sky(mode));
			background->decode(params, mode);
			// Load the scattering table.
			const TextureInfos * tex = Resources::manager().getTexture("scattering-precomputed", {GL_RGB32F, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE}, mode);
			background->addTexture(tex);
			
		}  else if(param.key == "bgcube"){
			backgroundMode = Background::SKYBOX;
			// Object is a textured skybox.
			background =  std::unique_ptr<Object>(new Object(Object::Type::Common, Resources::manager().getMesh("skybox", mode), false));
			// Move to the next parameter.
			const TextureInfos * tex = Codable::decodeTexture(params[++pid], mode);
			if(tex){
				background->addTexture(tex);
			} else {
				--pid;
			}
		}
	}
	// Update matrix.
	_sceneModel = Codable::decodeTransformation(params);
}

void Scene::loadSphericalHarmonics(const std::string & name){
	backgroundIrradiance.clear();
	backgroundIrradiance.resize(9);
	
	const std::string coeffsRaw = Resources::manager().getString(name);
	std::stringstream coeffsStream(coeffsRaw);
	
	float x = 0.0f; float y = 0.0f; float z = 0.0f;
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
			bbox = objects[oid].boundingBox();
			continue;
		}
		bbox.merge(objects[oid].boundingBox());
	}
	Log::Info() << Log::Resources << "Scene bounding box: [" << bbox.minis << ", " << bbox.maxis << "]." << std::endl;
	return bbox;
}

void Scene::update(double fullTime, double frameTime){
	for(auto & light : lights){
		light->update(fullTime, frameTime);
	}
	for(auto & object : objects){
		object.update(fullTime, frameTime);
	}
	background->update(fullTime, frameTime);
}

void Scene::clean() {
	for(auto& light : lights){
		light->clean();
	}
}
