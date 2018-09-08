#ifndef Framebuffer_h
#define Framebuffer_h

#include "Common.hpp"

/**
 \brief Represent a 2D rendering target, of any size, format and type, backed by an OpenGL framebuffer.
 \ingroup Engine
 */
class Framebuffer {

public:
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param format the OpenGL enum format (GL_RGB,...) to use
	 \param type the OpenGL enum type (GL_UNSIGNED_BYTE,...) to use
	 \param preciseFormat the precise format, combining format and type (GL_RGB8,...) to use
	 \param filtering the texture filtering (GL_LINEAR,...) to use
	 \param wrapping the texture wrapping mode (GL_CLAMP_TO_EDGE) to use
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, GLuint wrapping, bool depthBuffer);
	
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
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param size the new size
	 */
	void resize(glm::vec2 size);
	
	/** Clean internal resources.
	 */
	void clean() const;
	
	/**
	 Query the ID of the 2D texture backing the framebuffer.
	 \return the texture ID
	 */
	const GLuint textureId() const { return _idColor; }
	
	/**
	 Query the framebuffer width.
	 \return the width
	 */
	const unsigned int width() const { return _width; }
	
	/**
	 Query the framebuffer height.
	 \return the height
	 */
	const unsigned int height() const { return _height; }
	
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
	
	unsigned int _width; ///< The framebuffer width.
	unsigned int _height; ///< The framebuffer height.
	
	GLuint _id; ///< The framebuffer ID.
	GLuint _idColor; ///< The color texture ID.
	GLuint _idRenderbuffer; ///< The depth renderbuffer ID.
	
	GLuint _format; ///< OpenGL format (GL_RGB,...).
	GLuint _type; ///< OpenGL type (GL_FLOAT,...).
	GLuint _preciseFormat; ///< OpenGL precise format (GL_RGB32F,...).

	bool _useDepth; ///< Denotes if the framebuffer is backed by a depth renderbuffer.
	
};

#endif
