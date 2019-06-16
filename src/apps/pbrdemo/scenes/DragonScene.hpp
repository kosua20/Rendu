#ifndef DragonScene_h
#define DragonScene_h

#include "scene/Scene.hpp"


class DragonScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
};


void DragonScene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	
	//Position fixed objects.
	const glm::mat4 dragonModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.1,-0.05,-0.25)),glm::vec3(0.5f));
	const glm::mat4 planeModel = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	const glm::mat4 suzanneModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.2,0.0,0.0)),glm::vec3(0.25f));
	
	const Descriptor rgbaTex(GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	const Descriptor srgbaTex(GL_SRGB8_ALPHA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	// Objects creation.
	Object suzanne(Object::Type::PBRRegular, Resources::manager().getMesh("suzanne"), true);
	suzanne.addTexture(Resources::manager().getTexture("suzanne_texture_color", srgbaTex));
	suzanne.addTexture(Resources::manager().getTexture("suzanne_texture_normal", rgbaTex));
	suzanne.addTexture(Resources::manager().getTexture("suzanne_texture_rough_met_ao", rgbaTex));
	auto rot = std::shared_ptr<Animation>(new Rotation(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, Animation::Frame::MODEL));
	suzanne.addAnimation(rot);
	Object dragon(Object::Type::PBRRegular, Resources::manager().getMesh("dragon"), true);
	dragon.addTexture(Resources::manager().getTexture("dragon_texture_color", srgbaTex));
	dragon.addTexture(Resources::manager().getTexture("dragon_texture_normal", rgbaTex));
	dragon.addTexture(Resources::manager().getTexture("dragon_texture_rough_met_ao", rgbaTex));
	
	Object plane(Object::Type::PBRParallax, Resources::manager().getMesh("groundplane"), false);
	plane.addTexture(Resources::manager().getTexture("groundplane_texture_color", srgbaTex));
	plane.addTexture(Resources::manager().getTexture("groundplane_texture_normal", rgbaTex));
	plane.addTexture(Resources::manager().getTexture("groundplane_texture_rough_met_ao", rgbaTex));
	plane.addTexture(Resources::manager().getTexture("groundplane_texture_depth", rgbaTex));
	
	objects.push_back(suzanne); objects[0].set(suzanneModel);
	objects.push_back(dragon); objects[1].set(dragonModel);
	objects.push_back(plane); objects[2].set(planeModel);
	
	// Background creation.
	const TextureInfos * cubemapEnv = Resources::manager().getCubemap("corsica_beach_cube", {GL_RGB32F, GL_LINEAR, GL_CLAMP_TO_EDGE});
	backgroundReflection = cubemapEnv;
	background = Object(Object::Type::Skybox, Resources::manager().getMesh("skybox"), false);
	background.addTexture(cubemapEnv);
	loadSphericalHarmonics("corsica_beach_cube_shcoeffs");
	
	// Compute the bounding box of the shadow casters.
	const BoundingBox bbox = computeBoundingBox(true);
	
	// Lights creation.
	// Create directional light.
	directionalLights.emplace_back(glm::vec3(-2.0f,-1.5f,0.0f), glm::vec3(1.0f,1.0f, 0.92f), bbox);
	directionalLights[0].castShadow(true);
	// This is a trick: the light is going to renormalize the direction vector, so moving along a vertical range is akin to doing a partial rotation around an horizontal axis.
	auto moveDir = std::shared_ptr<Animation>(new BackAndForth(glm::vec3(0.0f, 1.0f, 0.0f), 0.75f, 1.0f,Animation::Frame::WORLD));
	directionalLights[0].addAnimation(moveDir);
	// Create spotlight.
	spotLights.emplace_back(glm::vec3(1.1f,2.0f,1.1f), glm::vec3(-1.0f,-1.0f,-1.0f), glm::vec3(0.0f,10.0f,10.0f), 0.5f, 0.6f, 5.0f, bbox);
	spotLights[0].castShadow(true);
	auto trans = std::shared_ptr<Animation>(new BackAndForth(glm::vec3(1.0f, 0.0f, 1.0f), 0.5f, 2.0f, Animation::Frame::WORLD));
	spotLights[0].addAnimation(trans);
	
	// Create point lights.
	const float lI = 4.0; // Light intensity.
	const std::vector<glm::vec3> colors = { glm::vec3(lI,0.0,0.0), glm::vec3(0.0,lI,0.0), glm::vec3(0.0,0.0,lI), glm::vec3(lI,lI,0.0)};
	auto anim = std::shared_ptr<Animation>(new Rotation(glm::vec3(0.0f, 1.0f, 0.0f), 0.8f, Animation::Frame::WORLD));
	for(size_t i = 0; i < 4; ++i){
		const glm::vec3 position = glm::vec3(-1.0f+2.0f*(i%2),-0.1f,-1.0f+2.0f*(i/2));
		pointLights.emplace_back(position, colors[i], 1.2f, bbox);
		pointLights.back().addAnimation(anim);
	}
}


#endif
