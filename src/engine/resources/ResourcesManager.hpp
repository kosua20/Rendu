#pragma once

#include "graphics/GPUTypes.hpp"
#include "graphics/Program.hpp"
#include "resources/Font.hpp"
#include "resources/Mesh.hpp"
#include "Common.hpp"


/**
 \brief Storage and loading options.
 \ingroup Resources
 */
enum class Storage : uint {
	NONE = 0,
	GPU  = 1,		   ///< Store on the GPU
	CPU  = 2,		   ///< Store on the CPU
	BOTH = (GPU | CPU), ///< Store on both the CPU and GPU
	FORCE_FRAME = 4 ///< For meshes, force computation of a local frame
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

/// Define raw binary blob as vectors.
using Data = std::vector<char>;

/**
 \brief The Resources manager is responsible for all resources loading and setup.
 \details It provides an abstraction over the file system: resources can be loaded directly from files on disk, or from a zip archive.
 \ingroup Resources
 */
class Resources {

	/// Image class.
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

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Resources & operator=(const Resources &) = delete;

	/** Copy constructor (disabled). */
	Resources(const Resources &) = delete;

	/** Move assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Resources & operator=(Resources &&) = delete;

	/** Move constructor (disabled). */
	Resources(Resources &&) = delete;

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

	/** Expand a multi-layer image (array or 3D depending on the suffix) name in its slices path, testing all possibles extensions.
	 \param name the name of the layered image
	 \param suffix the suffix to append before the layer number
	 \return a list of the image paths
	 */
	std::vector<std::string> getLayeredPaths(const std::string & name, const std::string & suffix);

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

	/** Get a text file resource, following include directives.
	 \param filename the file name
	 \param names will contain the included file names
	 \return the string content of the file
	 */
	std::string getStringWithIncludes(const std::string & filename, std::vector<std::string> & names);

	/** Get a text file resource, following include directives.
	 \param filename the file name
	 \return the string content of the file
	 */
	std::string getStringWithIncludes(const std::string& filename);
	
	/** Get a geometric mesh resource.
	 \param name the mesh file name
	 \param options data loading and storage options
	 \return the mesh informations
	 */
	const Mesh * getMesh(const std::string & name, Storage options);

	/** Get a texture resource. Automatically handle custom mipmaps if present.
	 \param name the texture base name
	 \param descriptor the texture layout to use
	 \param options data loading and storage options
	 \param refName the name to use for the texture in future calls
	 \return the texture informations
	 \note If the name is the string representation of an RGB(A) color ("1.0,0.0,1.0" for instance), a constant color 2D texture will be allocated using the passed descriptor.
	 \note Cubemaps will be automatically detected using suffixes _nx, _ny, _nz, _px, _py, _pz.
	 \note 2D arrays will be automatically detected using suffix _sX where X=0,1..., 3D textures using suffix _zX where X=0,1...
	 */
	const Texture * getTexture(const std::string & name, const Descriptor & descriptor, Storage options, const std::string & refName = "");

	/** Get an existing texture resource.
	 \param name the texture base name
	 \return the texture informations
	 */
	const Texture * getTexture(const std::string & name);

	const Texture * getDefaultTexture(TextureShape shape);

	/** Get a GPU program resource.
	 \param name the name to represent the program
	 \param vertexName the name of the vertex shader
	 \param fragmentName the name of the fragment shader
	 \param geometryName the name of the optional geometry shader
	 \param tessControlName the name of the optional tessellation control shader
	 \param tessEvalName the name of the optional tessellation evaluation shader
	 \return the program informations
	 */
	Program * getProgram(const std::string & name, const std::string & vertexName = "", const std::string & fragmentName= "", const std::string & geometryName = "", const std::string & tessControlName = "", const std::string & tessEvalName = "");

	/** Get a GPU program resource for 2D screen processing. It will use GPU::Vert::Passthrough as a vertex shader.
	 \param name the name of the fragment shader
	 \return the program informations
	 \see GPU::Vert::Passthrough
	 */
	Program * getProgram2D(const std::string & name);

	/** Load a font metadata and texture atlas from the resources.
	 \param name the font base name
	 \return the font data
	 */
	Font * getFont(const std::string & name);

	/** Load arbitrary data from the resources.
	 \param filename the data file base name
	 \return the data (internally managed)
	 */
	const Data * getData(const std::string & filename);

public:

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

	struct FileInfos {
		std::string path;
		std::string name;

		FileInfos(const std::string& apath, const std::string& aname);
	};

	/** Query all resource files with a given extension.
	 \param extension the extension of the files to list
	 \param files will contain the file names and their paths
	 */
	void getFiles(const std::string & extension, std::vector<FileInfos> & files) const;

private:
	/** Destructor (disabled). */
	~Resources() = default;

	/** Additional program information for reloading. */
	struct ProgramInfos {
		
		/** Basic constructor.
		 \param vertex vertex shader name
		 \param fragment fragment shader name
		 \param geometry geometry shader name
		 \param tessControl tessellation control shader name
		 \param tessEval tessellation evaluation shader name
		 */
		ProgramInfos(const std::string & vertex, const std::string & fragment, const std::string & geometry, const std::string & tessControl, const std::string & tessEval);

		std::string vertexName; ///< Vertex shader filename.
		std::string fragmentName; ///< Fragment shader filename.
		std::string geomName; ///< Geometry shader filename.
		std::string tessContName; ///< Tessellation control shader filename.
		std::string tessEvalName; ///< Tessellation evaluation shader filename.
	};

	std::unordered_map<std::string, std::string> _files; ///< Listing of available files and their paths.
	std::unordered_map<std::string, Texture> _textures;  ///< Loaded textures, identified by name.
	std::unordered_map<std::string, Mesh> _meshes;	   ///< Loaded meshes, identified by name.
	std::unordered_map<std::string, Font> _fonts;		   ///< Loaded font infos, identified by name.
	std::unordered_map<std::string, Data> _blobs;  	   ///< Loaded binary blobs, identified by name.
	std::unordered_map<std::string, Program> _programs;  ///< Loaded shader programs, identified by name.
	std::unordered_map<std::string, ProgramInfos> _progInfos;  ///< Additional info to support shader reloading.
};
