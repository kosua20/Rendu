#ifndef ProgramInfos_h
#define ProgramInfos_h

#include <gl3w/gl3w.h>
#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>

class ProgramInfos {
public:
	
	ProgramInfos();
	
	ProgramInfos(const std::string & vertexContent, const std::string & fragmentContent);
	
	~ProgramInfos();
	
	const GLint uniform(const std::string & name) const { return _uniforms.at(name); }
	
	void registerUniform(const std::string & name);

	void registerUniforms(const std::vector<std::string> & names);

	// Version that cache the value passed for the uniform. Other types will be added when needed.
	void registerUniform(const std::string & name, const glm::vec3 & val);
	
	void registerTexture(const std::string & name, int slot);
	
	void reload(const std::string & vertexContent, const std::string & fragmentContent);

	// To stay coherent with TextureInfos and MeshInfos, we keep the id public.
	
	const GLuint id() const { return _id; }
	
private:
	
	GLuint _id;
	std::map<std::string, GLint> _uniforms;
	std::map<std::string, int> _textures;
	std::map<std::string, glm::vec3> _vec3s;
	
};



#endif
