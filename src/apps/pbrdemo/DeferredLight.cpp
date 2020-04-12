#include "DeferredLight.hpp"
#include "graphics/GLUtilities.hpp"

DeferredLight::DeferredLight(const Texture * texAlbedo, const Texture * texNormals, const Texture * texDepth, const Texture * texEffects){
	_textures = {texAlbedo, texNormals, texDepth, texEffects};
	_sphere  = Resources::manager().getMesh("light_sphere", Storage::GPU);
	_cone  = Resources::manager().getMesh("light_cone", Storage::GPU);
	
	_pointProgram = Resources::manager().getProgram("point_light_pbr", "object_basic", "point_light_pbr");
	_spotProgram = Resources::manager().getProgram("spot_light_pbr", "object_basic", "spot_light_pbr");
	_dirProgram = Resources::manager().getProgram2D("directional_light_pbr");
}

void DeferredLight::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix){
	_view = viewMatrix;
	_proj = projMatrix;
}

void DeferredLight::updateShadowMapInfos(ShadowMode mode, float bias){
	_shadowBias = bias;
	_shadowMode = mode;
}

void DeferredLight::draw(const SpotLight * light) {
	
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightPositionViewSpace  = glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0f));
	const glm::mat4 mvp			= _proj * _view * light->model();
	const glm::mat4 viewToLight = light->vp() * glm::inverse(_view);
	
	glCullFace(GL_FRONT);
	_spotProgram->use();
	_spotProgram->uniform("mvp", mvp);
	_spotProgram->uniform("lightPosition", lightPositionViewSpace);
	_spotProgram->uniform("lightDirection", lightDirectionViewSpace);
	_spotProgram->uniform("lightColor", light->intensity());
	_spotProgram->uniform("lightRadius", light->radius());
	_spotProgram->uniform("intOutAnglesCos", glm::cos(light->angles()));
	_spotProgram->uniform("projectionMatrix", projectionVector);
	_spotProgram->uniform("viewToLight", viewToLight);
	
	// Active screen texture.
	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		const auto & shadowInfos = light->shadowMap();
		GLUtilities::bindTexture(shadowInfos.map, _textures.size());
		_spotProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_spotProgram->uniform("shadowBias", _shadowBias);
		_spotProgram->uniform("shadowMode", int(_shadowMode));
	} else {
		_spotProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_cone);
	
	glCullFace(GL_BACK);
}

void DeferredLight::draw(const PointLight * light) {
	
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::mat4 mvp		 = _proj * _view * light->model();
	const glm::mat3 viewToLight = glm::mat3(glm::inverse(_view));
	
	glCullFace(GL_FRONT);
	_pointProgram->use();
	_pointProgram->uniform("mvp", mvp);
	_pointProgram->uniform("lightPosition", lightPositionViewSpace);
	_pointProgram->uniform("lightColor", light->intensity());
	_pointProgram->uniform("lightRadius", light->radius());
	_pointProgram->uniform("projectionMatrix", projectionVector);
	_pointProgram->uniform("viewToLight", viewToLight);
	_pointProgram->uniform("lightFarPlane", light->farPlane());
	
	 // Active screen texture.
	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		const auto & shadowInfos = light->shadowMap();
		GLUtilities::bindTexture(shadowInfos.map, _textures.size());
		_pointProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_pointProgram->uniform("shadowBias", _shadowBias);
		_pointProgram->uniform("shadowMode", int(_shadowMode));
	} else {
		_pointProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_sphere);
	glCullFace(GL_BACK);
}

void DeferredLight::draw(const DirectionalLight * light) {
	const glm::mat4 viewToLight = light->vp() * glm::inverse(_view);
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0));
	
	_dirProgram->use();
	_dirProgram->uniform("lightDirection", lightDirectionViewSpace);
	_dirProgram->uniform("lightColor", light->intensity());
	_dirProgram->uniform("projectionMatrix", projectionVector);
	_dirProgram->uniform("viewToLight", viewToLight);

	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		const auto & shadowInfos = light->shadowMap();
		GLUtilities::bindTexture(shadowInfos.map, _textures.size());
		_dirProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_dirProgram->uniform("shadowBias", _shadowBias);
		_dirProgram->uniform("shadowMode", int(_shadowMode));
	} else {
		_dirProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	ScreenQuad::draw();
	
}
