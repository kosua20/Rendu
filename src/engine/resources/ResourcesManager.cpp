#include "resources/ResourcesManager.hpp"
#include "resources/Mesh.hpp"
#include "system/System.hpp"


#include <tinydir/tinydir.h>
#include <miniz/miniz.h>
#include <fstream>
#include <sstream>

/** By enabling RESOURCES_PACKAGED, the resources will be loaded from a zip archive
 instead of the resources directory. Basic text files can still be read from disk
 (for configuration, settings,...) by using Resources::loadStringFromExternalFile. */
//#define RESOURCES_PACKAGED

// Singleton.
Resources & Resources::manager() {
	static Resources * res = new Resources();
	return *res;
}

#ifdef RESOURCES_PACKAGED
void Resources::addResources(const std::string & path) {
	Log::Info() << Log::Resources << "Loading resources from archive (" << path + ".zip"
				<< ")." << std::endl;
	parseArchive(path + ".zip");
}
#else

void Resources::addResources(const std::string & path) {
	Log::Info() << Log::Resources << "Loading resources from disk (" << path << ")." << std::endl;
	parseDirectory(path);
}
#endif

void Resources::parseArchive(const std::string & archivePath) {

	mz_zip_archive zip_archive = {0, 0, 0, MZ_ZIP_MODE_INVALID, MZ_ZIP_TYPE_INVALID, MZ_ZIP_NO_ERROR,
		0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	const int status		   = mz_zip_reader_init_file(&zip_archive, archivePath.c_str(), 0);
	if(!status) {
		Log::Error() << Log::Resources << "Unable to load zip file \"" << archivePath << "\" (" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << ")." << std::endl;
	}

	// Get and print information about each file in the archive.
	for(unsigned int i = 0; i < static_cast<unsigned int>(mz_zip_reader_get_num_files(&zip_archive)); ++i) {
		mz_zip_archive_file_stat file_stat;

		if(!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
			Log::Error() << Log::Resources << "Error reading file infos." << std::endl;
			mz_zip_reader_end(&zip_archive);
		}

		if(mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
			continue;
		}

		const std::string filePath		  = std::string(file_stat.m_filename);
		const std::string fileNameWithExt = filePath.substr(filePath.find_last_of("/\\") + 1);
		// Filter empty files and system files.
		if(!fileNameWithExt.empty() && fileNameWithExt.at(0) != '.') {
			if(_files.count(fileNameWithExt) == 0) {
				_files[fileNameWithExt] = (archivePath + "/").append(filePath);
			} else {
				// If the file already exists somewhere else in the hierarchy, warn about this.
				Log::Error() << Log::Resources << "Error: asset named \"" << fileNameWithExt << "\" alread exists." << std::endl;
			}
		}
	}
	mz_zip_reader_end(&zip_archive);
}

void Resources::parseDirectory(const std::string & directoryPath) {
	// Open directory.
	tinydir_dir dir;
	auto * widenedPath = System::widen(directoryPath);
	if(tinydir_open(&dir, widenedPath) == -1) {
		tinydir_close(&dir);
		Log::Error() << Log::Resources << "Unable to open resources directory at path \"" << directoryPath << "\"" << std::endl;
	}
	// For each file in dir.
	while(dir.has_next) {
		tinydir_file file;
		if(tinydir_readfile(&dir, &file) == -1) {
			// Handle any read error.
			Log::Error() << Log::Resources << "Error getting file in directory \"" << System::narrow(dir.path) << "\"" << std::endl;

		} else if(file.is_dir) {
			// Extract subdirectory name, check that it isn't a special dir, and recursively parse it.
			const std::string dirName = System::narrow(file.name);
			if(!dirName.empty() && dirName[0] != '.') {
				parseDirectory((directoryPath + "/").append(dirName));
			}

		} else {
			// Else, we have a regular file.
			const std::string fileNameWithExt = System::narrow(file.name);
			// Filter empty files and system files.
			if(!fileNameWithExt.empty() && fileNameWithExt.at(0) != '.') {
				if(_files.count(fileNameWithExt) == 0) {
					// Store the file and its path.
					_files[fileNameWithExt] = System::narrow(dir.path) + "/" + fileNameWithExt;

				} else {
					// If the file already exists somewhere else in the hierarchy, warn about this.
					Log::Error() << Log::Resources << "Error: asset named \"" << fileNameWithExt << "\" alread exists." << std::endl;
				}
			}
		}
		// Get to next file.
		if(tinydir_next(&dir) == -1) {
			// Reach end of dir early.
			break;
		}
	}
	tinydir_close(&dir);
}

// Image path utilities.

std::vector<std::string> Resources::getCubemapPaths(const std::string & name) {
	const std::vector<std::string> names {name + "_px", name + "_nx", name + "_py", name + "_ny", name + "_pz", name + "_nz"};
	std::vector<std::string> paths;
	paths.reserve(6);
	for(auto & faceName : names) {
		const std::string filePath = getImagePath(faceName);
		// If a face is missing, cancel the whole loading.
		if(filePath.empty()) {
			return std::vector<std::string>();
		}
		// Else append the path.
		paths.push_back(filePath);
	}
	return paths;
}

std::string Resources::getImagePath(const std::string & name) {
	std::string path;
	// Check if the file exists with an image extension.
	if(_files.count(name + ".png") > 0) {
		path = _files[name + ".png"];
	} else if(_files.count(name + ".jpg") > 0) {
		path = _files[name + ".jpg"];
	} else if(_files.count(name + ".jpeg") > 0) {
		path = _files[name + ".jpeg"];
	} else if(_files.count(name + ".bmp") > 0) {
		path = _files[name + ".bmp"];
	} else if(_files.count(name + ".tga") > 0) {
		path = _files[name + ".tga"];
	} else if(_files.count(name + ".exr") > 0) {
		path = _files[name + ".exr"];
	}
	return path;
}

// Base methods.

#ifdef RESOURCES_PACKAGED

char * Resources::getRawData(const std::string & path, size_t & size) {
	char * rawContent;
	mz_zip_archive zip_archive = {0};
	// Extract the archive path and the file internal path.
	const auto extensionPos = path.find(".zip/");
	if(extensionPos == std::string::npos) {
		Log::Error() << Log::Resources << "Unable to find archive for path \"" << path << "\"." << std::endl;
		return NULL;
	}
	const std::string archivePath = path.substr(0, extensionPos + 4);
	const std::string filePath	= path.substr(extensionPos + 5);

	int status = mz_zip_reader_init_file(&zip_archive, archivePath.c_str(), 0);
	if(!status) {
		Log::Error() << Log::Resources << "Unable to load zip file at path \"" << archivePath << "\" (" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << ")." << std::endl;
		return NULL;
	}
	rawContent = (char *)mz_zip_reader_extract_file_to_heap(&zip_archive, filePath.c_str(), &size, 0);
	mz_zip_reader_end(&zip_archive);
	return rawContent;
}

#else

char * Resources::getRawData(const std::string & path, size_t & size) {
	return Resources::loadRawDataFromExternalFile(path, size);
}

#endif

std::string Resources::getString(const std::string & filename) {
	std::string path;
	if(_files.count(filename) > 0) {
		path = _files[filename];
	} else if(_files.count(filename + ".txt") > 0) {
		path = _files[filename + ".txt"];
	} else {
		Log::Error() << Log::Resources << "Unable to find text file named \"" << filename << "\"." << std::endl;
		return "";
	}

	size_t rawSize	= 0;
	char * rawContent = Resources::manager().getRawData(path, rawSize);
	std::string content(rawContent, rawSize);
	delete[] rawContent;
	return content;
}

// Mesh method.

const Mesh * Resources::getMesh(const std::string & name, Storage mode) {
	if(_meshes.count(name) > 0) {
		return &_meshes[name];
	}

	const std::string meshText = getString(name + ".obj");
	if(meshText.empty()) {
		Log::Error() << Log::Resources << "Unable to load mesh named " << name << "." << std::endl;
		return nullptr;
	}
	// Load geometry. For now we only support OBJs.
	std::stringstream meshStream(meshText);
	_meshes.emplace(std::make_pair(name, Mesh(meshStream, Mesh::Load::Indexed, name)));
	
	auto & mesh = _meshes[name];
	// If uv or positions are missing, tangent/binormals won't be computed.
	mesh.computeTangentsAndBinormals();
	// Compute bounding box.
	mesh.computeBoundingBox();

	if(mode & Storage::GPU) {
		// Setup GL buffers and attributes.
		mesh.upload();
	}
	// If we are not planning on using the CPU data, remove it.
	if(!(mode & Storage::CPU)) {
		mesh.clearGeometry();
	}

	return &_meshes[name];
}

// Texture methods.

const Texture * Resources::getTexture(const std::string & name) {
	if(_textures.count(name) > 0) {
		return &_textures[name];
	}
	Log::Error() << Log::Resources << "Unable to find existing texture \"" << name << "\"" << std::endl;
	return nullptr;
}

const Texture * Resources::getTexture(const std::string & name, const Descriptor & descriptor, Storage mode, const std::string & refName) {
	const std::string & keyName = refName.empty() ? name : refName;

	// If texture already loaded, return it.
	if(_textures.count(keyName) > 0) {
		auto & texture = _textures[keyName];
		if(mode & Storage::GPU) {
			// If we want to store the texture on the GPU...
			if(texture.gpu) {
				// If the texture is already on the GPU, check that the layout is the same, else raise a warning.
				if(!texture.gpu->hasSameLayoutAs(descriptor)) {
					Log::Warning() << Log::Resources << "Texture \"" << keyName
								   << "\" already exist with a different descriptor." << std::endl;
				}
			} else {
				// Else upload to the GPU.
				texture.upload(descriptor, texture.levels == 1);
			}
		}
		// If we require CPU data but the images are empty, the texture CPU data was cleared...
		// Don't try and reload, just print an error.
		if((mode & Storage::CPU) && texture.images.empty()) {
			Log::Error() << Log::Resources << "Texture \"" << keyName
						 << "\" exists but is not CPU available." << std::endl;
		}
		return &_textures[keyName];
	}

	// Else, find the corresponding file(s).
	// Supported names:
	// * "file", "file_0": 2D
	// * "file_nx", "file_0_nx": cubemap
	// Future support:
	// * "file_s0", "file_0_s0": array 2D
	// * "file_nx_s0", "file_0_nx_s0": array cubemap
	// * "file_z0",  "file_0_z0": 3D

	std::vector<std::vector<std::string>> paths;

	const std::string path2D					= getImagePath(name);
	const std::string path2DMip					= getImagePath(name + "_0");
	const std::vector<std::string> pathCubes	= getCubemapPaths(name);
	const std::vector<std::string> pathCubesMip = getCubemapPaths(name + "_0");

	TextureShape shape = TextureShape::D2;

	if(!path2D.empty()) {
		shape = TextureShape::D2;
		paths.push_back({path2D});

	} else if(!pathCubes.empty()) {
		shape = TextureShape::Cube;
		paths.push_back(pathCubes);

	} else if(!path2DMip.empty()) {
		shape = TextureShape::D2;
		// We need to find the number of mipmap levels.
		unsigned int currLevel = 0;
		std::string mipmapPath = path2DMip;
		while(!mipmapPath.empty()) {
			// Transfer it to the final paths vector.
			paths.push_back({mipmapPath});
			++currLevel;
			// Next name to test.
			mipmapPath = getImagePath(name + "_" + std::to_string(currLevel));
		}

	} else if(!pathCubesMip.empty()) {
		shape = TextureShape::Cube;
		// We need to find the number of mipmap levels.
		unsigned int currLevel				 = 0;
		std::vector<std::string> mipmapPaths = pathCubesMip;
		while(!mipmapPaths.empty()) {
			// Transfer them to the final paths vector.
			paths.push_back(mipmapPaths);
			++currLevel;
			// Next name to test.
			mipmapPaths = getCubemapPaths(name + "_" + std::to_string(currLevel));
		}
	}

	if(paths.empty()) {
		// If couldn't file the image(s), return empty texture infos.
		Log::Error() << Log::Resources << "Unable to find texture named \"" << name << "\"." << std::endl;
		return nullptr;
	}

	// Format and orientation.
	const unsigned int channels = descriptor.getChannelsCount();
	// Cubemaps don't need to be flipped.
	const bool flip	= !(shape & TextureShape::Cube);
	_textures[keyName] = Texture(keyName);
	Texture & texture  = _textures[keyName];

	// Load all images.
	texture.images.reserve(paths.size() * paths[0].size());
	for(const auto & levelPaths : paths) {
		for(const auto & filePath : levelPaths) {
			texture.images.emplace_back();
			Image & image = texture.images.back();
			const int ret = image.load(filePath, channels, flip, false);
			if(ret != 0) {
				Log::Error() << Log::Resources << "Unable to load the texture at path " << filePath << "." << std::endl;
			}
		}
	}
	// Obtain the reference infos of the texture.
	texture.shape  = shape;
	texture.width  = texture.images[0].width;
	texture.height = texture.images[0].height;
	texture.depth  = uint(paths[0].size());
	texture.levels = uint(paths.size());

	// If GPU mode, send them to the GPU.
	if(mode & Storage::GPU) {
		// If only one level was given, generate the mipmaps.
		texture.upload(descriptor, texture.levels == 1);
	}
	// If GPU only, clear the CPU data.
	if(!(mode & Storage::CPU)) {
		texture.clearImages();
	}
	return &_textures[keyName];
}

// Program/shaders methods.

Program * Resources::getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName) {
	
	if(_programs.count(name) > 0) {
		return &_programs.at(name);
	}
	
	const std::string vName = vertexName.empty() ? name : vertexName;
	const std::string fName = fragmentName.empty() ? name : fragmentName;
	// For the geometry name, we don't replace by the default name.
	const std::string gName = geometryName;
	
	_programs.emplace(std::make_pair(name, Program(vName, fName, gName)));

	return &_programs.at(name);
}

