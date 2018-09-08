#ifndef FramebufferCube_h
#define FramebufferCube_h

#include "Common.hpp"

/**
 \brief Represent a cubemap rendering target, of any size, format and type, backed by an OpenGL framebuffer composed of six layers.
 \ingroup Engine
 */
class FramebufferCube {

public:
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param side the width and height of each face of the framebuffer
	 \param format the OpenGL enum format (GL_RGB,...) to use
	 \param type the OpenGL enum type (GL_UNSIGNED_BYTE,...) to use
	 \param preciseFormat the precise format, combining format and type (GL_RGB8,...) to use
	 \param filtering the texture filtering (GL_LINEAR,...) to use
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	FramebufferCube(unsigned int side, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, bool depthBuffer);
	
	/**
	 Bind the framebuffer.
	 */
	void bind() const;
	
	/**
	 Set the viewport to the size of the framebuffer.
	 */
	void setViewport() const;

	/**
	 Unbind the framebuffer.
	 \note Technically bind the window backbuffer.
	 */
	void unbind() const;
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param side the new width and height for each face
	 */
	void resize(unsigned int side);
	
	/** Clean internal resources.
	 */
	void clean() const;
	
	/**
	 Query the ID to the cubemap texture backing the framebuffer.
	 \return the texture ID
	 */
	const GLuint textureId() const { return _idColor; }
	
	/**
	 Query the framebuffer side size.
	 \return the width/height of each face
	 */
	const unsigned int side() const { return _side; }
	
	/**
	 Query the framebuffer ID.
	 \return the ID
	 */
	const GLuint id() const { return _id; }
	
	/**
	 Query the framebuffer OpenGL format.
	 \return the format
	 */
	const GLuint format() const { return _format; }
	
	/**
	 Query the framebuffer OpenGL type.
	 \return the type
	 */
	const GLuint type() const { return _type; }
	
	/**
	 Query the framebuffer precise OpenGL format.
	 \return the precise format
	 */
	const GLuint preciseFormat() const { return _preciseFormat; }
	
private:
	
	unsigned int _side; ///< The size of each cubemap face sides.
	
	GLuint _id; ///< The framebuffer ID.
	GLuint _idColor; ///< The color texture ID.
	GLuint _idRenderbuffer; ///< The depth buffer ID.
	
	GLuint _format; ///< OpenGL format (GL_RGB,...).
	GLuint _type; ///< OpenGL type (GL_FLOAT,...).
	GLuint _preciseFormat; ///< OpenGL precise format (GL_RGB32F,...).

	bool _useDepth; ///< Denotes if the framebuffer is backed by a depth buffer.
	
};

#endif
