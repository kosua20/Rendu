#include "VarianceShadowMap.hpp"
#include "scene/Scene.hpp"
#include "graphics/GPU.hpp"

VarianceShadowMap2DArray::VarianceShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution)
	: _map("Shadow map 2D Variance array"), _mapDepth("Shadow map 2D Depth array") {
	_lights = lights;

	_map.setupAsDrawable(Layout::RG32F, uint(resolution.x), uint(resolution.y), TextureShape::Array2D, 1, uint(lights.size()));
	_mapDepth.setupAsDrawable(Layout::DEPTH_COMPONENT32F, uint(resolution.x), uint(resolution.y), TextureShape::Array2D, 1, uint(lights.size()));

	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(false, "Shadow maps 2D"));
	_program = Resources::manager().getProgram("object_depth_array_variance", "light_shadow_vertex", "light_shadow_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(&_map, ShadowMode::VARIANCE, lid);
	}
}

void VarianceShadowMap2DArray::draw(const Scene & scene) {

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

		GPU::beginRender(lid , 0, glm::vec4(1.0f), 1.0f, Load::Operation::DONTCARE, &_mapDepth, &_map);

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
	
	// Apply box blur.
	_blur->process(_map, _map);
}

VarianceShadowMapCubeArray::VarianceShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side)
	: _map("Shadow map cube Variance array"), _mapDepth("Shadow map cube Depth array"){
	_lights = lights;

	_map.setupAsDrawable(Layout::RG16F, side, side, TextureShape::ArrayCube, 1, uint(lights.size()));
	_mapDepth.setupAsDrawable(Layout::DEPTH_COMPONENT32F,side, side, TextureShape::ArrayCube, 1, uint(lights.size()));

	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(true, "Shadow maps cube"));
	_program = Resources::manager().getProgram("object_cube_depth_array_variance", "light_shadow_linear_vertex", "light_shadow_linear_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(&_map, ShadowMode::VARIANCE, lid);
	}
}

void VarianceShadowMapCubeArray::draw(const Scene & scene) {

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
			GPU::beginRender(lid * 6 + i, 0, glm::vec4(1.0f), 1.0f, Load::Operation::DONTCARE, &_mapDepth, &_map);
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
	// Apply box blur.
	_blur->process(_map, _map);
}
