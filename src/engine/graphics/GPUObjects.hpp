#pragma once

#include "Common.hpp"


/** \brief Regroups format, type, filtering and wrapping informations for a color buffer.
  \ingroup Graphics
 */
struct Descriptor {
	
	GLuint typedFormat; ///< The precise typed format.
	GLuint filtering; ///< Minification filtering mode.
	GLuint wrapping; ///< Wrapping mode.
	
	/** Default constructor. RGB8, linear, clamp. */
	Descriptor();
	
	/** Constructor.
	 \param typedFormat_ the precise typed format to use
	 \param filtering_ the texture minification filtering (GL_LINEAR_MIPMAP_NEAREST,...) to use
	 \param wrapping_ the texture wrapping mode (GL_CLAMP_TO_EDGE) to use
	 */
	Descriptor(const GLuint typedFormat_, const GLuint filtering_, const GLuint wrapping_);
	
	/** Obtain the separate type, format and channel count of the texture typed format.
	 \param type will contain the type (GL_FLOAT,...)
	 \param format will contain the general layout (GL_RG,...)
	 \return the number of channels
	 */
	unsigned int getTypeAndFormat(GLuint & type, GLuint & format) const;
	
	/** Query the number of channels.
	 \return the number of channels
	 */
	unsigned int getChannelsCount() const;
	
	/** Obtain the texture magnification filter, removing the mipmaping qualifier.
	 \return the magnification filter
	 */
	GLuint getMagnificationFilter() const;
};

/**
 \brief Store a texture data on the GPU.
 \ingroup Graphics
 */
struct GPUTexture {
	
	Descriptor descriptor = Descriptor(); ///< The texture format, type, filtering.
	GLuint id = 0; ///< The OpenGL texture ID.
	
	/** Clean internal GPU buffer. */
	void clean();
};

/**
 \brief Store geometry buffers on the GPU.
 \ingroup Graphics
 */
struct GPUMesh {
	
	GLuint vId = 0; ///< The vertex array OpenGL ID.
	GLuint eId = 0; ///< The element buffer OpenGL ID.
	GLsizei count = 0; ///< The number of vertices (cached).
	GLuint vbo = 0; ///< The vertex buffer objects OpenGL ID.
	
	/** Clean internal GPU buffers. */
	void clean();
	
};
