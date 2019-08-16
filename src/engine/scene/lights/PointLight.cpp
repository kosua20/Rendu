#include "scene/lights/PointLight.hpp"
#include "graphics/GLUtilities.hpp"

PointLight::PointLight(): Light() {
	_radius = 1.0f;
	_lightPosition = glm::vec3(0.0f);
}

PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius) : Light(color) {
	_radius = radius;
	_lightPosition = worldPosition;
}

void PointLight::init(const std::vector<const Texture *>& textureIds){
	_program = Resources::manager().getProgram("point_light", "object_basic", "point_light");
	_sphere = Resources::manager().getMesh("light_sphere", Storage::GPU);
	// Setup the framebuffer.
	/// \todo Enable only if the light is a shadow caster, to avoid wasteful allocation.
	const Descriptor descriptor = { Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_shadowFramebuffer = std::unique_ptr<FramebufferCube>(new FramebufferCube(512, descriptor, true));
	
	_textures = textureIds;
	_textures.push_back(_shadowFramebuffer->textureId());
	// Load the shaders
	_programDepth = Resources::manager().getProgram("object_layer_depth", "object_layer", "light_shadow_linear", "object_layer");
	checkGLError();
}


void PointLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::mat3 viewToLight = glm::mat3(glm::inverse(viewMatrix));
	
	glCullFace(GL_FRONT);
	_program->use();
	_program->uniform("mvp", mvp);
	_program->uniform("lightPosition", lightPositionViewSpace);
	_program->uniform("lightColor", _color);
	_program->uniform("lightRadius", _radius);
	// Projection parameter for position reconstruction.
	_program->uniform("projectionMatrix", projectionVector);
	// Inverse screen size uniform.
	_program->uniform("inverseScreenSize", invScreenSize);
	_program->uniform("viewToLight", viewToLight);
	_program->uniform("lightFarPlane", _farPlane);
	_program->uniform("castShadow", _castShadows);
	
	// Active screen texture.
	for(GLuint i = 0;i < _textures.size()-1; ++i){
		GLUtilities::bindTexture(_textures[i], i);
	}
	// Activate the shadow cubemap.
	if(_castShadows){
		GLUtilities::bindTexture(_textures.back(), uint(int(_textures.size())-1));
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_sphere);
	
	glCullFace(GL_BACK);
}

void PointLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows){
		return;
	}
	
	static const char* uniformNames[6] = {"vps[0]", "vps[1]", "vps[2]", "vps[3]", "vps[4]", "vps[5]"};
	
	_shadowFramebuffer->bind();
	_shadowFramebuffer->setViewport();
	glClearColor(1.0f,1.0f,1.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	
	_programDepth->use();
	// Udpate the light mvp matrices.
	for(size_t mid = 0; mid < 6; ++mid){
		_programDepth->uniform(uniformNames[mid], _mvps[mid]);
	}
	// Pass the world space light position, and the projection matrix far plane.
	_programDepth->uniform("lightPositionWorld", _lightPosition);
	_programDepth->uniform("lightFarPlane", _farPlane);
	
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
		_programDepth->uniform("model", object.model());
		GLUtilities::drawMesh(*(object.mesh()));
		glEnable(GL_CULL_FACE);
	}
	
	_shadowFramebuffer->unbind();
	
	// No blurring pass for now.
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const Program * debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	debugProgram->use();
	debugProgram->uniform("mvp", mvp);
	debugProgram->uniform("lightColor", colorLow);
	GLUtilities::drawMesh(*_sphere);
	
	const glm::mat4 modelMatrix1 = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(0.02f*_radius));
	const glm::mat4 mvp1 = projectionMatrix * viewMatrix * modelMatrix1;
	debugProgram->uniform("mvp", mvp1);
	debugProgram->uniform("lightColor", _color);
	GLUtilities::drawMesh(*_sphere);

}

void PointLight::update(double fullTime, double frameTime){
	glm::vec4 position = glm::vec4(_lightPosition, 0.0);
	for(auto & anim : _animations){
		position = anim->apply(position, fullTime, frameTime);
	}
	_lightPosition = glm::vec3(position);
	setScene(_sceneBox);
}

void PointLight::setScene(const BoundingBox & sceneBox){
	_sceneBox = sceneBox;
	
	const glm::mat4 model = glm::translate(glm::mat4(1.0f), -_lightPosition);
	
	// Compute the projection matrix based on the scene bounding box.
	// As both the view matrices and the bounding boxe are axis aligned, we can avoid costly transformations.
	const glm::vec3 deltaMini = _lightPosition - _sceneBox.minis;
	const glm::vec3 deltaMaxi =  _lightPosition - _sceneBox.maxis;
	// Absolute value of each min/max  distance on each axis.
	const glm::vec3 candidatesNear = glm::min(glm::abs(deltaMini), glm::abs(deltaMaxi));
	const glm::vec3 candidatesFar = glm::max(glm::abs(deltaMini), glm::abs(deltaMaxi));
	
	const float size = glm::length(_sceneBox.getSize());
	float far = candidatesFar[0];
	float near = candidatesNear[0];
	bool allInside = true;
	for(int i = 0; i < 3; ++i){
		// The light is inside the bbox along the axis i if the two delta have different signs.
		const bool isInside = (std::signbit(deltaMini[i]) != std::signbit(deltaMaxi[i]));
		allInside = allInside && isInside;
		// In this case we enforce a small near.
		near = (std::min)(near, candidatesNear[i]);
		far = (std::max)(far, candidatesFar[i]);
	}
	if(allInside){
		near = 0.01f*size;
		far = size;
	}
	_farPlane = far;
	const glm::mat4 projection = glm::perspective(float(M_PI/2.0), 1.0f, near, _farPlane);
	
	// Create the constant view matrices for the 6 faces.
	const glm::vec3 ups[6] = { glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0) };
	const glm::vec3 centers[6] = { glm::vec3(1.0,0.0,0.0), glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,-1.0,0.0) , glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0)};
	
	// Udpate the mvps.
	_mvps.resize(6);
	for(size_t mid = 0; mid < 6; ++mid){
		const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), centers[mid], ups[mid]);
		_mvps[mid] = projection * view * model;
	}
}

bool PointLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
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
	
	// Attenuation with increasing distance to the light.
	const float radiusRatio = localRadius / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation = attenNum*attenNum;
	return true;
}

void PointLight::decode(const KeyValues & params){
	Light::decodeBase(params);
	for(const auto & param : params.elements){
		if(param.key == "position"){
			_lightPosition = Codable::decodeVec3(param);
		} else if(param.key == "radius" && !param.values.empty()){
			_radius = std::stof(param.values[0]);
		}
	}
}

void PointLight::clean() const {
	if(_shadowFramebuffer){
		_shadowFramebuffer->clean();
	}
}
