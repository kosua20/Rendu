#include "ResourcesManager.h"

#include <iostream>
#include <tinydir/tinydir.h>
#include "ProgramUtilities.h"

/// Singleton.
Resources Resources::_resourcesManager = Resources("resources");


Resources::Resources(const std::string & root) : _rootPath(root){
	// Parse directory for all files contained in it and its subdirectory.
	parseDirectory(_rootPath);
}


Resources::~Resources(){ }


void Resources::parseDirectory(const std::string & directoryPath){
	// Open directory.
	tinydir_dir dir;
	if(tinydir_open(&dir, directoryPath.c_str()) == -1){
		tinydir_close(&dir);
		std::cerr << "Unable to open resources directory at path \"" << directoryPath << "\"" << std::endl;
	}
	
	// For each file in dir.
	while (dir.has_next) {
		tinydir_file file;
		if(tinydir_readfile(&dir, &file) == -1){
			// Handle any read error.
			std::cerr << "Error getting file in directory \"" << std::string(dir.path) << "\"" << std::endl;
			
		} else if(file.is_dir){
			// Extract subdirectory name, check that it isn't a special dir, and recursively aprse it.
			const std::string dirName(file.name);
			if(dirName.size() > 0 && dirName != "." && dirName != ".."){
				// @CHECK: "/" separator on Windows.
				parseDirectory(directoryPath + "/" + dirName);
			}
			
		} else {
			// Else, we have a regular file.
			const std::string fileNameWithExt(file.name);
			// Filter empty files and system files.
			if(fileNameWithExt.size() > 0 && fileNameWithExt.at(0) != '.' ){
				if(_files.count(fileNameWithExt) == 0){
					// Store the file and its path.
					// @CHECK: "/" separator on Windows.
					_files[fileNameWithExt] = std::string(dir.path) + "/" + fileNameWithExt;
					
				} else {
					// If the file already exsist somewhere else in the hierarchy, warn about this.
					std::cerr << "Error: asset named \"" << fileNameWithExt << "\" alread exists." << std::endl;
				}
			}
		}
		// Get to next file.
		if (tinydir_next(&dir) == -1){
			// Reach end of dir early.
			break;
		}
		
	}
	tinydir_close(&dir);
}


const TextureInfos Resources::getTexture(const std::string & name, bool srgb){
	// If texture already loaded, return it.
	if(_textures.count(name) > 0){
		return _textures[name];
	}
	// Else, find the corresponding file.
	TextureInfos infos;
	std::string path = getImagePath(name);
	// If couldn't file the image, return empty texture infos.
	if(path.empty()){
		return infos;
	}
	// Else, load it and store the infos.
	int width = 0;
	int height = 0;
	infos.id = ProgramUtilities::loadTexture(path, width, height, srgb);
	infos.width = width;
	infos.height = height;
	infos.cubemap = false;
	_textures[name] = infos;
	return infos;
}


const TextureInfos Resources::getCubemap(const std::string & name, bool srgb){
	// If texture already loaded, return it.
	if(_textures.count(name) > 0){
		return _textures[name];
	}
	// Else, find the corresponding files.
	TextureInfos infos;
	std::vector<std::string> paths = getCubemapPaths(name);
	if(paths.empty()){
		return infos;
	}
	// Load them and store the infos.
	int width = 0;
	int height = 0;
	infos.id = ProgramUtilities::loadTextureCubemap(paths, width, height, srgb);
	infos.width = width;
	infos.height = height;
	infos.cubemap = true;
	_textures[name] = infos;
	return infos;
}


const std::vector<std::string> Resources::getCubemapPaths(const std::string & name){
	const std::vector<std::string> names { name + "_r", name + "_l", name + "_u", name + "_d", name + "_b", name + "_f" };
	std::vector<std::string> paths;
	paths.reserve(6);
	for(auto & faceName : names){
		const std::string filePath = getImagePath(faceName);
		// If a face is missing, cancel the whole loading.
		if(filePath.empty()){
			return std::vector<std::string>();
		}
		// Else append the path.
		paths.push_back(filePath);
	}
	return paths;
}

const std::string Resources::getImagePath(const std::string & name){
	std::string path = "";
	// Check if the file exists with an image extension.
	if(_files.count(name + ".png") > 0){
		path = _files[name + ".png"];
	} else if(_files.count(name + ".jpg") > 0){
		path = _files[name + ".jpg"];
	} else if(_files.count(name + ".jpeg") > 0){
		path = _files[name + ".jpeg"];
	} else if(_files.count(name + ".bmp") > 0){
		path = _files[name + ".bmp"];
	} else if(_files.count(name + ".tga") > 0){
		path = _files[name + ".tga"];
	} else {
		std::cerr << "Unable to find image named \"" << name << "\"" << std::endl;
	}
	return path;
}



