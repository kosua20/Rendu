#ifndef ResourcesManager_h
#define ResourcesManager_h

#include <gl3w/gl3w.h>
#include <string>
#include <vector>
#include <map>
#include "ProgramUtilities.h"

struct TextureInfos {
	GLuint id;
	int width;
	int height;
	bool cubemap;
};

class Resources {
public:
	
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
private:
	
	void parseDirectory(const std::string & directoryPath);
	
	const std::string getImagePath(const std::string & name);
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	const std::string & _rootPath;
	std::map<std::string, std::string> _files;
	std::map<std::string, TextureInfos> _textures;
	
	
	
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
