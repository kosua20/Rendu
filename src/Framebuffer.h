#ifndef Framebuffer_h
#define Framebuffer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>


class Framebuffer {

public:
	
	Framebuffer();
	
	Framebuffer(int width, int height);

	~Framebuffer();
	
	/// Bind the framebuffer.
	void bind();
	
	/// Unbind the framebuffer.
	void unbind();
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	void setup(GLuint type, GLuint filtering, GLuint wrapping);
	
	/// Resize the framebuffer.
	void resize(int width, int height);
	
	/// Clean.
	void clean();
	
	/// The ID to the texture containing the result of the framebuffer pass.
	GLuint textureId() { return _idColor; }
	
	/// The framebuffer size (can be different from the default renderer size).
	int _width;
	int _height;
	
private:

	GLuint _id;
	GLuint _idColor;
	GLuint _idRenderbuffer;
};

#endif
