#include "scene/lights/DirectionalLight.hpp"
#include "graphics/GLUtilities.hpp"

DirectionalLight::DirectionalLight(const glm::vec3 & worldDirection, const glm::vec3 & color) :
	Light(color),
	_lightDirection(glm::normalize(worldDirection)) {
}

void DirectionalLight::init(const Texture * albedo, const Texture * normal, const Texture * depth, const Texture * effects) {
	// Setup the framebuffer.
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_shadowPass					= std::unique_ptr<Framebuffer>(new Framebuffer(512, 512, descriptor, true));
	_blur						= std::unique_ptr<BoxBlur>(new BoxBlur(512, 512, false, descriptor));

	_textures = {albedo, normal, depth, effects, _blur->textureId()};
	
	// Load the shaders
	_program	  = Resources::manager().getProgram2D("directional_light");
	_programDepth = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow");
}

void DirectionalLight::draw(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix, const glm::vec2 &) const {

	const glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector		= glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0));

	_program->use();
	_program->uniform("lightDirection", lightDirectionViewSpace);
	_program->uniform("lightColor", _color);
	// Projection parameter for position reconstruction.
	_program->uniform("projectionMatrix", projectionVector);
	_program->uniform("viewToLight", viewToLight);
	_program->uniform("castShadow", _castShadows);

	ScreenQuad::draw(_textures);
}

void DirectionalLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows) {
		return;
	}
	_shadowPass->bind();
	_shadowPass->setViewport();

	GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
	glEnable(GL_CULL_FACE);

	_programDepth->use();
	for(auto & object : objects) {
		if(!object.castsShadow()) {
			continue;
		}
		if(object.twoSided()) {
			glDisable(GL_CULL_FACE);
		}
		_programDepth->uniform("hasMask", object.masked());
		if(object.masked()) {
			GLUtilities::bindTexture(object.textures()[0], 0);
		}
		const glm::mat4 lightMVP = _mvp * object.model();
		_programDepth->uniform("mvp", lightMVP);
		GLUtilities::drawMesh(*(object.mesh()));
		glEnable(GL_CULL_FACE);
	}
	_shadowPass->unbind();

	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blur->process(_shadowPass->textureId());
	glEnable(GL_DEPTH_TEST);
}

void DirectionalLight::drawDebug(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) const {

	const Program * debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	const Mesh * debugMesh		 = Resources::manager().getMesh("light_arrow", Storage::GPU);

	const glm::mat4 vp		 = projectionMatrix * viewMatrix * glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
	const glm::vec3 colorLow = _color / (std::max)(_color[0], (std::max)(_color[1], _color[2]));

	debugProgram->use();
	debugProgram->uniform("mvp", vp);
	debugProgram->uniform("lightColor", colorLow);
	GLUtilities::drawMesh(*debugMesh);
}

void DirectionalLight::update(double fullTime, double frameTime) {
	glm::vec4 direction = glm::vec4(_lightDirection, 0.0f);
	for(auto & anim : _animations) {
		direction = anim->apply(direction, fullTime, frameTime);
	}
	_lightDirection = glm::normalize(glm::vec3(direction));
	setScene(_sceneBox);
}

void DirectionalLight::setScene(const BoundingBox & sceneBox) {
	_sceneBox						 = sceneBox;
	const BoundingSphere sceneSphere = _sceneBox.getSphere();
	const glm::vec3 lightPosition	= sceneSphere.center - sceneSphere.radius * 1.1f * _lightDirection;
	const glm::vec3 lightTarget		 = sceneSphere.center;
	_viewMatrix						 = glm::lookAt(lightPosition, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
	const float absz1				= abs(lightSpacebox.minis[2]);
	const float absz2				= abs(lightSpacebox.maxis[2]);
	const float near				= (std::min)(absz1, absz2);
	const float far					= (std::max)(absz1, absz2);
	const float scaleMargin			= 1.5f;
	_projectionMatrix				= glm::ortho(scaleMargin * lightSpacebox.minis[0], scaleMargin * lightSpacebox.maxis[0], scaleMargin * lightSpacebox.minis[1], scaleMargin * lightSpacebox.maxis[1], (1.0f / scaleMargin) * near, scaleMargin * far);
	_mvp							= _projectionMatrix * _viewMatrix;
}

void DirectionalLight::clean() const {
	if(_blur) {
		_blur->clean();
	}
	if(_shadowPass) {
		_shadowPass->clean();
	}
}

bool DirectionalLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {

	if(_castShadows && raycaster.intersectsAny(position, -_lightDirection)) {
		return false;
	}
	direction   = -_lightDirection;
	attenuation = 1.0f;
	return true;
}

void DirectionalLight::decode(const KeyValues & params) {
	Light::decodeBase(params);
	for(const auto & param : params.elements) {
		if(param.key == "direction") {
			_lightDirection = glm::normalize(Codable::decodeVec3(param));
		}
	}
}
