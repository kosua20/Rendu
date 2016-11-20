#ifndef Gbuffer_h
#define Gbuffer_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <map>

enum class TextureType {
	Albedo,
	Normal,
	Depth,
	Effects
};

class Gbuffer {

public:
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	Gbuffer(int width, int height);

	~Gbuffer();
	
	/// Bind the framebuffer.
	void bind();
	
	/// Unbind the framebuffer.
	void unbind();
	
	/// Resize the framebuffer.
	void resize(int width, int height);
	
	void resize(glm::vec2 size);
	
	/// Clean.
	void clean();
	
	/// The ID to the texture containing the result of the framebuffer pass.
	GLuint textureId(const TextureType& type) { return _textureIds[type]; }
	
	std::map<std::string, GLuint> textureIds();
	
	/// The framebuffer size (can be different from the default renderer size).
	int _width;
	int _height;
	
private:

	GLuint _id;
	std::map<TextureType, GLuint> _textureIds;
};

#endif
