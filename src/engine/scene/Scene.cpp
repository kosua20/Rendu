#include "scene/Scene.hpp"
#include "scene/Sky.hpp"
#include "system/TextUtilities.hpp"
#include "system/Query.hpp"

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

bool Scene::init(Storage options) {
	if(_loaded) {
		return true;
	}

	Query timer;
	timer.begin();

	// Define loaders for each keyword.
	std::unordered_map<std::string, bool (Scene::*)(const KeyValues &, Storage)> loaders = {
		{"scene", &Scene::loadScene}, {"object", &Scene::loadObject}, {"material", &Scene::loadMaterial}, {"point", &Scene::loadLight}, {"directional", &Scene::loadLight}, {"spot", &Scene::loadLight}, {"camera", &Scene::loadCamera}, {"background", &Scene::loadBackground}, {"probe", &Scene::loadProbe}};

	// Parse the file.
	const std::string sceneFile			   = Resources::manager().getString(_name);
	if(sceneFile.empty()) {
		return false;
	}

	bool success = true;
	const std::vector<KeyValues> allParams = Codable::decode(sceneFile);

	// Process each group of keyvalues.
	for(const auto & element : allParams) {
		const std::string key = element.key;
		// By construction (see above), all keys should have a loader.
		const bool elemSuccess = (this->*loaders[key])(element, options);
		success = success && elemSuccess;
	}

	// Update all objects poses.
	for(Object & object : objects) {
		const glm::mat4 newModel = _sceneModel * object.model();
		object.set(newModel);
	}
	// Update all objects materials.
	for(Object& object : objects){
		bool foundMaterial = false;
		for(const Material& material : materials){
			if(material.name() == object.materialName()){
				object.setMaterial(&material);
				foundMaterial = true;
				break;
			}
		}
		if(!foundMaterial){
			Log::Error() << Log::Resources << "Missing material " << object.materialName() << "." << std::endl;
			return false;
		}
	}
	
	// The scene model matrix has been applied to all objects, we can reset it.
	_sceneModel = glm::mat4(1.0f);
	BoundingBox bboxCasters;
	computeBoundingBoxes(_bbox, bboxCasters);
	// Update all lights bounding box infos.
	for(auto & light : lights) {
		light->setScene(bboxCasters);
	}

	// Check if the environment probes has been setup.
	if(probes.empty()){
		probes.emplace_back();
		KeyValues defaultKv("probe");
		probes.back().decode(defaultKv, options);
	}
	// Assign a size to probes with no specified size, ensuring they cover the whole scene.
	for(LightProbe& probe : probes){
		probe.updateSize(_bbox);
	}
	
	// Sort objects by material.
	std::sort(objects.begin(), objects.end(), [](const Object & a, const Object & b){
		return int(a.material().type()) < int(b.material().type());
	});

	// Check if the scene is static.
	for(const auto & obj : objects){
		if(obj.animated()){
			_animated = true;
		}
	}
	// Check if there is a material transparent in the scene.
	for(const auto& material : materials){
		if((material.type() == Material::Type::Transparent) || (material.type() == Material::Type::TransparentIrid)){
			_transparent = true;
			break;
		}
	}

	for(const auto & light : lights){
		if(light->animated()){
			_animated = true;
			break;
		}
	}

	timer.end();
	_loaded = success;
	Log::Info() << Log::Resources << (_loaded ? "Loading took " : "Loading failed after ") << (float(timer.value())/1000000.0f) << "ms." << std::endl;
	return _loaded;
}

bool Scene::loadObject(const KeyValues & params, Storage options) {
	objects.emplace_back();
	return objects.back().decode(params, options);
}

bool Scene::loadMaterial(const KeyValues & params, Storage options) {
	materials.emplace_back();
	return materials.back().decode(params, options);
}

bool Scene::loadLight(const KeyValues & params, Storage) {
	const std::shared_ptr<Light> light = Light::decode(params);
	if(light != nullptr) {
		lights.push_back(light);
		return true;
	}
	return false;
}

bool Scene::loadCamera(const KeyValues & params, Storage) {
	return _camera.decode(params);
}

