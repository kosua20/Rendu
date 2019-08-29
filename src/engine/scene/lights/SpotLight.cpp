#include "scene/lights/SpotLight.hpp"
#include "graphics/GLUtilities.hpp"

SpotLight::SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, float innerAngle, float outerAngle, float radius) : Light(color),
	_lightDirection(glm::normalize(worldDirection)), _lightPosition(worldPosition), _innerHalfAngle(0.5f*innerAngle), _outerHalfAngle(0.5f*outerAngle), _radius(radius) {
}


void SpotLight::init(const std::vector<const Texture *>& textureIds){
	// Setup the framebuffer.
	const Descriptor descriptor = { Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_shadowPass = std::unique_ptr<Framebuffer>(new Framebuffer(512, 512, descriptor, true));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(512, 512, false, descriptor));
	
	_cone = Resources::manager().getMesh("light_cone", Storage::GPU);
	_textures = textureIds;
	_textures.push_back(_blur->textureId());
	
	// Load the shaders.
	_program = Resources::manager().getProgram("spot_light", "object_basic", "spot_light");
	_programDepth = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow");
	checkGLError();
}

void SpotLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0f));
	
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width = 2.0f*std::tan(_outerHalfAngle);
	const glm::mat4 modelMatrix = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius*glm::vec3(width,width,1.0f));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	
	glCullFace(GL_FRONT);
	_program->use();
	_program->uniform("mvp", mvp);
	_program->uniform("lightPosition", lightPositionViewSpace);
	_program->uniform("lightDirection", lightDirectionViewSpace);
	_program->uniform("lightColor", _color);
	_program->uniform("lightRadius", _radius);
	_program->uniform("innerAngleCos", std::cos(_innerHalfAngle));
	_program->uniform("outerAngleCos", std::cos(_outerHalfAngle));
	// Projection parameter for position reconstruction.
	_program->uniform("projectionMatrix", projectionVector);
	// Inverse screen size uniform.
	_program->uniform("inverseScreenSize", invScreenSize);
	_program->uniform("viewToLight", viewToLight);
	_program->uniform("castShadow", _castShadows);
	
	// Active screen texture.
	GLUtilities::bindTextures(_textures);
	
	// Select the geometry.
	GLUtilities::drawMesh(*_cone);
	
	glCullFace(GL_BACK);
}

void SpotLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows){
		return;
	}
	_shadowPass->bind();
	_shadowPass->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
	
	glEnable(GL_CULL_FACE);
	
	_programDepth->use();
	for(auto& object : objects){
		if(!object.castsShadow()){
			continue;
		}
		if(object.twoSided()){
			glDisable(GL_CULL_FACE);
		}
		_programDepth->uniform("hasMask", object.masked());
		if(object.masked()){
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


void SpotLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const Program * debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width = 2.0f*std::tan(_outerHalfAngle);
	const glm::mat4 modelMatrix = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius*glm::vec3(width,width,1.0f));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	debugProgram->use();
	debugProgram->uniform("mvp", mvp);
	debugProgram->uniform("lightColor", colorLow);
	
	GLUtilities::drawMesh(*_cone);
}

void SpotLight::update(double fullTime, double frameTime){
	glm::vec4 position = glm::vec4(_lightPosition, 1.0f);
	glm::vec4 direction = glm::vec4(_lightDirection, 0.0f);
	for(auto & anim : _animations){
		position = anim->apply(position, fullTime, frameTime);
		direction = anim->apply(direction, fullTime, frameTime);
	}
	_lightPosition = glm::vec3(position);
	_lightDirection = glm::normalize(glm::vec3(direction));
	setScene(_sceneBox);
}

void SpotLight::setScene(const BoundingBox & sceneBox){
	_sceneBox = sceneBox;
	_viewMatrix = glm::lookAt(_lightPosition, _lightPosition+_lightDirection, glm::vec3(0.0f,1.0f,0.0f));
	
	// Compute the projection matrix, automatically finding the near and far.
	float near;
	float far;
	if(_sceneBox.contains(_lightPosition)){
		const float size = glm::length(_sceneBox.getSize());
		near = 0.01f * size;
		far = 1.0f*size;
	} else {
		const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
		const float absz1 = abs(lightSpacebox.minis[2]);
		const float absz2 = abs(lightSpacebox.maxis[2]);
		near = (std::min)(absz1, absz2);
		far = (std::max)(absz1, absz2);
	}
	_projectionMatrix = glm::perspective(2.0f*_outerHalfAngle, 1.0f, near, far);
	_mvp = _projectionMatrix * _viewMatrix;
}

bool SpotLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
	if(_castShadows && !raycaster.visible(position, _lightPosition)){
		return false;
	}
	direction = _lightPosition - position;
	
	// Early exit if we are outside the sphere of influence.
	const float localRadius = glm::length(direction);
	if(localRadius > _radius){
		return false;
	}
	if(localRadius > 0.0f){
		direction /= localRadius;
	}
	
	// Compute the angle between the light direction and the (light, surface point) vector.
	const float currentCos = glm::dot(-direction, _lightDirection);
	const float outerCos = std::cos(_outerHalfAngle);
	// If we are outside the spotlight cone, no lighting.
	if(currentCos < std::cos(outerCos)){
		return false;
	}
	// Compute the spotlight attenuation factor based on our angle compared to the inner and outer spotlight angles.
	const float innerCos = std::cos(_innerHalfAngle);
	const float angleAttenuation = glm::clamp((currentCos - outerCos)/(innerCos - outerCos), 0.0f, 1.0f);
	
	// Attenuation with increasing distance to the light.
	const float radiusRatio = localRadius / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation = angleAttenuation * attenNum * attenNum;
	return true;
}

void SpotLight::decode(const KeyValues & params){
	Light::decodeBase(params);
	for(const auto & param : params.elements){
		if(param.key == "direction"){
			_lightDirection = glm::normalize(Codable::decodeVec3(param));
			
		} else if(param.key == "position"){
			_lightPosition = Codable::decodeVec3(param);
			
		} else if(param.key == "cone" && param.values.size() >= 2){
			const float innerAngle = std::stof(param.values[0]);
			const float outerAngle = std::stof(param.values[1]);
			_innerHalfAngle = 0.5f*innerAngle;
			_outerHalfAngle = 0.5f*outerAngle;
			
		} else if(param.key == "radius" && !param.values.empty()){
			_radius = std::stof(param.values[0]);
			
		}
	}
}


void SpotLight::clean() const {
	if(_blur){
		_blur->clean();
	}
	if(_shadowPass){
		_shadowPass->clean();
	}
}