Program * Resources::getProgram2D(const std::string & name) {
	return getProgram(name, "passthrough", name);
}

void Resources::reload() {
	for(auto & prog : _programs) {
		prog.second.reload();
	}
	Log::Info() << Log::Resources << "Shader programs reloaded." << std::endl;
}

Font * Resources::getFont(const std::string & name) {
	if(_fonts.count(name) > 0) {
		return &_fonts.at(name);
	}
	
	// Load the font descriptor and associated atlas.
	const std::string fontInfosText = getString(name + ".fnt");
	if(fontInfosText.empty()) {
		Log::Error() << Log::Resources << "Unable to load font named " << name << "." << std::endl;
		return nullptr;
	}
	std::stringstream fontStream(fontInfosText);
	_fonts.emplace(std::make_pair(name, Font(fontStream)));
	return &_fonts.at(name);
}

void Resources::getFiles(const std::string & extension, std::map<std::string, std::string> & files) const {
	files.clear();
	for(const auto & file : _files) {
		const std::string & fileName = file.first;
		const size_t lastPoint		 = fileName.find_last_of('.');
		if(lastPoint == std::string::npos) {
			//No extension, ext should be empty.
			if(extension.empty()) {
				files[fileName] = file.second;
			}
			continue;
		}
		const std::string fileExt = fileName.substr(lastPoint + 1);
		if(extension == fileExt) {
			// Obtain the name without the extension.
			files[fileName.substr(0, lastPoint)] = file.second;
		}
	}
}

