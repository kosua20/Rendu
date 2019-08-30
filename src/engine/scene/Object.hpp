#pragma once

#include "scene/Animation.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Represent a 3D textured object.
 \ingroup Scene
 */
class Object {

public:
	/// \brief Type of shading/effects.
	enum Type : int {
		Common = 0,  ///< Any type of shading.
		PBRRegular,  ///< PBR shading. \see GLSL::Vert::Object_gbuffer, GLSL::Frag::Object_gbuffer
		PBRParallax, ///< PBR with parallax mapping. \see GLSL::Vert::Object_parallax_gbuffer, GLSL::Frag::Object_parallax_gbuffer
		PBRNoUVs
	};

	/** Constructor */
	Object() = default;

	/** Construct a new object.
	 \param type the type of shading and effects to use when rendering this object
	 \param mesh the geometric mesh infos
	 \param castShadows denote if the object should cast shadows
	 */
	Object(Type type, const Mesh * mesh, bool castShadows);

	/** Register a texture.
	 \param infos the texture infos to add
	 */
	void addTexture(const Texture * infos);

	/** Add an animation to apply at each frame.
	 \param anim the animation to add
	 */
	void addAnimation(const std::shared_ptr<Animation> & anim);

	/** Update the object transformation matrix.
	 \param model the new model matrix
	 */
	void set(const glm::mat4 & model) { _model = model; }

	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	virtual void update(double fullTime, double frameTime);

	/** Query the bounding box of the object.
	 \return the bounding box
	 \note For mesh space bounding box, call boundingBox on mesh().
	 */
	BoundingBox boundingBox() const;

	/** Mesh getter.
	 \return the mesh infos
	 */
	const Mesh * mesh() const { return _mesh; }

	/** Textures array getter.
	 \return a vector containing the infos of the textures associated to the object
	 */
	const std::vector<const Texture *> & textures() const { return _textures; }

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

	/** Are the object faces visible from both sides.
	 \return a boolean denoting if the faces have two sides
	 */
	bool twoSided() const { return _twoSided; }

	/** Should an alpha clip mask be applied when rendering the object (for leaves or chains for instance).
	 \return a boolean denoting if masking should be applied
	 */
	bool masked() const { return _masked; }

	/** Setup an object parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 type: objecttype
	 mesh: meshname
	 translation: X,Y,Z
	 scaling: scale
	 orientation: axisX,axisY,axisZ angle
	 shadows: bool
	 twosided: bool
	 masked: bool
	 textures:
	 	- texturetype: ...
	 	- ...
	 animations:
	 	- animationtype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 \param mode the storage mode (CPU, GPU, both)
	 */
	virtual void decode(const KeyValues & params, Storage mode);

	/** Destructor.*/
	virtual ~Object() = default;

	/** Copy constructor.*/
	Object(const Object &) = default;

	/** Copy assignment. */
	Object & operator=(const Object &) = default;

	/** Move constructor.*/
	Object(Object &&) = default;

	/** Move assignment. */
	Object & operator=(Object &&) = default;

protected:
	const Mesh * _mesh = nullptr;						 ///< Geometry of the object.
	std::vector<const Texture *> _textures;				 ///< Textures used by the object.
	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (applied in order).
	glm::mat4 _model = glm::mat4(1.0f);					 ///< The transformation matrix of the 3D model.
	Type _material   = Type::Common;					 ///< The material type.
	bool _castShadow = true;							 ///< Can the object casts shadows.
	bool _twoSided   = false;							 ///< Should faces of the object be visible from the two sides.
	bool _masked	 = false;							 ///< The object RGB texture has a non-empty alpha channel.
};
