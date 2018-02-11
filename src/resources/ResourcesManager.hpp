#ifndef ResourcesManager_h
#define ResourcesManager_h

#include "GLUtilities.hpp"
#include "ProgramInfos.hpp"
#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class Resources {
public:
	
	enum ShaderType {
		Vertex, Fragment
	};
	
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name);
	
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName);
	
	const std::string getShader(const std::string & name, const ShaderType & type);

	const MeshInfos getMesh(const std::string & name);
	
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
	const std::string getTextFile(const std::string & filename);
	
	void reload();
	
	static std::string loadStringFromFile(const std::string & filename);
	
	static std::string trim(const std::string & str, const std::string & del);
	
private:
	
	void parseDirectory(const std::string & directoryPath);
	
	const std::string getImagePath(const std::string & name);
	
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	const std::string & _rootPath;
	
	std::map<std::string, std::string> _files;
	
	std::map<std::string, TextureInfos> _textures;
	
	std::map<std::string, MeshInfos> _meshes;
	
	std::map<std::string, std::shared_ptr<ProgramInfos>> _programs;
	
	//std::map<std::string, std::string> _shaders;
	
/// Singleton management.
		
public:
	
	static Resources& manager();
	
private:
	
	Resources(const std::string & root);
	
	~Resources();
	
	Resources& operator= (const Resources&);
	
	Resources (const Resources&);
	
};

#endif
