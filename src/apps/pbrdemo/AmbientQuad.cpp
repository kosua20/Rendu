#include "AmbientQuad.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/GPU.hpp"

AmbientQuad::AmbientQuad(const Texture * texAlbedo, const Texture * texNormals, const Texture * texEffects, const Texture * texDepth, const Texture * texSSAO) {

	_program = Resources::manager().getProgram2D("ambient_pbr");

	// Load texture.
	const Texture * textureBrdf = Resources::manager().getTexture("brdf-precomputed", {Layout::RG32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	// Ambient pass: needs the albedo, the normals, the depth, the effects, the AO result, the BRDF table and the  envmap.
	_textures.resize(7);
	_textures[0] = texAlbedo;
	_textures[1] = texNormals;
	_textures[2] = texEffects;
	_textures[3] = texDepth;
	_textures[4] = texSSAO;
	_textures[5] = textureBrdf;
	_textures[6] = nullptr;

	checkGPUError();
}

void AmbientQuad::draw(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix, const LightProbe & environment) {

	GPU::setDepthState(false);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	const glm::mat4 invView = glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	_textures[6] = environment.map();

	_program->use();
	_program->uniform("inverseV", invView);
	_program->uniform("projectionMatrix", projectionVector);
	_program->uniform("maxLod", float(_textures[6]->levels-1));
	_program->uniform("cubemapPos", environment.position());
	_program->uniform("cubemapCenter", environment.center());
	_program->uniform("cubemapExtent", environment.extent());
	_program->uniform("cubemapCosSin", environment.rotationCosSin());
	_program->buffer(*environment.shCoeffs(), 0);
	_program->textures(_textures);
	ScreenQuad::draw();
}
