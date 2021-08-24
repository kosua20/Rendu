#pragma once

#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Represent a surface material.
 \ingroup Scene
 */
class Material {

public:
	/// \brief Type of shading/effects.
	enum Type : int {
		None = 0,  ///< Any type of shading.
		Regular,  ///< PBR shading. \see GPU::Vert::Object_gbuffer, GPU::Frag::Object_gbuffer
		Parallax, ///< PBR with parallax mapping. \see GPU::Vert::Object_parallax_gbuffer, GPU::Frag::Object_parallax_gbuffer
		Emissive,  	 ///< Emissive objects (no shading, pure emitter)
		Transparent, ///< Transparent object
	};

	/** Constructor */
	Material() = default;

	/** Construct a new material.
	 \param type the type of shading and effects associated with this material
	 */
	Material(Type type);

	/** Register a texture.
	 \param infos the texture infos to add
	 */
	void addTexture(const Texture * infos);

	/** Textures array getter.
	 \return a vector containing the infos of the textures associated to the material
	 */
	const std::vector<const Texture *> & textures() const { return _textures; }

	/** Type getter.
	 \return the type of the object
	 \note This can be used in different way by different applications.
	 */
	const Type & type() const { return _material; }

	/** Are the object faces visible from both sides.
	 \return a boolean denoting if the faces have two sides
	 */
	bool twoSided() const { return _twoSided; }

	/** Should an alpha clip mask be applied when rendering the object (for leaves or chains for instance).
	 \return a boolean denoting if masking should be applied
	 */
	bool masked() const { return _masked; }

	const std::string& name() const { return _name; }

	/** Setup a material parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 type: materialtype
	 twosided: bool
	 masked: bool
	 textures:
	 	- texturetype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 \param options data loading and storage options
	 */
	virtual void decode(const KeyValues & params, Storage options);

	/** Generate a key-values representation of the material. See decode for the keywords and layout.
	\return a tuple representing the material.
	*/
	virtual KeyValues encode() const;
	
	/** Destructor.*/
	virtual ~Material() = default;

	/** Copy constructor.*/
	Material(const Material &) = delete;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Material & operator=(const Material &) = delete;

	/** Move constructor.*/
	Material(Material &&) = default;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Material & operator=(Material &&) = default;

protected:

	std::vector<const Texture *> _textures;				 ///< Textures used by the material.
	std::string _name;					 ///< The material name.
	Type _material   = Type::None;		 ///< The material type.
	bool _twoSided   = false;			 ///< Should faces of objects be visible from the two sides.
	bool _masked	 = false;			 ///< The material uses alpha cutout.

};

STD_HASH(Material::Type);
