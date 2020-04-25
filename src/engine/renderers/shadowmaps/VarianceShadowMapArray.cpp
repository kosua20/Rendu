#include "VarianceShadowMapArray.hpp"
#include "scene/Scene.hpp"
#include "graphics/GLUtilities.hpp"

VarianceShadowMap2DArray::VarianceShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer(TextureShape::Array2D, uint(resolution.x), uint(resolution.y), uint(lights.size()), 1, {descriptor}, true));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(false));
	_program = Resources::manager().getProgram("object_depth", "light_shadow_vertex", "light_shadow_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_map->texture(), lid);
	}
}

void VarianceShadowMap2DArray::draw(const Scene & scene) const {

	GLUtilities::setCullState(true);
	GLUtilities::setDepthState(true);
	_map->setViewport();
	_program->use();

	for(size_t lid = 0; lid < _lights.size(); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			continue;
		}
		_map->bind(lid);
		GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);

		const Frustum lightFrustum(light->vp());

		for(auto & object : scene.objects) {
			if(!object.castsShadow()) {
				continue;
			}
			// Frustum culling.
			if(!lightFrustum.intersects(object.boundingBox())){
				continue;
			}
			if(object.twoSided()) {
				GLUtilities::setCullState(false);
			}
			_program->uniform("hasMask", object.masked());
			if(object.masked()) {
				GLUtilities::bindTexture(object.textures()[0], 0);
			}
			const glm::mat4 lightMVP = light->vp() * object.model();
			_program->uniform("mvp", lightMVP);
			GLUtilities::drawMesh(*(object.mesh()));
			GLUtilities::setCullState(true);
		}
	}
	_map->unbind();
	
	// --- Blur pass --------
	GLUtilities::setDepthState(false);
	_blur->process(_map->texture(), *_map);
}

void VarianceShadowMap2DArray::clean(){
	_blur->clean();
	_map->clean();
}

VarianceShadowMapCubeArray::VarianceShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer( TextureShape::ArrayCube, side, side, uint(lights.size()), 1,  {descriptor}, true));
	_program = Resources::manager().getProgram("object_cube_depth", "light_shadow_linear_vertex", "light_shadow_linear_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_map->texture(), lid);
	}
}

void VarianceShadowMapCubeArray::draw(const Scene & scene) const {

	GLUtilities::setDepthState(true);
	GLUtilities::setCullState(true);
	_map->setViewport();
	_program->use();

	for(size_t lid = 0; lid < _lights.size(); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			return;
		}
		// Udpate the light vp matrices.
		const auto & faces = light->vpFaces();

		// Pass the world space light position, and the projection matrix far plane.
		_program->uniform("lightPositionWorld", light->position());
		_program->uniform("lightFarPlane", light->farPlane());
		for(int i = 0; i < 6; ++i){
			// We render each face sequentially, culling objects that are not visible.
			_map->bind(lid * 6 + i);
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
				if(object.twoSided()) {
					GLUtilities::setCullState(false);
				}
				const glm::mat4 mvp = faces[i] * object.model();
				_program->uniform("mvp", mvp);
				_program->uniform("m", object.model());
				_program->uniform("hasMask", object.masked());
				if(object.masked()) {
					GLUtilities::bindTexture(object.textures()[0], 0);
				}
				GLUtilities::drawMesh(*(object.mesh()));
				GLUtilities::setCullState(true);
			}
		}
	}
	_map->unbind();
	// No blurring pass for now.
	GLUtilities::setDepthState(false);
}

void VarianceShadowMapCubeArray::clean(){
	_map->clean();
}
