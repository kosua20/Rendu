#ifndef ProgramInfos_h
#define ProgramInfos_h

#include "../Common.hpp"
#include <map>

class ProgramInfos {
public:
	
	ProgramInfos();
	
	ProgramInfos(const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName);
	
	~ProgramInfos();
	
	const GLint uniform(const std::string & name) const;

	// Version that cache the values passed for the uniform array. Other types will be added when needed.
	void cacheUniformArray(const std::string & name, const std::vector<glm::vec3> & vals);
	
	void registerTexture(const std::string & name, unsigned int slot);
	
	void registerTextures(const std::vector<std::string> & textures);
	
	void reload();
	
	void validate();

	void saveBinary(const std::string & outputPath);

	// To stay coherent with TextureInfos and MeshInfos, we keep the id public.
	const GLuint id() const { return _id; }
	
private:
	
	GLuint _id;
	std::string _vertexName;
	std::string _fragmentName;
	std::string _geometryName;
	std::map<std::string, GLint> _uniforms;
	std::map<std::string, unsigned int> _textures;
	std::map<std::string, glm::vec3> _vec3s;
	GLuint _nextTextureSlot = 0;
	
};



#endif
