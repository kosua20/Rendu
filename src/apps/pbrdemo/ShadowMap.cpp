#include "ShadowMap.hpp"
#include "graphics/GLUtilities.hpp"

ShadowMap2D::ShadowMap2D(const std::shared_ptr<Light> & light, const glm::vec2 & resolution){
	_light = light;
	const Descriptor descriptor = {Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer(resolution.x, resolution.y, descriptor, true));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(resolution.x, resolution.y, false, descriptor));
	_program = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow");
	_light->registerShadowMap(_blur->textureId());
}

void ShadowMap2D::draw(const Scene & scene) const {
	if(!_light->castsShadow()){
		return;
	}
	_map->bind();
	_map->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	_program->use();
	
	for(auto & object : scene.objects) {
		if(!object.castsShadow()) {
			continue;
		}
		if(object.twoSided()) {
			glDisable(GL_CULL_FACE);
		}
		_program->uniform("hasMask", object.masked());
		if(object.masked()) {
			GLUtilities::bindTexture(object.textures()[0], 0);
		}
		const glm::mat4 lightMVP = _light->vp() * object.model();
		_program->uniform("mvp", lightMVP);
		GLUtilities::drawMesh(*(object.mesh()));
		glEnable(GL_CULL_FACE);
	}
	_map->unbind();
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blur->process(_map->textureId());
}

void ShadowMap2D::clean(){
	_blur->clean();
	_map->clean();
}

ShadowMapCube::ShadowMapCube(const std::shared_ptr<PointLight> & light, int side){
	_light = light;
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<FramebufferCube>(new FramebufferCube(side, descriptor, FramebufferCube::CubeMode::COMBINED, true));
	_program = Resources::manager().getProgram("object_layer_depth", "object_layer", "light_shadow_linear", "object_layer");
	_light->registerShadowMap(_map->textureId());
}

void ShadowMapCube::draw(const Scene & scene) const {
	if(!_light->castsShadow()){
		return;
	}
	
	static const char * uniformNames[6] = {"vps[0]", "vps[1]", "vps[2]", "vps[3]", "vps[4]", "vps[5]"};
	
	_map->bind();
	_map->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	
	_program->use();
	// Udpate the light vp matrices.
	const auto & faces = _light->vpFaces();
	for(size_t mid = 0; mid < 6; ++mid) {
		_program->uniform(uniformNames[mid], faces[mid]);
	}
	// Pass the world space light position, and the projection matrix far plane.
	_program->uniform("lightPositionWorld", _light->position());
	_program->uniform("lightFarPlane", _light->farPlane());
	
	for(auto & object : scene.objects) {
		if(!object.castsShadow()) {
			continue;
		}
		if(object.twoSided()) {
			glDisable(GL_CULL_FACE);
		}
		_program->uniform("hasMask", object.masked());
		if(object.masked()) {
			GLUtilities::bindTexture(object.textures()[0], 0);
		}
		_program->uniform("model", object.model());
		GLUtilities::drawMesh(*(object.mesh()));
		glEnable(GL_CULL_FACE);
	}
	
	_map->unbind();
	// No blurring pass for now.
	glDisable(GL_DEPTH_TEST);
}

void ShadowMapCube::clean(){
	_map->clean();
}
