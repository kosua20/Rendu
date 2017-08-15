#ifndef ResourcesManager_h
#define ResourcesManager_h

#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <map>
#include "ProgramUtilities.h"
#include "MeshUtilities.h"

class Resources {
public:
	
	const MeshInfos getMesh(const std::string & name);
	
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
private:
	
	void parseDirectory(const std::string & directoryPath);
	
	const std::string getImagePath(const std::string & name);
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	const std::string & _rootPath;
	
	std::map<std::string, std::string> _files;
	
	std::map<std::string, TextureInfos> _textures;
	
	std::map<std::string, MeshInfos> _meshes;
	
	std::map<std::string, std::string> _shaders;
	
/// Singleton management.
		
public:
	
	static Resources& manager(){ return _resourcesManager; }
	
private:
	
	Resources(const std::string & root);
	
	~Resources();
	
	Resources& operator= (const Resources&);
	
	Resources (const Resources&);

	static Resources _resourcesManager;
	
};



#endif
