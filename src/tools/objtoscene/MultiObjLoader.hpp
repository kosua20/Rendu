#pragma once

#include "Common.hpp"
#include "helpers/TextUtilities.hpp"
#include "resources/MeshUtilities.hpp"

/**
 \brief OBJ material descriptor.
 \ingroup ObjToScene
 */
struct ObjMaterial {
	// Textures.
	std::string normalTexturePath;
	std::string alphaTexturePath;
	std::string displacementTexturePath;
	std::string colorTexturePath;
	std::string roughTexturePath;
	std::string metalTexturePath;
	std::string specTexturePath;
	
	// Scalars.
	glm::vec3 color = glm::vec3(0.0f);
	float rough = 0.0f;
	float metal = 0.0f;
	float spec  = 0.0f;
	
	// Have some of the scalars been set?
	bool hasColor = false;
	bool hasRough = false;
	bool hasMetal = false;
	bool hasSpec = false;
};

/**
 \brief Associate a mesh and a material.
 \ingroup ObjToScene
 */
struct ObjMaterialMesh {
	
	Mesh mesh; ///< The mesh.
	std::string name; ///< Name of the object.
	std::string material; ///< Name of the material.
	
	/**
	 Constructor.
	 \param aName the name of the object.
	 */
	ObjMaterialMesh(const std::string & aName){
		name = aName;
	}
};

int loadCompositeObj(const std::string & filePath, std::vector<ObjMaterialMesh>& objects, std::map<std::string, ObjMaterial>& materials);
