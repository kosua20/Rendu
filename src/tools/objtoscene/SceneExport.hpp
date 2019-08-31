#pragma once

#include "CompositeObj.hpp"
#include "system/System.hpp"
#include "Common.hpp"
#include <map>

/**
 \brief Helpers to export a Rendu scene.
 \ingroup ObjToScene
 */
namespace SceneExport {

/** \brief Contain exported texture infos for a given material.
	 */
struct Material {
	std::string colorName;		///< Color texture name.
	std::string normalName;		///< Normal map name.
	std::string roughMetAoName; ///< Roughness-metalness-ambient occlusion texture name.
	std::string depthName;		///< Optional depth map.
	bool hasAlpha = false;		///< Alpha mask.
};

/** Save a small colored texture.
	 \param outputPath the output image file path
	 \param color the color to store in the texture
	 \return an error code or 0
	 */
int saveColor(const std::string & outputPath, const glm::vec3 & color);

/** Save a material parameters as a series of textures.
	 \param baseName material base name
	 \param material the material to export
	 \param outputDirPath the output directory
	 \return the exported material information
	 */
Material saveMaterial(const std::string & baseName, const CompositeObj::Material & material, const std::string & outputDirPath);

/** Save a scene description, listing all objects with materials. The file can then be decoded by Codable objects.
	 \param objects the scene objects
	 \param materials the scene materials
	 \param outputPath the destination file
	 \return an error code or 0
	 */
int saveDescription(const std::vector<CompositeObj::Object> & objects, const std::map<std::string, Material> & materials, const std::string & outputPath);

}
