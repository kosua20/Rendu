#ifndef Object_h
#define Object_h

#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief Represent a 3D textured object.
 \ingroup Scene
 */
class Object {

public:

	/// \brief Type of shading/effects.
	enum Type : int {
		Skybox = 0, ///< \see GLSL::Vert::Skybox_gbuffer, GLSL::Frag::Skybox_gbuffer
		PBRRegular = 1, ///< \see GLSL::Vert::Object_gbuffer, GLSL::Frag::Object_gbuffer
		PBRParallax = 2, ///< \see GLSL::Vert::Parallax_gbuffer, GLSL::Frag::Parallax_gbuffer
		Common = 3
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
	
	/** Update the object transformation matrix.
	 \param model the new model matrix
	 */
	void update(const glm::mat4& model);
	
	/** Query the bounding box of the object.
	 \return the bounding box
	 \note For mesh space bounding box, call boundingBox on mesh().
	 */
	BoundingBox getBoundingBox() const;
	
	
	const MeshInfos * mesh() const { return _mesh; }
	
	const std::vector<TextureInfos *> & textures() const { return _textures; }
	
	const glm::mat4 & model() const { return _model; }
	
	const Type & type() const { return _material; }
	
	bool castsShadow() const { return _castShadow; }
	
private:
	
	const MeshInfos * _mesh; ///< Geometry of the object.
	std::vector<TextureInfos *> _textures; ///< Textures used by the object.
	glm::mat4 _model; ///< The transformation matrix of the 3D model.
	Type _material; ///< The material type.
	bool _castShadow; ///< Can the object casts shadows.

};

#endif
