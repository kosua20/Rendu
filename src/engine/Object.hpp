#ifndef Object_h
#define Object_h
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief Represent a 3D textured object.
 \ingroup Engine
 */
class Object {

public:

	/// \brief Type of shading/effects.
	enum Type {
		Skybox = 0, Regular = 1, Parallax = 2, Custom = 3
	};

	/** Constructor */
	Object();

	/** Construct a new object.
	 \param type the type of shading and effects to use when rendering this object
	 \param meshPath name of the geometric mesh to use
	 \param texturesPaths names and SRGB flags of the 2D textures to use
	 \param cubemapPaths names and SRGB flags of the cubemap textures to use
	 \param castShadows denote if the object should cast shadows
	 \warning The textures sRGB flag will only be honored if they are loaded from disk for the first time.
	 */
	Object(const Object::Type & type, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths = {}, bool castShadows = true);
	
	/** Construct a new object.
	 \param program the custom shader program to use when rendering this object
	 \param meshPath name of the geometric mesh to use
	 \param texturesPaths names and SRGB flags of the 2D textures to use
	 \param cubemapPaths names and SRGB flags of the cubemap textures to use
	 \warning The textures sRGB flag will only be honored if they are loaded from disk for the first time.
	 */
	Object(std::shared_ptr<ProgramInfos> & program, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths = {});
	
	/** Update the object transformation matrix.
	 \param model the new model matrix
	 */
	void update(const glm::mat4& model);
	
	/** Render the object using its textures and shading program.
	 \param view the camera view matrix
	 \param projection the camera projection matrix
	 */
	void draw(const glm::mat4& view, const glm::mat4& projection) const;
	
	/**
	 Just bind and draw the geometry, with no implicit shader or textures.
	 */
	void drawGeometry() const;
	
	/** Clean internal data */
	void clean() const;
	
	/** Query the bounding box of the object.
	 \return the bounding box
	 */
	BoundingBox getBoundingBox() const;
	
	/** Query if the object should cast shadows or not.
	 \return if it is a shadow caster
	 */
	bool castsShadow() const { return _castShadow; }
	
	/** Query the object transformation palcing it in world space.
	 \return the model matrix
	 */
	const glm::mat4 & model() const { return _model; }
	
private:
	
	std::shared_ptr<ProgramInfos> _program; ///< Shader responsible for the object rendering.
	MeshInfos _mesh; ///< Geometry of the object.
	
	std::vector<TextureInfos> _textures; ///< Textures used by the object.
	
	glm::mat4 _model; ///< The transformation matrix of the 3D model.
	
	int _material; ///< The material ID, based on shading effects.
	bool _castShadow; ///< Can the object casts shadows.

};

#endif
