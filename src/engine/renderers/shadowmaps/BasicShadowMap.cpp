#include "BasicShadowMap.hpp"
#include "scene/Scene.hpp"
#include "graphics/GPU.hpp"

BasicShadowMap2DArray::BasicShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution, ShadowMode mode) : _map("Shadow map 2D array"){
	_lights = lights;
	/// \bug The depth buffer will contain extra garbage data and can't be used as an input to the light pass currently.
	_map.setupAsDrawable(Layout::DEPTH_COMPONENT32F, uint(resolution.x), uint(resolution.y), TextureShape::Array2D, 1, uint(lights.size()));

	_program = Resources::manager().getProgram("object_depth_array", "light_shadow_vertex", "light_shadow_basic");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(&_map, mode, lid);
	}
}

void BasicShadowMap2DArray::draw(const Scene & scene) {

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::setViewport(_map);
	_program->use();
	_program->defaultTexture(0);

	for(uint lid = 0; lid < uint(_lights.size()); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			continue;
		}
		GPU::beginRender(lid, 0, 1.f, Load::Operation::DONTCARE, &_map);
		const Frustum lightFrustum(light->vp());

		for(auto & object : scene.objects) {
			if(!object.castsShadow()) {
				continue;
			}
			// Frustum culling.
			if(!lightFrustum.intersects(object.boundingBox())){
				continue;
			}
			const Material& mat = object.material();
			GPU::setCullState(!mat.twoSided(), Faces::BACK);

			_program->uniform("hasMask", mat.masked());
			if(mat.masked()) {
				_program->texture(mat.textures()[0], 0);
			}
			const glm::mat4 lightMVP = light->vp() * object.model();
			_program->uniform("mvp", lightMVP);
			GPU::drawMesh(*(object.mesh()));
		}
		GPU::endRender();
	}

}

BasicShadowMapCubeArray::BasicShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side, ShadowMode mode) : _map("Shadow map cube array"){
	_lights = lights;
	_map.setupAsDrawable(Layout::DEPTH_COMPONENT32F, side, side, TextureShape::ArrayCube, 1, uint(lights.size()));
	_program = Resources::manager().getProgram("object_cube_depth_array", "light_shadow_linear_vertex", "light_shadow_linear_basic");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(&_map, mode, lid);
	}
}

void BasicShadowMapCubeArray::draw(const Scene & scene) {

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);
	GPU::setViewport(_map);
	_program->use();
	_program->defaultTexture(0);

	for(uint lid = 0; lid < uint(_lights.size()); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			continue;
		}
		// Udpate the light vp matrices.
		const auto & faces = light->vpFaces();

		// Pass the world space light position, and the projection matrix far plane.
		_program->uniform("lightPositionWorld", light->position());
		_program->uniform("lightFarPlane", light->farPlane());
		for(uint i = 0; i < 6; ++i){
			// We render each face sequentially, culling objects that are not visible.
			GPU::beginRender(lid * 6 + i, 0, 1.f, Load::Operation::DONTCARE, &_map);
			const Frustum lightFrustum(faces[i]);

			for(auto & object : scene.objects) {
				if(!object.castsShadow()) {
					continue;
				}
				// Frustum culling.
				if(!lightFrustum.intersects(object.boundingBox())){
					continue;
				}
				const Material& mat = object.material();
				GPU::setCullState(!mat.twoSided(), Faces::BACK);
				const glm::mat4 mvp = faces[i] * object.model();
				_program->uniform("mvp", mvp);
				_program->uniform("m", object.model());
				_program->uniform("hasMask", mat.masked());
				if(mat.masked()) {
					_program->texture(mat.textures()[0], 0);
				}
				GPU::drawMesh(*(object.mesh()));
			}
			GPU::endRender();
		}
	}
}

EmptyShadowMap2DArray::EmptyShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights){
	for(size_t lid = 0; lid < lights.size(); ++lid){
		lights[lid]->registerShadowMap(nullptr, ShadowMode::NONE, 0);
	}
}

void EmptyShadowMap2DArray::draw(const Scene & ) {
}

EmptyShadowMapCubeArray::EmptyShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights){
	for(size_t lid = 0; lid < lights.size(); ++lid){
		lights[lid]->registerShadowMap(nullptr, ShadowMode::NONE, 0);
	}
}

void EmptyShadowMapCubeArray::draw(const Scene & ) {
}
