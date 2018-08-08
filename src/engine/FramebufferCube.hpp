#ifndef FramebufferCube_h
#define FramebufferCube_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


class FramebufferCube {

public:
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	FramebufferCube(unsigned int side, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, bool depthBuffer);

	~FramebufferCube();
	
	/// Bind the framebuffer.
	void bind() const;

	void setViewport() const;

	/// Unbind the framebuffer.
	void unbind() const;
	
	/// Resize the framebuffer.
	void resize(unsigned int side);
	
	/// Clean.
	void clean() const;
	
	/// The ID to the texture containing the result of the framebuffer pass.
	const GLuint textureId() const { return _idColor; }
	
	/// The framebuffer size (can be different from the default renderer size).
	const unsigned int side() const { return _side; }
	const GLuint id() const { return _id; }
	const GLuint format() const { return _format; }
	const GLuint type() const { return _type; }
	const GLuint preciseFormat() const { return _preciseFormat; }
	
private:
	
	unsigned int _side;
	
	GLuint _id;
	GLuint _idColor;
	GLuint _idRenderbuffer;
	
	GLuint _format;
	GLuint _type;
	GLuint _preciseFormat;

	bool _useDepth;
	
};

#endif
