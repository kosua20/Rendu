#include "processing/SSAO.hpp"
#include "system/Random.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

SSAO::SSAO(unsigned int width, unsigned int height, float radius) {
	_radius = radius;
	_ssaoFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, Layout::R8, false));
	_blurSSAOBuffer = std::unique_ptr<BoxBlur>(new BoxBlur(width, height, true, Descriptor(Layout::R8, Filter::LINEAR_LINEAR, Wrap::CLAMP)));
	_programSSAO = Resources::manager().getProgram2D("ssao");
	
	// Generate samples.
	std::vector<glm::vec3> samples;
	// We need random vectors in the half sphere above z, with more samples close to the center.
	for(int i = 0; i < 24; ++i){
		glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
									  Random::Float(-1.0f, 1.0f),
									  Random::Float(0.0f, 1.0f) );
		samples.push_back(glm::normalize(randVec));
		samples.back() *= Random::Float(0.0f,1.0f);
		// Skew the distribution towards the center.
		float scale = i/24.0f;
		scale = 0.1f+0.9f*scale*scale;
		samples.back() *= scale;
	}
	// Send the samples to the GPU.
	_programSSAO->cacheUniformArray("samples", samples);
	
	// Noise texture (same size as the box blur applied after SSAO computation).
	// We need to generate two dimensional normalized offsets.
	_noiseTextureID.width = 5;
	_noiseTextureID.height = 5;
	_noiseTextureID.depth = 1;
	_noiseTextureID.levels = 1;
	_noiseTextureID.shape = TextureShape::D2;
	_noiseTextureID.images.emplace_back();
	Image & img = _noiseTextureID.images.back();
	img.width = 5;
	img.height = 5;
	img.components = 3;
	
	for(int i = 0; i < 25; ++i){
		const glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
									  Random::Float(-1.0f, 1.0f),
									  0.0f);
		const glm::vec3 norVec = glm::normalize(randVec);
		img.pixels.push_back(norVec[0]);
		img.pixels.push_back(norVec[1]);
		img.pixels.push_back(norVec[2]);
	}
	
	// Send the texture to the GPU.
	GLUtilities::setupTexture(_noiseTextureID, { Layout::RGB32F, Filter::NEAREST, Wrap::REPEAT});
	GLUtilities::uploadTexture(_noiseTextureID);
	
	checkGLError();
	
}

// Draw function
void SSAO::process(const glm::mat4 & projection, const Texture * depthTex, const Texture * normalTex){
	
	_ssaoFramebuffer->bind();
	_ssaoFramebuffer->setViewport();
	_programSSAO->use();
	_programSSAO->uniform("projectionMatrix", projection);
	_programSSAO->uniform("radius", _radius);
	ScreenQuad::draw({depthTex, normalTex, &_noiseTextureID});
	_ssaoFramebuffer->unbind();
	
	// Blurring pass
	_blurSSAOBuffer->process(_ssaoFramebuffer->textureId());
}


// Clean function
void SSAO::clean() const {
	_blurSSAOBuffer->clean();
	_ssaoFramebuffer->clean();
}

void SSAO::clear() {
	_blurSSAOBuffer->clear();
}

// Handle screen resizing
void SSAO::resize(unsigned int width, unsigned int height){
	_blurSSAOBuffer->resize(width, height);
	_ssaoFramebuffer->resize(width, height);
}

const Texture * SSAO::textureId() const {
	return _blurSSAOBuffer->textureId();
}

float & SSAO::radius(){
	return _radius;
}

