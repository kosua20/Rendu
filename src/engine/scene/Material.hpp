#pragma once

#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Represent a surface material, including textures describing the surface parameters.
 \ingroup Scene
 \details Different predefined materials can be used, with predefined parameters stored in ordered textures.

 __Regular:__ GGX material, dielectric or conductor.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion (RGB8)

 __Parallax:__ GGX material with parallax occlusion mapping.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion (RGB8)
	- Texture 3: Depth (R8)

 __Clearcoat:__ GGX material with an additional dielectric transparent layer overlayed top.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion (RGB8)
	- Texture 3: Clear coat strength, clear coat roughness (RG8)

 __Anisotropic:__ GGX material with anisotropic specular behavior.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion (RGB8)
	- Texture 3: Anisotropy direction (2D unit vector), anisotropy strength (scaled to [-1,1] parameterization) (RGB8)

 __Sheen:__ Cloth-like material, assumed to be a dielectric.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, sheen strength, ambient occlusion (RGB8)
	- Texture 3: Sheen color, sheen roughness (RGBA8)

 __Iridescent:__ GGX material with thin-film interferences on the surface.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion (RGB8)
	- Texture 3: Thin film refraction index (scaled to [1.2,2.4]), thin film thickness (scaled to [300nm,800nm]) (RG8)

 __Subsurface:__ Dieletric GGX material with subsurface scattering/transmission.
	- Texture 0: Albedo, alpha cutout (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, metalness, ambient occlusion, thickness (RGBA8)
	- Texture 3: Subsurface color, subsurface roughness (RGBA8)

 __Emissive:__ light emitting material, assume to be optionally covered by a transparent dielectric
	- Texture 0: Emissive color, alpha cutout (RGBA32)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness (R8)

 __Transparent:__ transparent object with a dieletric surface
	- Texture 0: Albedo, opacity (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, unused, ambient occlusion (RGB8)

 __TransparentIrid:__ transparent object with a dieletric surface and thin film interferences
	- Texture 0: Albedo, opacity (RGBA8)
	- Texture 1: Normal map (RGB8)
	- Texture 2: Roughness, unused, ambient occlusion (RGB8)
	- Texture 3: Thin film refraction index (scaled to [1.2,2.4]), thin film thickness (scaled to [300nm,800nm]) (RG8)

 __None:__ any parameters can be specified depending on the shader used in the renderer code.

 */
class Material {

public:
	/// \brief Type of shading/effects.
	enum Type : int {
		None = 0,  	 ///< Any type of shading.
		Regular,  	 ///< PBR shading.
		Parallax, 	 ///< PBR with parallax mapping.
		Clearcoat,   ///< PBR shading with an additional clear coat specular layer.
		Anisotropic, ///< PBR shading with an anisotropic BRDF.
		Sheen, 		 ///< PBR shading with a sheen BRDF.
		Iridescent,  ///< PBR shading with iridescent Fresnel.
		Subsurface,  ///< PBR shading with subsurface scattering.
		Emissive,  	 ///< Emissive objects (no diffuse shading).
		Transparent, ///< Transparent object.
		TransparentIrid, ///< Transparent object with iridescent Fresnel.
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

	/** Register a new parameter.
	 \param param the values to add
	 */
	void addParameter(const glm::vec4& param);

	/** Textures array getter.
	 \return a vector containing the textures associated to the material
	 */
	const std::vector<const Texture *> & textures() const { return _textures; }

	/** Parameters array getter.
	 \return a vector containing the parameters associated to the material
	 */
	const std::vector<glm::vec4> & parameters() const { return _parameters; }

	/** Type getter.
	 \return the type of material
	 \note This can be used in different way by different applications.
	 */
	const Type & type() const { return _material; }

	/** Is the surface from both sides.
	 \return a boolean denoting if the surface has two sides
	 */
	bool twoSided() const { return _twoSided; }

	/** Should an alpha clip mask be applied when rendering the surface (for leaves or chains for instance).
	 \return a boolean denoting if masking should be applied
	 */
	bool masked() const { return _masked; }

	/** \return the name of the material */
	const std::string& name() const { return _name; }

	/** Setup a material parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 type: materialtype
	 twosided: bool
	 masked: bool
	 textures:
	 	- texturetype: ...
	 	- ...
	 parameters:
		- R,G,B,A
		- ...
	 \endverbatim
	 \param params the parameters tuple
	 \param options data loading and storage options
	 \return decoding status	 
	 */
	bool decode(const KeyValues & params, Storage options);

	/** Generate a key-values representation of the material. See decode for the keywords and layout.
	\return a tuple representing the material.
	*/
	virtual KeyValues encode() const;
	
	/** Destructor.*/
	virtual ~Material() = default;

	/** Copy constructor.*/
	Material(const Material &) = delete;

	/** Copy assignment.
	 \return a reference to the material assigned to
	 */
	Material & operator=(const Material &) = delete;

	/** Move constructor.*/
	Material(Material &&) = default;

	/** Move assignment.
	 \return a reference to the material assigned to
	 */
	Material & operator=(Material &&) = default;

protected:

	std::vector<const Texture *> _textures;	///< Textures used by the material.
	std::vector<glm::vec4> _parameters;	 ///< Parameters used by the material.
	std::string _name;					 ///< The material name.
	Type _material   = Type::None;		 ///< The material type.
	bool _twoSided   = false;			 ///< Should faces of objects be visible from the two sides.
	bool _masked	 = false;			 ///< The material uses alpha cutout.

};

STD_HASH(Material::Type);
