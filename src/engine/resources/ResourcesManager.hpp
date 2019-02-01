#ifndef ResourcesManager_h
#define ResourcesManager_h
#include "../Common.hpp"
#include "../graphics/GLUtilities.hpp"
#include "../graphics/ProgramInfos.hpp"

/**
 \brief The Resources manager is responsible for all resources loading and setup.
 \details It provides an abstraction over the file system: resources can be loaded directly from files on disk, or from a zip archive.
 \ingroup Resources
 */
class Resources {
	
	friend class ImageUtilities;
	
public:
	
	/// \brief Shader types
	enum ShaderType {
		Vertex, Fragment, Geometry
	};
	
	/** Singleton accessor.
	 \return the resources manager singleton
	 */
	static Resources& manager();
	
	/** Default resources root path or archive path.
	 */
	static std::string defaultPath;
	
private:
	
	/** Constructor. Parse the directory or archive structure at the given path.
	 \param root the resources root path
	 */
	Resources(const std::string & root);
	
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
	const std::string getImagePath(const std::string & name);
	
	/** Expand a cubemap base name in its faces paths, testing all possibles extensions.
	 \param name the base name of the cubemap
	 \return a list of each face path
	 */
	const std::vector<std::string> getCubemapPaths(const std::string & name);
	
	/** Load raw binary data from a resource file
	 \param path the path to the file
	 \param size will contain the number of bytes loaded from the file
	 \return a pointer to the file binary data
	 */
	char * getRawData(const std::string & path, size_t & size);
	
public:
	
	
	/** Add another resources directory/archive.
	 \param path the path to the additional directory/archive to parse
	 */
	void addResources(const std::string & path);
	
	/** Get a text file resource.
	 \param filename the file name
	 \return the string content of the file
	 */
	const std::string getString(const std::string & filename);
	
	/** Get a geometric mesh resource.
	 \param name the mesh file name
	 \return the mesh informations
	 */
	const MeshInfos getMesh(const std::string & name);
	
	/** Get a 2D texture resource. Automatically handle custom mipmaps if present.
	 \param name the texture base name
	 \param srgb should the texture be gamma corrected
	 \return the texture informations
	 */
	const TextureInfos getTexture(const std::string & name, bool srgb = true);
	
	/** Get a cubemap texture resource. Automatically handle custom mipmaps if present.
	 \param name the texture base name
	 \param srgb should the texture be gamma corrected
	 \return the texture informations
	 */
	const TextureInfos getCubemap(const std::string & name, bool srgb = true);
	
	/** Get a shader text resource.
	 \param name the shader file name
	 \param type the type of shader (detemrines the extension)
	 \return the shader content
	 */
	const std::string getShader(const std::string & name, const ShaderType & type);
	
	/** Get an OpenGL program resource.
	 \param name the name of all the program shaders
	 \param useGeometryShader should the program use a geometry shader
	 \return the program informations
	 */
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name, const bool useGeometryShader = false);
	
	/** Get an OpenGL program resource.
	 \param name the name to represent the program
	 \param vertexName the name of the vertex shader
	 \param fragmentName the name of the fragment shader
	 \param geometryName the name of the optional geometry shader
	 \return the program informations
	 */
	const std::shared_ptr<ProgramInfos> getProgram(const std::string & name, const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName = "");
	
	/** Get an OpenGL program resource for 2D screen processing. It will use GLSL::Vert::Passthrough as a vertex shader.
	 \param name the name of the fragment shader
	 \return the program informations
	 \see GLSL::Vert::Passthrough
	 */
	const std::shared_ptr<ProgramInfos> getProgram2D(const std::string & name);
	
	/** Reload all shader programs.
	 */
	void reload();
	
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
	static void saveRawDataToExternalFile(const std::string & path, char * rawContent, const size_t size);
	
	/** Write text data to an external file
	 \param path the  path to the file on disk
	 \param content the string to save
	 */
	static void saveStringToExternalFile(const std::string & path, const std::string & content);
	
	/** Trim characters from both ends of a string.
	 \param str the string to trim from
	 \param del the characters to delete
	 \return the trimmed string
	 */
	static std::string trim(const std::string & str, const std::string & del);
	
	/** Query all resource files with a given extension.
	 \param extension the extension of the files to list
	 \param files will contain the file names and their paths
	 */
	void getFiles(const std::string & extension, std::map<std::string, std::string> & files) const;
	
private:
	
	/** Destructor (disabled). */
	~Resources(){};
	
	/** Assignment operator (disabled). */
	Resources& operator= (const Resources&) = delete;
	
	/** Copy constructor (disabled). */
	Resources (const Resources&) = delete;
	
	
	std::map<std::string, std::string> _files; ///< Listing of available files and their paths.
	std::map<std::string, TextureInfos> _textures; ///< Loaded textures, identified by name.
	std::map<std::string, MeshInfos> _meshes; ///< Loaded meshes, identified by name.
	std::map<std::string, std::shared_ptr<ProgramInfos>> _programs; ///< Loaded shader programs, identified by name.
	
};

#endif
