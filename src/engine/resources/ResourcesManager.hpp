#pragma once

#include "graphics/GPUObjects.hpp"
#include "graphics/Program.hpp"
#include "resources/Font.hpp"
#include "resources/Mesh.hpp"

#include "Common.hpp"

/**
 \brief Denote if data is stored on the GPU or CPU.
 \ingroup Resources
 */
enum class Storage : uint {
	GPU  = 1,		   ///< On the GPU
	CPU  = 2,		   ///< On the CPU
	BOTH = (GPU | CPU) ///< On both the CPU and GPU
};

/** Combining operator for Storage.
 \param t0 first flag
 \param t1 second flag
 \return the combination of both flags.
 */
inline Storage operator|(Storage t0, Storage t1) {
	return static_cast<Storage>(static_cast<uint>(t0) | static_cast<uint>(t1));
}

/** Extracting operator for Storage.
 \param t0 reference flag
 \param t1 flag to extract
 \return true if t0 'contains' t1
 */
inline bool operator&(Storage t0, Storage t1) {
	return bool(static_cast<uint>(t0) & static_cast<uint>(t1));
}

/**
 \brief The Resources manager is responsible for all resources loading and setup.
 \details It provides an abstraction over the file system: resources can be loaded directly from files on disk, or from a zip archive.
 \ingroup Resources
 */
class Resources {

	friend class Image;

public:
	/** Singleton accessor.
	 \return the resources manager singleton
	 */
	static Resources & manager();

	/** Add another resources directory/archive.
	 \param path the path to the additional directory/archive to parse
	 */
	void addResources(const std::string & path);

	/** Reload all shader programs.
	 */
	void reload();

	/** Clean all loaded resources, both CPU and GPU side. */
	void clean();

	/** Copy assignment operator (disabled). */
	Resources & operator=(const Resources &) = delete;

	/** Copy constructor (disabled). */
	Resources(const Resources &) = delete;

	/** Move assignment operator (disabled). */
	Resources & operator=(const Resources &&) = delete;

	/** Move constructor (disabled). */
	Resources(const Resources &&) = delete;

private:
	/** Constructor. 
	 */
	Resources() = default;

	/** Parse the archive at the given path (using miniz), listing all files it contains.
	 \param archivePath the path to the archive
	 */
	void parseArchive(const std::string & archivePath);

	/** Parse the directory at the given path (using tinydir), listing all files it contains.
	 \param directoryPath the path to the directory
	 */
	void parseDirectory(const std::string & directoryPath);

	/** Expand an image name in its path, testing all possibles extensions.
	 \param name the name of the image
	 \return the image path
	 */
	std::string getImagePath(const std::string & name);

	/** Expand a cubemap base name in its faces paths, testing all possibles extensions.
	 \param name the base name of the cubemap
	 \return a list of each face path
	 */
	std::vector<std::string> getCubemapPaths(const std::string & name);

	/** Load raw binary data from a resource file
	 \param path the path to the file
	 \param size will contain the number of bytes loaded from the file
	 \return a pointer to the file binary data
	 */
	char * getRawData(const std::string & path, size_t & size);

public:
	/** Get a text file resource.
	 \param filename the file name
	 \return the string content of the file
	 */
	std::string getString(const std::string & filename);

	/** Get a geometric mesh resource.
	 \param name the mesh file name
	 \param mode denote if data will be available in the CPU and/or GPU memory
	 \return the mesh informations
	 */
	const Mesh * getMesh(const std::string & name, Storage mode);

	/** Get a 2D texture resource. Automatically handle custom mipmaps if present.
	 \param name the texture base name
	 \param descriptor the texture layout to use
	 \param mode denote if data will be available in the CPU and/or GPU memory
	 \param refName the name to use for the texture in future calls
	 \return the texture informations
	 */
	const Texture * getTexture(const std::string & name, const Descriptor & descriptor, Storage mode, const std::string & refName = "");

	/** Get an existing 2D texture resource.
	 \param name the texture base name
	 \return the texture informations
	 */
	const Texture * getTexture(const std::string & name);

	/** Get an OpenGL program resource.
	 \param name the name of all the program shaders
	 \param useGeometryShader should the program use a geometry shader
	 \return the program informations
	 \todo Merge with the one below
	 */
	Program * getProgram(const std::string & name, bool useGeometryShader = false);

	/** Get an OpenGL program resource.
	 \param name the name to represent the program
	 \param vertexName the name of the vertex shader
	 \param fragmentName the name of the fragment shader
	 \param geometryName the name of the optional geometry shader
	 \return the program informations
	 */
	Program * getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName = "");

	/** Get an OpenGL program resource for 2D screen processing. It will use GLSL::Vert::Passthrough as a vertex shader.
	 \param name the name of the fragment shader
	 \return the program informations
	 \see GLSL::Vert::Passthrough
	 */
	Program * getProgram2D(const std::string & name);

	/** Load a font metadata and texture atlas from the resources.
	 \param name the font base name
	 \return the font data
	 */
	Font * getFont(const std::string & name);

	/** Load raw binary data from an external file
	 \param path the path to the file on disk
	 \param size will contain the number of bytes loaded from the file
	 \return a pointer to the file binary data
	 */
	static char * loadRawDataFromExternalFile(const std::string & path, size_t & size);

	/** Load text data from an external file
	 \param path the  path to the file on disk
	 \return the file string content
	 \note Mainly used to load configuration or user selected files.
	 */
	static std::string loadStringFromExternalFile(const std::string & path);

	/** Write raw binary data to an external file
	 \param path the  path to the file on disk
	 \param rawContent a pointer to the file binary data
	 \param size will contain the number of bytes loaded from the file
	 */
	static void saveRawDataToExternalFile(const std::string & path, char * rawContent, size_t size);

	/** Write text data to an external file
	 \param path the  path to the file on disk
	 \param content the string to save
	 */
	static void saveStringToExternalFile(const std::string & path, const std::string & content);

	/** Check if a file exists on disk.
	 \param path the  path to the file on disk
	 \return true if the file exists.
	 */
	static bool externalFileExists(const std::string & path);

	/** Query all resource files with a given extension.
	 \param extension the extension of the files to list
	 \param files will contain the file names and their paths
	 */
	void getFiles(const std::string & extension, std::map<std::string, std::string> & files) const;

private:
	/** Destructor (disabled). */
	~Resources() = default;

	std::map<std::string, std::string> _files; ///< Listing of available files and their paths.
	std::map<std::string, Texture> _textures;  ///< Loaded textures, identified by name.
	std::map<std::string, Mesh> _meshes;	   ///< Loaded meshes, identified by name.
	std::map<std::string, Font> _fonts;		   ///< Loaded font infos, identified by name.
	std::map<std::string, Program> _programs;  ///< Loaded shader programs, identified by name.
};
