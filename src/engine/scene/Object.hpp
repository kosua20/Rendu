#pragma once

#include "scene/Codable.hpp"
#include "scene/Animation.hpp"
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
	 \param mesh the geometric mesh infos
	 \param castShadows denote if the object should cast shadows
	 */
	Object(const Object::Type type, const MeshInfos * mesh, bool castShadows);
	
	/** Register a texture.
	 \param infos the texture infos to add
	 */
	void addTexture(const TextureInfos * infos);
	
	/** Add an animation to apply at each frame.
	 \param anim the animation to add
	 */
	void addAnimation(std::shared_ptr<Animation> anim);
	
	/** Update the object transformation matrix.
	 \param model the new model matrix
	 */
	void set(const glm::mat4 & model){ _model = model; }
	
	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	void update(double fullTime, double frameTime);
	
	/** Query the bounding box of the object.
	 \return the bounding box
	 \note For mesh space bounding box, call boundingBox on mesh().
	 */
	BoundingBox getBoundingBox() const;
	
	/** Mesh getter.
	 \return the mesh infos
	 */
	const MeshInfos * mesh() const { return _mesh; }
	
	/** Textures array getter.
	 \return a vector containing the infos of the textures associated to the object
	 */
	const std::vector<const TextureInfos *> & textures() const { return _textures; }
	
	/** Object pose getter.
	 \return the model matrix
	 */
	const glm::mat4 & model() const { return _model; }
	
	/** Type getter.
	 \return the type of the object
	 \note This can be used in different way by different applications.
	 */
	const Type & type() const { return _material; }
	
	/** Is the object casting a shadow.
	 \return a boolean denoting if the object is a caster
	 */
	bool castsShadow() const { return _castShadow; }
	
	void decode(const std::vector<KeyValues>& params);
	
private:
	
	const MeshInfos * _mesh; ///< Geometry of the object.
	std::vector<const TextureInfos *> _textures; ///< Textures used by the object.
	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (applied in order).
	glm::mat4 _model = glm::mat4(1.0f); ///< The transformation matrix of the 3D model.
	Type _material = Type::Common; ///< The material type.
	bool _castShadow = true; ///< Can the object casts shadows.
};
