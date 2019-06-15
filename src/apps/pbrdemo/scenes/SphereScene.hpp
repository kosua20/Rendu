#ifndef SphereScene_h
#define SphereScene_h

#include "scene/Scene.hpp"


class SphereScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
};


void SphereScene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	
	const Descriptor rgbaTex(GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	const Descriptor srgbaTex(GL_SRGB8_ALPHA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	// Objects creation.
	Object sphere1(Object::Type::PBRRegular, Resources::manager().getMesh("sphere"), true);
	sphere1.addTexture(Resources::manager().getTexture("sphere_wood_lacquered_albedo", srgbaTex));
	sphere1.addTexture(Resources::manager().getTexture("sphere_wood_lacquered_normal", rgbaTex));
	sphere1.addTexture(Resources::manager().getTexture("sphere_wood_lacquered_rough_met_ao", rgbaTex));
	
	Object sphere2(Object::Type::PBRRegular, Resources::manager().getMesh("sphere"), true);
	sphere2.addTexture(Resources::manager().getTexture("sphere_gold_worn_albedo", srgbaTex));
	sphere2.addTexture(Resources::manager().getTexture("sphere_gold_worn_normal", rgbaTex));
	sphere2.addTexture(Resources::manager().getTexture("sphere_gold_worn_rough_met_ao", rgbaTex));
	
	const glm::mat4 model1 = glm::translate(glm::scale(glm::mat4(1.0f),glm::vec3(0.3f)), glm::vec3(1.2f,0.0f, 0.0f));
	const glm::mat4 model2 = glm::translate(glm::scale(glm::mat4(1.0f),glm::vec3(0.3f)), glm::vec3(-1.2f,0.0f, 0.0f));
	sphere1.update(model1);
	sphere2.update(model2);
	
	objects.push_back(sphere1);
	objects.push_back(sphere2);
	
	// Background creation.
	const TextureInfos * cubemapEnv = Resources::manager().getCubemap("studio", {GL_RGB32F, GL_LINEAR, GL_CLAMP_TO_EDGE});
	backgroundReflection = cubemapEnv->id;
	background = Object(Object::Type::Skybox, Resources::manager().getMesh("skybox"), false);
	background.addTexture(cubemapEnv);
	loadSphericalHarmonics("studio_shcoeffs");
	
	// Compute the bounding box of the shadow casters.
	const BoundingBox bbox = computeBoundingBox(true);
	
	// Create point lights.
	pointLights.emplace_back( glm::vec3(0.5f,-0.1f,0.5f), 6.0f*glm::vec3(0.2f,0.8f,1.2f), 0.9f, bbox);
	pointLights.emplace_back( glm::vec3(-0.5f,-0.1f,0.5f), 6.0f*glm::vec3(2.1f,0.3f,0.6f), 0.9f, bbox);
	
}

void SphereScene::update(double fullTime, double frameTime){
	const glm::mat4 model = glm::rotate(objects[0].model(), 0.1f*float(frameTime), glm::vec3(0.0f,1.0f,0.0f));
	objects[0].update(model);
	
}

#endif
