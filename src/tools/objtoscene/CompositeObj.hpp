#pragma once

#include "Common.hpp"
#include "system/TextUtilities.hpp"
#include "resources/Mesh.hpp"

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
	 */
	struct Object {
		
		Mesh mesh; ///< The mesh.
		std::string name; ///< Name of the object.
		std::string material; ///< Name of the material.
		
		/**
		 Constructor.
		 \param aName the name of the object.
		 */
		explicit Object(const std::string & aName) : name(aName) {
		}
	};

	/** Load a multi-objects, multi-materials OBJ file.
	 \param filePath the path to the OBJ file
	 \param objects will be filled the objects infos
	 \param materials will be filled with the materials infos
	 \return an error code or 0
	 */
	int load(const std::string & filePath, std::vector<Object>& objects, std::map<std::string, Material>& materials);

}
