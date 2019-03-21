#ifndef SSAO_h
#define SSAO_h
#include "processing/BoxBlur.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

/**
 \brief Computes screen space ambient occlusion.
 \see GLSL::Frag::SSAO
 \ingroup Processing
 */
class SSAO {

public:
	
	/**
	 Constructor.
	 \param width the internal resolution width
	 \param height the internal resolution height
	 \param radius
	 */
	SSAO(unsigned int width, unsigned int height, float radius);

	/**
	 
	 */
	void process(const glm::mat4 & projection, const GLuint depthTex, const GLuint normalTex);
	
	/**
	 */
	void clean() const;

	/**
	 */
	void resize(unsigned int width, unsigned int height);
	
	/**
	 Clear the final framebuffer texture.
	 */
	void clear();
	
	/**
	 Query the texture containing the result of the SSAO+blur pass.
	 \return the texture ID
	 */
	GLuint textureId() const;
	
private:
	
	std::unique_ptr<Framebuffer> _ssaoFramebuffer; ///< SSAO framebuffer
	std::unique_ptr<BoxBlur> _blurSSAOBuffer; ///< SSAO blur processing.
	std::shared_ptr<ProgramInfos> _programSSAO; ///< The SSAO program.
	
	float _radius = 0.5f;
	GLuint _noiseTextureID;
};

#endif
