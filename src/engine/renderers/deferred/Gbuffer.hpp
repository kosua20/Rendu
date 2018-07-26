#ifndef Gbuffer_h
#define Gbuffer_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>

enum class TextureType {
	Albedo, // or base color
	Normal,
	Depth,
	Effects // roughness, metallicness, ambient occlusion, id.
};

class Gbuffer {

public:
	
	/// Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	Gbuffer(unsigned int width, unsigned int height);

	~Gbuffer();
	
	/// Bind the framebuffer.
	void bind() const;
	
	/// Unbind the framebuffer.
	void unbind() const;
	
	/// Resize the framebuffer.
	void resize(unsigned int width, unsigned int height);
	
	void resize(glm::vec2 size);
	
	/// Clean.
	void clean() const;
	
	/// The ID to the texture containing the result of the framebuffer pass.
	const GLuint textureId(const TextureType& type) { return _textureIds[type]; }
	
	const std::map<std::string, GLuint> textureIds(const std::vector<TextureType>& included = std::vector<TextureType>()) const ;
	
	/// The framebuffer size (can be different from the default renderer size).
	const unsigned int width() const { return _width; }
	const unsigned int height() const { return _height; }
	
private:
	unsigned int _width;
	unsigned int _height;
	
	GLuint _id;
	std::map<TextureType, GLuint> _textureIds;
};

#endif