// Static utilities methods.

char * Resources::loadRawDataFromExternalFile(const std::string & path, size_t & size) {

	std::ifstream inputFile(System::widen(path), std::ios::binary | std::ios::ate);
	if(inputFile.bad() || inputFile.fail()) {
		Log::Error() << Log::Resources << "Unable to load file at path \"" << path << "\"." << std::endl;
		size = 0;
		return nullptr;
	}
	const std::ifstream::pos_type fileSize = inputFile.tellg();
	char * rawContent					   = new char[fileSize];
	inputFile.seekg(0, std::ios::beg);
	inputFile.read(&rawContent[0], fileSize);
	inputFile.close();
	size = fileSize;
	return rawContent;
}

std::string Resources::loadStringFromExternalFile(const std::string & path) {
	std::ifstream inputFile(System::widen(path));
	if(inputFile.bad() || inputFile.fail()) {
		Log::Error() << Log::Resources << "Unable to load file at path \"" << path << "\"." << std::endl;
		return "";
	}
	std::stringstream buffer;
	// Read the stream in a buffer.
	buffer << inputFile.rdbuf();
	inputFile.close();
	// Create a string based on the content of the buffer.
	std::string line = buffer.str();
	return line;
}

void Resources::saveRawDataToExternalFile(const std::string & path, char * rawContent, size_t size) {
	std::ofstream outputFile(System::widen(path), std::ios::binary);

	if(!outputFile.is_open()) {
		Log::Error() << Log::Resources << "Unable to save file at path \"" << path << "\"." << std::endl;
		return;
	}
	outputFile.write(rawContent, size);
	outputFile.close();
}

void Resources::saveStringToExternalFile(const std::string & path, const std::string & content) {
	std::ofstream outputFile(System::widen(path));
	if(outputFile.bad() || outputFile.fail()) {
		Log::Error() << Log::Resources << "Unable to save file at path \"" << path << "\"." << std::endl;
		return;
	}
	outputFile << content;
	outputFile.close();
}

bool Resources::externalFileExists(const std::string & path) {
	// Just try to open the file.
	std::ifstream file(path);
	const bool opened = file.is_open();
	file.close();
	return opened;
}

void Resources::clean() {
	Log::Info() << Log::Resources << "Cleaning up." << std::endl;

	for(auto & tex : _textures) {
		tex.second.clean();
	}
	for(auto & mesh : _meshes) {
		mesh.second.clean();
	}
	for(auto & prog : _programs) {
		prog.second.clean();
	}
	_textures.clear();
	_meshes.clear();
	_fonts.clear();
	_programs.clear();
	_files.clear();
}
