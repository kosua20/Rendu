#include "VarianceShadowMapArray.hpp"
#include "scene/Scene.hpp"
#include "graphics/GLUtilities.hpp"

VarianceShadowMap2DArray::VarianceShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer(TextureShape::Array2D, uint(resolution.x), uint(resolution.y), uint(lights.size()), {descriptor}, true));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(TextureShape::Array2D, uint(resolution.x), uint(resolution.y), uint(lights.size()), descriptor, false));
	_program = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_blur->textureId(), lid);
	}
}

void VarianceShadowMap2DArray::draw(const Scene & scene) const {

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
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
				glDisable(GL_CULL_FACE);
			}
			_program->uniform("hasMask", object.masked());
			if(object.masked()) {
				GLUtilities::bindTexture(object.textures()[0], 0);
			}
			const glm::mat4 lightMVP = light->vp() * object.model();
			_program->uniform("mvp", lightMVP);
			GLUtilities::drawMesh(*(object.mesh()));
			glEnable(GL_CULL_FACE);
		}
	}
	_map->unbind();
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blur->process(_map->textureId());
}

void VarianceShadowMap2DArray::clean(){
	_blur->clean();
	_map->clean();
}

VarianceShadowMapCubeArray::VarianceShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer( TextureShape::ArrayCube, side, side, uint(lights.size()), {descriptor}, true));
	_program = Resources::manager().getProgram("object_cube_depth", "object_basic_texture_worldpos", "light_shadow_linear_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_map->textureId(), lid);
	}
}

void VarianceShadowMapCubeArray::draw(const Scene & scene) const {

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
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
					glDisable(GL_CULL_FACE);
				}
				const glm::mat4 mvp = faces[i] * object.model();
				_program->uniform("mvp", mvp);
				_program->uniform("m", object.model());
				_program->uniform("hasMask", object.masked());
				if(object.masked()) {
					GLUtilities::bindTexture(object.textures()[0], 0);
				}
				GLUtilities::drawMesh(*(object.mesh()));
				glEnable(GL_CULL_FACE);
			}
		}
	}
	_map->unbind();
	// No blurring pass for now.
	glDisable(GL_DEPTH_TEST);
}

void VarianceShadowMapCubeArray::clean(){
	_map->clean();
}