bool Scene::loadBackground(const KeyValues & params, Storage options) {
	background = std::unique_ptr<Object>(new Object(Resources::manager().getMesh("plane", options), false));
	_backgroundMaterial = Material(Material::Type::None);

	bool success = true;
	for(const auto & param : params.elements) {
		if(param.key == "color") {
			// Background is a plane, store the color.
			backgroundMode = Background::COLOR;
			const glm::vec3 color = Codable::decodeVec3(param);
			_backgroundMaterial.addParameter(glm::vec4(color, 1.0f));

		} else if(param.key == "image" && !param.elements.empty()) {
			// Load image described as sub-element.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
			success = tex != nullptr;
			if(success){
				backgroundMode = Background::IMAGE;
				_backgroundMaterial.addTexture(tex);
			}

		} else if(param.key == "cube" && !param.elements.empty()) {
			// Object is a textured skybox.
			// Load cubemap described as subelement.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
			success = tex != nullptr;
			if(success){
				_backgroundMaterial.addTexture(tex);
				backgroundMode = Background::SKYBOX;
				background = std::unique_ptr<Object>(new Object(Resources::manager().getMesh("skybox", options), false));
				success = background->decode(params, options);
			}

		} else if(param.key == "sun") {
			// In that case the background is a sky object.
			// Load the scattering table.
			const Texture * tex = Resources::manager().getTexture("scattering-precomputed", Layout::RGBA16F, options);
			success = tex != nullptr;
			if(success){
				_backgroundMaterial.addTexture(tex);
				backgroundMode = Background::ATMOSPHERE;
				background	 = std::unique_ptr<Sky>(new Sky(options));
				success = background->decode(params, options);
			}

		}
	}
	if(!success){
		Log::Error() << Log::Resources << "Unable to load background." << std::endl;
	}

	background->setMaterial(&_backgroundMaterial);
	return success;
}

bool Scene::loadProbe(const KeyValues & params, Storage options) {
	probes.emplace_back();
	return probes.back().decode(params, options);
}

bool Scene::loadScene(const KeyValues & params, Storage) {
	// Update matrix, there is at most one transformation in the scene object.
	_sceneModel = Codable::decodeTransformation(params.elements);
	return true;
}

std::vector<KeyValues> Scene::encode() const {
	std::vector<KeyValues> tokens;
	
	// Encode the scene transformation.
	if(_sceneModel != glm::mat4(1.0f)){
		tokens.emplace_back("scene");
		auto & scnNode = tokens.back();

		// Encode the scene transformation.
		const auto modelKeys = Codable::encode(_sceneModel);
		for(const auto & key : modelKeys){
			scnNode.elements.push_back(key);
		}
	}

	// Encode the environment probes.
	for(const LightProbe& probe : probes){
		tokens.push_back(probe.encode());
	}
	
	// Encode the background.
	KeyValues bgNode("background");
	
	switch (backgroundMode) {
		case Background::COLOR:
		{
			bgNode.elements.emplace_back("color");
			const glm::vec4 color = background->material().parameters()[0];
			bgNode.elements.back().values = Codable::encode(glm::vec3(color));
			break;
		}
		case Background::IMAGE:
			bgNode.elements.emplace_back("image");
			bgNode.elements.back().elements = { Codable::encode(background->material().textures()[0]) };
			break;
		case Background::SKYBOX:
			bgNode.elements.emplace_back("cube");
			bgNode.elements.back().elements = { Codable::encode(background->material().textures()[0]) };
		break;
		case Background::ATMOSPHERE:
			bgNode = background->encode();
			bgNode.key = "background";
		break;
		default:
			break;
	}
	tokens.push_back(bgNode);
	// Encode the objects
	for(const auto & obj : objects){
		tokens.push_back(obj.encode());
	}
	// Encode the materials
	for(const auto & mat : materials){
		tokens.push_back(mat.encode());
	}
	// Encode the lights
	for(const auto & light : lights){
		tokens.push_back(light->encode());
	}
	tokens.push_back(_camera.encode());
	return tokens;
}

void Scene::computeBoundingBoxes(BoundingBox & globalBox, BoundingBox & casterBox) {
	globalBox = BoundingBox();
	casterBox = BoundingBox();

	if(objects.empty()) {
		return;
	}

	for(Object & obj : objects) {
		const BoundingBox & objBox = obj.boundingBox();
		globalBox.merge(objBox);

		if(obj.castsShadow()) {
			casterBox.merge(objBox);
		}
	}

	Log::Info() << Log::Resources << "Scene bounding box:" << std::endl
				<< "\t\tmini: " << globalBox.minis << std::endl
				<< "\t\tmaxi: " << globalBox.maxis << "." << std::endl;
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
