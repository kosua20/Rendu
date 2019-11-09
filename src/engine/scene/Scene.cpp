#include "scene/Scene.hpp"
#include "scene/Sky.hpp"
#include "system/TextUtilities.hpp"


#include <map>
#include <sstream>

Scene::Scene(const std::string & name) {
	// Append the extension if needed.
	std::string fullName = name;
	if(!TextUtilities::hasSuffix(name, ".scene")) {
		fullName += ".scene";
	}
	_name = fullName;
}

void printToken(const KeyValues & tk, const std::string & shift) {
	Log::Info() << shift << tk.key << ": " << std::endl;
	if(!tk.values.empty()) {
		Log::Info() << shift << "\t";
		for(const auto & val : tk.values) {
			Log::Info() << val << " | ";
		}
		Log::Info() << std::endl;
	}
	for(const auto & subtk : tk.elements) {
		printToken(subtk, shift + "\t");
	}
}

void Scene::init(Storage mode) {
	if(_loaded) {
		return;
	}
	const auto start = std::chrono::steady_clock::now();
	
	// Define loaders for each keyword.
	std::map<std::string, void (Scene::*)(const KeyValues &, Storage)> loaders = {
		{"scene", &Scene::loadScene}, {"object", &Scene::loadObject}, {"point", &Scene::loadLight}, {"directional", &Scene::loadLight}, {"spot", &Scene::loadLight}, {"camera", &Scene::loadCamera}, {"background", &Scene::loadBackground}};

	// Parse the file.
	const std::string sceneFile			   = Resources::manager().getString(_name);
	const std::vector<KeyValues> allParams = Codable::parse(sceneFile);

	// Process each group of keyvalues.
	for(const auto & element : allParams) {
		const std::string key = element.key;
		// By construction (see above), all keys should have a loader.
		(this->*loaders[key])(element, mode);
	}

	// Update all objects poses.
	for(auto & object : objects) {
		const glm::mat4 newModel = _sceneModel * object.model();
		object.set(newModel);
	}
	// The scene model matrix has been applied to all objects, we can reset it.
	_sceneModel = glm::mat4(1.0f);
	// Update all lights bounding box infos.
	_bbox = computeBoundingBox(true);
	for(auto & light : lights) {
		light->setScene(_bbox);
	}
	_loaded = true;
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << Log::Resources << "Loading took " << duration.count() << "ms." << std::endl;
}

void Scene::loadObject(const KeyValues & params, Storage mode) {
	objects.emplace_back();
	objects.back().decode(params, mode);
}

void Scene::loadLight(const KeyValues & params, Storage) {
	const auto light = Light::decode(params);
	if(light) {
		lights.push_back(light);
	}
}

void Scene::loadCamera(const KeyValues & params, Storage) {
	_camera.decode(params);
}

void Scene::loadBackground(const KeyValues & params, Storage mode) {
	background = std::unique_ptr<Object>(new Object(Object::Type::Common, Resources::manager().getMesh("plane", mode), false));

	for(const auto & param : params.elements) {
		if(param.key == "color") {
			backgroundMode = Background::COLOR;
			// Background is a plane, store the color.
			backgroundColor = Codable::decodeVec3(param);

		} else if(param.key == "image" && !param.elements.empty()) {
			backgroundMode = Background::IMAGE;
			// Load image described as sub-element.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, mode);
			background->addTexture(tex);

		} else if(param.key == "cube" && !param.elements.empty()) {
			backgroundMode = Background::SKYBOX;
			// Object is a textured skybox.
			background = std::unique_ptr<Object>(new Object(Object::Type::Common, Resources::manager().getMesh("skybox", mode), false));
			background->decode(params, mode);
			// Load cubemap described as subelement.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, mode);
			background->addTexture(tex);

		} else if(param.key == "sun") {
			// In that case the background is a sky object.
			backgroundMode = Background::ATMOSPHERE;
			background	 = std::unique_ptr<Sky>(new Sky(mode));
			background->decode(params, mode);
			// Load the scattering table.
			const Texture * tex = Resources::manager().getTexture("scattering-precomputed", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, mode);
			background->addTexture(tex);
		}
	}
}

void Scene::loadScene(const KeyValues & params, Storage mode) {
	backgroundIrradiance = std::vector<glm::vec3>(9, glm::vec3(0.0f));

	for(const auto & param : params.elements) {
		if(param.key == "irradiance" && !param.values.empty()) {
			// Load the SH coefficients from the corresponding text file.
			const std::string coeffsRaw = Resources::manager().getString(param.values[0]);
			std::stringstream coeffsStream(coeffsRaw);
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			for(int i = 0; i < 9; ++i) {
				coeffsStream >> x >> y >> z;
				backgroundIrradiance[i] = glm::vec3(x, y, z);
			}

		} else if(param.key == "probe" && !param.elements.empty()) {
			// Load cubemap described as sub-element.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			backgroundReflection = Resources::manager().getTexture(texInfos.first, texInfos.second, mode);
		}
	}
	// Update matrix, there is at most one transformation in the scene object.
	_sceneModel = Codable::decodeTransformation(params.elements);
}

BoundingBox Scene::computeBoundingBox(bool onlyShadowCasters) {
	BoundingBox bbox;
	if(objects.empty()) {
		return bbox;
	}

	for(const auto & obj : objects) {
		if(onlyShadowCasters && !obj.castsShadow()) {
			continue;
		}
		bbox.merge(obj.boundingBox());
	}
	Log::Info() << Log::Resources << "Scene bounding box:" << std::endl
				<< "mini: " << bbox.minis << std::endl
				<< "maxi: " << bbox.maxis << "." << std::endl;
	return bbox;
}

void Scene::update(double fullTime, double frameTime) {
	for(auto & light : lights) {
		light->update(fullTime, frameTime);
	}
	for(auto & object : objects) {
		object.update(fullTime, frameTime);
	}
	background->update(fullTime, frameTime);
}

void Scene::clean() {
	for(auto & light : lights) {
		light->clean();
	}
}
