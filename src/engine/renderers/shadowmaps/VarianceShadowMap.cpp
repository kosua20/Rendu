#include "VarianceShadowMap.hpp"
#include "scene/Scene.hpp"
#include "graphics/GLUtilities.hpp"

VarianceShadowMap2D::VarianceShadowMap2D(const std::shared_ptr<Light> & light, const glm::vec2 & resolution){
	_light = light;
	const Descriptor descriptor = {Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer(uint(resolution.x), uint(resolution.y), descriptor, true, "Shadow map 2D"));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(false, "Shadow maps 2D"));
	_program = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow_variance");
	_light->registerShadowMap(_map->texture());
}

void VarianceShadowMap2D::draw(const Scene & scene) const {
	if(!_light->castsShadow()){
		return;
	}

	GLUtilities::setCullState(true, Faces::BACK);
	GLUtilities::setBlendState(false);
	GLUtilities::setDepthState(true, TestFunction::LESS, true);

	_map->bind();
	_map->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);

	_program->use();

	const Frustum lightFrustum(_light->vp());

	for(auto & object : scene.objects) {
		if(!object.castsShadow()) {
			continue;
		}
		// Frustum culling.
		if(!lightFrustum.intersects(object.boundingBox())){
			continue;
		}
		GLUtilities::setCullState(!object.twoSided(), Faces::BACK);
		_program->uniform("hasMask", object.masked());
		if(object.masked()) {
			GLUtilities::bindTexture(object.textures()[0], 0);
		}
		const glm::mat4 lightMVP = _light->vp() * object.model();
		_program->uniform("mvp", lightMVP);
		GLUtilities::drawMesh(*(object.mesh()));
	}
	
	// Blur pass.
	_blur->process(_map->texture(), *_map);
}

VarianceShadowMapCube::VarianceShadowMapCube(const std::shared_ptr<PointLight> & light, int side){
	_light = light;
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer( TextureShape::Cube, side, side, 6, 1, {descriptor}, true, "Shadow map cube"));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(true, "Shadow maps cube"));
	_program = Resources::manager().getProgram("object_cube_depth", "object_basic_texture_worldpos", "light_shadow_linear_variance");
	_light->registerShadowMap(_map->texture());
}

void VarianceShadowMapCube::draw(const Scene & scene) const {
	if(!_light->castsShadow()){
		return;
	}

	GLUtilities::setDepthState(true, TestFunction::LESS, true);
	GLUtilities::setCullState(true, Faces::BACK);
	GLUtilities::setBlendState(false);

	// Udpate the light vp matrices.
	const auto & faces = _light->vpFaces();

	_map->setViewport();
	_program->use();
	// Pass the world space light position, and the projection matrix far plane.
	_program->uniform("lightPositionWorld", _light->position());
	_program->uniform("lightFarPlane", _light->farPlane());
	for(int i = 0; i < 6; ++i){
		// We render each face sequentially, culling objects that are not visible.
		_map->bind(i);
		GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
		const Frustum lightFrustum(faces[i]);

		for(auto & object : scene.objects) {
			if(!object.castsShadow()) {
				continue;
			}
			// Frustum culling.
			if(!lightFrustum.intersects(object.boundingBox())){
				continue;
			}
			GLUtilities::setCullState(!object.twoSided(), Faces::BACK);
			const glm::mat4 mvp = faces[i] * object.model();
			_program->uniform("mvp", mvp);
			_program->uniform("m", object.model());
			_program->uniform("hasMask", object.masked());
			if(object.masked()) {
				GLUtilities::bindTexture(object.textures()[0], 0);
			}
			GLUtilities::drawMesh(*(object.mesh()));
		}
	}
	// Blur pass.
	_blur->process(_map->texture(), *_map);
}
