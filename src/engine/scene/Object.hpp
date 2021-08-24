#pragma once

#include "scene/Animated.hpp"
#include "scene/Animation.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/// Foward declarations.
class Material;

/**
 \brief Represent a 3D textured object.
 \ingroup Scene
 */
class Object {

public:

	/** Constructor */
	Object() = default;

	/** Construct a new object.
	 \param mesh the geometric mesh infos
	 \param castShadows denote if the object should cast shadows
	 */
	Object(const Mesh * mesh, bool castShadows);

	/** Add an animation to apply at each frame.
	 \param anim the animation to add
	 */
	void addAnimation(const std::shared_ptr<Animation> & anim);

	/** Update the object transformation matrix.
	 \param model the new model matrix
	 */
	void set(const glm::mat4 & model);

	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	virtual void update(double fullTime, double frameTime);

	/** Query the bounding box of the object in world space.
	 \return the bounding box
	 \note For mesh space bounding box, call boundingBox on mesh().
	 */
	const BoundingBox & boundingBox() const;

	/** Mesh getter.
	 \return the mesh infos
	 */
	const Mesh * mesh() const { return _mesh; }

	/** Object pose getter.
	 \return the model matrix
	 */
	const glm::mat4 & model() const { return _model; }

	/** Is the object casting a shadow.
	 \return a boolean denoting if the object is a caster
	 */
	bool castsShadow() const { return _castShadow; }

	/** Should the object use its texture coordinates (if they exist)
	 \return a boolean denoting if the UV should be used
	 */
	bool useTexCoords() const { return !_skipUVs; }

	/** Check if the object is moving over time.
	 \return a boolean denoting if animations are applied to the object
	 */
	bool animated() const { return !_animations.empty(); }

	void setMaterial(const Material* material);

	const Material& material() const { return *_material; }

	const std::string& materialName() const { return _materialName; }

	/** Setup an object parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 material: material name
	 mesh: meshname
	 translation: X,Y,Z
	 scaling: scale
	 orientation: axisX,axisY,axisZ angle
	 shadows: bool
	 skipuvs: bool
	 animations:
	 	- animationtype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 \param options data loading and storage options
	 */
	virtual void decode(const KeyValues & params, Storage options);

	/** Generate a key-values representation of the object. See decode for the keywords and layout.
	\return a tuple representing the object.
	*/
	virtual KeyValues encode() const;
	
	/** Destructor.*/
	virtual ~Object() = default;

	/** Copy constructor.*/
	Object(const Object &) = delete;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Object & operator=(const Object &) = delete;

	/** Move constructor.*/
	Object(Object &&) = default;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Object & operator=(Object &&) = default;

protected:
	const Mesh * _mesh = nullptr;						 ///< Geometry of the object.
	// Material information.
	std::string _materialName;							///< Material name.
	const Material * _material = nullptr;				///< Material.
	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (applied in order).
	Animated<glm::mat4> _model { glm::mat4(1.0f) };		///< The transformation matrix of the 3D model, updated by the animations.
	mutable BoundingBox _bbox;							///< The world space object bounding box.
	bool _castShadow = true;			 ///< Can the object casts shadows.
	bool _skipUVs	 = false;			 ///< The object doesn't use UV coordinates.

	mutable bool _dirtyBbox  = true;	 ///< Has the bounding box been updated following an animation update.
};
