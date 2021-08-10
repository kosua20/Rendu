#pragma once

#include "resources/Mesh.hpp"
#include "Common.hpp"

/**
 \brief Commposite OBJ files loading.
 \ingroup ObjToScene
 */
namespace CompositeObj {

/**
 \brief OBJ material descriptor.
*/
struct Material {
	// Textures.
	std::string normalTexturePath; 		///< Normal texture path.
	std::string alphaTexturePath;	 	///< Alpha texture path.
	std::string displacementTexturePath;///< Displacement texture path.
	std::string colorTexturePath;		///< Albedo texture path.
	std::string roughTexturePath;		///< Roughness texture path.
	std::string metalTexturePath;		///< Metalness texture path.
	std::string specTexturePath;		///< Specular texture path.

	// Scalars.
	glm::vec3 color = glm::vec3(0.0f);  ///< Albedo value.
	float rough		= 0.0f;	///< Roughness value.
	float metal		= 0.0f; ///< Metalness value.
	float spec		= 0.0f; ///< Specular value.

	// Have some of the scalars been set?
	bool hasColor = false; ///< Has an albedo value.
	bool hasRough = false; ///< Has a roughness value.
	bool hasMetal = false; ///< Has a metalness value.
	bool hasSpec  = false; ///< Has a specular value.
};

/**
 \brief Associate a mesh and a material.
*/
struct Object {

	Mesh mesh;			  ///< The mesh.
	std::string name;	 ///< Name of the object.
	std::string material; ///< Name of the material.

	/**
		 Constructor.
		 \param aName the name of the object.
		 */
	explicit Object(const std::string & aName) :
		mesh(aName), name(aName) {
	}
};

/** Load a multi-objects, multi-materials OBJ file.
	 \param filePath the path to the OBJ file
	 \param objects will be filled the objects infos
	 \param materials will be filled with the materials infos
	 \return an error code or 0
	 */
int load(const std::string & filePath, std::vector<Object> & objects, std::unordered_map<std::string, Material> & materials);

}
