#include "SSAO.hpp"
#include "helpers/Random.hpp"

SSAO::SSAO(unsigned int width, unsigned int height, float radius) {
	_radius = radius;
	_ssaoFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, GL_R8, false));
	_blurSSAOBuffer = std::unique_ptr<BoxBlur>(new BoxBlur(width, height, true, Descriptor(GL_R8, GL_LINEAR_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE)));
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
	std::vector<glm::vec3> noise;
	for(int i = 0; i < 25; ++i){
		glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
									  Random::Float(-1.0f, 1.0f),
									  0.0f);
		noise.push_back(glm::normalize(randVec));
	}
	
	// Send the texture to the GPU.
	glGenTextures(1, &_noiseTextureID);
	glBindTexture(GL_TEXTURE_2D, _noiseTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 5 , 5, 0, GL_RGB, GL_FLOAT, &(noise[0]));
	// Need nearest filtering and repeat.
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	checkGLError();
	
}

// Draw function
void SSAO::process(const glm::mat4 & projection, const GLuint depthTex, const GLuint normalTex){
	
	_ssaoFramebuffer->bind();
	_ssaoFramebuffer->setViewport();
	glUseProgram(_programSSAO->id());
	glUniformMatrix4fv(_programSSAO->uniform("projectionMatrix"), 1, GL_FALSE, &projection[0][0]);
	glUniform1f(_programSSAO->uniform("radius"), _radius);
	ScreenQuad::draw({depthTex, normalTex, _noiseTextureID});
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

GLuint SSAO::textureId() const {
	return _blurSSAOBuffer->textureId();
}

