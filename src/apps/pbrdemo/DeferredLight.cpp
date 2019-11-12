#include "DeferredLight.hpp"
#include "graphics/GLUtilities.hpp"

DeferredLight::DeferredLight(const Texture * texAlbedo, const Texture * texNormals, const Texture * texDepth, const Texture * texEffects){
	_textures = {texAlbedo, texNormals, texDepth, texEffects};
	_sphere  = Resources::manager().getMesh("light_sphere", Storage::GPU);
	_cone  = Resources::manager().getMesh("light_cone", Storage::GPU);
	
	_pointProgram = Resources::manager().getProgram("point_light", "object_basic", "point_light");
	_spotProgram = Resources::manager().getProgram("spot_light", "object_basic", "spot_light");
	_dirProgram = Resources::manager().getProgram2D("directional_light");
}

void DeferredLight::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix){
	_view = viewMatrix;
	_proj = projMatrix;
}

void DeferredLight::draw(const SpotLight * light) const {
	
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
	_spotProgram->uniform("castShadow", light->castsShadow());
	
	// Active screen texture.
	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		GLUtilities::bindTexture(light->shadowMap(), _textures.size());
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_cone);
	
	glCullFace(GL_BACK);
}

void DeferredLight::draw(const PointLight * light) const {
	
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
	_pointProgram->uniform("castShadow", light->castsShadow());
	
	 // Active screen texture.
	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		GLUtilities::bindTexture(light->shadowMap(), _textures.size());
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_sphere);
	glCullFace(GL_BACK);
}

void DeferredLight::draw(const DirectionalLight * light) const {
	const glm::mat4 viewToLight = light->vp() * glm::inverse(_view);
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0));
	
	_dirProgram->use();
	_dirProgram->uniform("lightDirection", lightDirectionViewSpace);
	_dirProgram->uniform("lightColor", light->intensity());
	_dirProgram->uniform("projectionMatrix", projectionVector);
	_dirProgram->uniform("viewToLight", viewToLight);
	_dirProgram->uniform("castShadow", light->castsShadow());
	GLUtilities::bindTextures(_textures);
	if(light->castsShadow()) {
		GLUtilities::bindTexture(light->shadowMap(), _textures.size());
	}
	ScreenQuad::draw();
	
}
