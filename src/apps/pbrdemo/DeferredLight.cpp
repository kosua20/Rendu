#include "DeferredLight.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"

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

void DeferredLight::draw(const SpotLight * light) {
	
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightPositionViewSpace  = glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0f));
	const glm::mat4 mvp			= _proj * _view * light->model();
	const glm::mat4 viewToLight = light->vp() * glm::inverse(_view);

	GPU::setDepthState(false);
	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
	GPU::setCullState(true, Faces::FRONT);

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
	_spotProgram->textures(_textures);

	const auto & shadowInfos = light->shadowMap();
	if(light->castsShadow() && (shadowInfos.map != nullptr)) {
		_spotProgram->texture(shadowInfos.map, uint(_textures.size()));
		_spotProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_spotProgram->uniform("shadowBias", shadowInfos.bias);
		_spotProgram->uniform("shadowMode", int(shadowInfos.mode));
	} else {
		_spotProgram->defaultTexture(uint(_textures.size()));
		_spotProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	// Select the geometry.
	GPU::drawMesh(*_cone);
}

void DeferredLight::draw(const PointLight * light) {
	
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::mat4 mvp		 = _proj * _view * light->model();
	const glm::mat3 viewToLight = glm::mat3(glm::inverse(_view));

	GPU::setDepthState(false);
	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
	GPU::setCullState(true, Faces::FRONT);

	_pointProgram->use();
	_pointProgram->uniform("mvp", mvp);
	_pointProgram->uniform("lightPosition", lightPositionViewSpace);
	_pointProgram->uniform("lightColor", light->intensity());
	_pointProgram->uniform("lightRadius", light->radius());
	_pointProgram->uniform("projectionMatrix", projectionVector);
	_pointProgram->uniform("viewToLight", glm::mat4(viewToLight));
	_pointProgram->uniform("lightFarPlane", light->farPlane());
	
	 // Active screen texture.
	_pointProgram->textures(_textures);

	const auto & shadowInfos = light->shadowMap();
	if(light->castsShadow() && (shadowInfos.map != nullptr)) {
		_pointProgram->texture(shadowInfos.map, uint(_textures.size()));
		_pointProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_pointProgram->uniform("shadowBias", shadowInfos.bias);
		_pointProgram->uniform("shadowMode", int(shadowInfos.mode));
	} else {
		_pointProgram->defaultTexture(uint(_textures.size()));
		_pointProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	// Select the geometry.
	GPU::drawMesh(*_sphere);
}

void DeferredLight::draw(const DirectionalLight * light) {
	const glm::mat4 viewToLight = light->vp() * glm::inverse(_view);
	// Projection parameter for position reconstruction.
	const glm::vec4 projectionVector = glm::vec4(_proj[0][0], _proj[1][1], _proj[2][2], _proj[3][2]);
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0));

	GPU::setDepthState(false);
	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
	GPU::setCullState(true, Faces::BACK);

	_dirProgram->use();
	_dirProgram->uniform("lightDirection", lightDirectionViewSpace);
	_dirProgram->uniform("lightColor", light->intensity());
	_dirProgram->uniform("projectionMatrix", projectionVector);
	_dirProgram->uniform("viewToLight", viewToLight);

	_dirProgram->textures(_textures);

	const auto & shadowInfos = light->shadowMap();
	if(light->castsShadow() && (shadowInfos.map != nullptr)) {
		_dirProgram->texture(shadowInfos.map, uint(_textures.size()));
		_dirProgram->uniform("shadowLayer", int(shadowInfos.layer));
		_dirProgram->uniform("shadowBias", shadowInfos.bias);
		_dirProgram->uniform("shadowMode", int(shadowInfos.mode));
	} else {
		_dirProgram->defaultTexture(uint(_textures.size()));
		_dirProgram->uniform("shadowMode", int(ShadowMode::NONE));
	}
	ScreenQuad::draw();
	
}

DeferredProbe::DeferredProbe(const Texture * texAlbedo, const Texture * texNormals, const Texture * texEffects, const Texture * texDepth, const Texture * texSSAO){

	_box  = Resources::manager().getMesh("cube", Storage::GPU);
	_program = Resources::manager().getProgram("probe_pbr", "object_basic", "probe_pbr");

	// Load texture.
	const Texture * textureBrdf = Resources::manager().getTexture("brdf-precomputed", Layout::RGBA16F, Storage::GPU);

	// Ambient pass: needs the albedo, the normals, the depth, the effects, the AO result, the BRDF table and the  envmap.
	_textures.resize(6);
	_textures[0] = texAlbedo;
	_textures[1] = texNormals;
	_textures[2] = texEffects;
	_textures[3] = texDepth;
	_textures[4] = texSSAO;
	_textures[5] = textureBrdf;
}

void DeferredProbe::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix){
	_viewProj = projMatrix * viewMatrix;
	_invView = glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	_projectionVector = glm::vec4(projMatrix[0][0], projMatrix[1][1], projMatrix[2][2], projMatrix[3][2]);
}

void DeferredProbe::draw(const LightProbe & probe) {
	// Place the probe.
	glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), probe.position()), probe.rotation(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, probe.size() + probe.fade());
	const glm::mat4 mvp = _viewProj * model;

	GPU::setDepthState(false);
	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
	GPU::setCullState(true, Faces::FRONT);

	const Texture* envmap = probe.map();

	_program->use();
	_program->uniform("mvp", mvp);
	_program->uniform("inverseV", _invView);
	_program->uniform("projectionMatrix", _projectionVector);
	_program->uniform("maxLod", float(envmap->levels-1));
	_program->uniform("cubemapPos", probe.position());
	_program->uniform("cubemapCenter", probe.center());
	_program->uniform("cubemapExtent", probe.extent());
	_program->uniform("cubemapSize", probe.size());
	_program->uniform("cubemapFade", probe.fade());
	_program->uniform("cubemapCosSin", probe.rotationCosSin());
	
	_program->buffer(*probe.shCoeffs(), 0);

	_program->textures(_textures);
	_program->texture(*envmap, _textures.size());

	GPU::drawMesh(*_box);
}
