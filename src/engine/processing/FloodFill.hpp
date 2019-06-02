#pragma once
#include "graphics/Framebuffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief
 \ingroup Processing
 */
class FloodFill {

public:

	/** Constructor.
	 \param width internal processing width
	 \param height internal processing height
	 */
	FloodFill(unsigned int width, unsigned int height);
	
	/** Filter a given input texture.
	 \param textureId the GPU ID of the texture
	 */
	void process(const GLuint textureId);
	
	/** Cleanup internal resources. */
	void clean() const;
	
	/** Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/** The GPU ID of the filter result.
	 \return the ID of the result texture
	 */
	GLuint textureId(){ return _final->textureId(); }
	
private:
	
	const ProgramInfos * _extract;
	const ProgramInfos * _floodfill;
	const ProgramInfos * _composite;
	
	std::unique_ptr<Framebuffer> _ping;
	std::unique_ptr<Framebuffer> _pong;
	std::unique_ptr<Framebuffer> _final;
	
	int _iterations;
	
};

