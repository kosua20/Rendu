#ifndef DeskScene_h
#define DeskScene_h

#include "scene/Scene.hpp"


class DeskScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
	
};


void DeskScene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	
	const Descriptor rgbaTex(GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	const Descriptor srgbaTex(GL_SRGB8_ALPHA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	glm::mat4 sceneMatrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)), glm::vec3(0.0f,0.0f,-2.0f));
	
	const std::vector<std::string> objectNames = {"candle", "desk", "hammer", "lighter", "rock", "screwdriver", "spyglass"};
	for(const auto& name : objectNames){
		Object obj(Object::Type::PBRRegular, Resources::manager().getMesh(name), true);
		obj.addTexture(Resources::manager().getTexture(name + "_albedo", srgbaTex));
		obj.addTexture(Resources::manager().getTexture(name + "_normal", rgbaTex));
		obj.addTexture(Resources::manager().getTexture(name + "_rough_met_ao", rgbaTex));
		obj.set(sceneMatrix);
		objects.push_back(obj);
	}
	
	// Background creation.
	const TextureInfos * cubemapEnv = Resources::manager().getCubemap("small_apartment", {GL_RGB32F, GL_LINEAR, GL_CLAMP_TO_EDGE});
	backgroundReflection = cubemapEnv;
	background = Object(Object::Type::Skybox, Resources::manager().getMesh("skybox"), false);
	background.addTexture(cubemapEnv);
	loadSphericalHarmonics("small_apartment_shcoeffs");
	
	// Compute the bounding box of the shadow casters.
	const BoundingBox bbox = computeBoundingBox(true);
	
	// Create point candle light.
	const glm::vec3 candleLightPosition = glm::vec3(0.09f,0.52f,-0.36f);
	pointLights.emplace_back(candleLightPosition, glm::vec3(3.0f, 2.0f, 0.2f), 2.5f, bbox);
	pointLights[0].castShadow(true);
}


#endif
