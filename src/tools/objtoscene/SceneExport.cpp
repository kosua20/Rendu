#include "SceneExport.hpp"
#include "resources/Image.hpp"

namespace SceneExport {

const int textureSize = 8; ///< Default uniform texture size.

int saveColor(const std::string & outputPath, const glm::vec3 & color) {
	Image combinedImage(textureSize, textureSize, 3);
	for(int y = 0; y < textureSize; ++y) {
		for(int x = 0; x < textureSize; ++x) {
			combinedImage.rgb(x, y) = color;
		}
	}
	return combinedImage.save(outputPath, false);
}

Material saveMaterial(const std::string & baseName, const CompositeObj::Material & material, const std::string & outputDirPath) {

	// Check which textures are available.
	const bool hasTextureColor		  = !material.colorTexturePath.empty();
	const bool hasTextureNormal		  = !material.normalTexturePath.empty();
	const bool hasTextureRough		  = !material.roughTexturePath.empty();
	const bool hasTextureSpec		  = !material.specTexturePath.empty();
	const bool hasTextureMetal		  = !material.metalTexturePath.empty();
	const bool hasTextureAlpha		  = !material.alphaTexturePath.empty();
	const bool hasTextureDisplacement = !material.displacementTexturePath.empty();

	// Basic material info.
	Material outMaterial;
	outMaterial.colorName	  = baseName + "_texture_color";
	outMaterial.normalName	 = baseName + "_texture_normal";
	outMaterial.roughMetAoName = baseName + "_texture_rough_met_ao";
	outMaterial.depthName	  = hasTextureDisplacement ? baseName + "_texture_depth" : "";
	outMaterial.hasAlpha	   = hasTextureAlpha;

	// Output files.
	const std::string outputColorPath  = outputDirPath + outMaterial.colorName + ".png";
	const std::string outputNormalPath = outputDirPath + outMaterial.normalName + ".png";
	const std::string outputRmaoPath   = outputDirPath + outMaterial.roughMetAoName + ".png";
	const std::string outputDepthPath  = outputDirPath + outMaterial.depthName + ".png";

	// Color export. Possible cases:
	// 		- RGB texture and Alpha texture.
	// 		- RGB texture, no alpha
	// 		- RGB color and Alpha texture
	// 		- RGB color
	// 		- none

	if(hasTextureColor && hasTextureAlpha) {
		// Load both images, which should have the same size.
		Image colorMap, maskMap;
		colorMap.load(material.colorTexturePath, 3, false, true);
		maskMap.load(material.alphaTexturePath, 1, false, true);
		// Safety check.
		if(colorMap.width != maskMap.width || colorMap.height != maskMap.height) {
			Log::Warning() << "Mask and color images have different sizes, keeping only color." << std::endl;
			colorMap.save(outputColorPath, false);
		} else {
			// Combine both.
			Image combinedImage(int(colorMap.width), int(colorMap.height), 4);
			for(uint y = 0; y < combinedImage.height; ++y) {
				for(uint x = 0; x < combinedImage.width; ++x) {
					combinedImage.rgba(x, y) = glm::vec4(colorMap.rgb(x, y), maskMap.r(x, y));
				}
			}
			combinedImage.save(outputColorPath, false);
		}

	} else if(hasTextureColor) {
		// Just copy the image.
		Image colorMap;
		colorMap.load(material.colorTexturePath, 3, false, true);
		colorMap.save(outputColorPath, false);

	} else if(hasTextureAlpha) {
		// Load alpha.
		Image maskMap;
		maskMap.load(material.alphaTexturePath, 1, false, true);
		// Fill in with material/default color.
		const glm::vec3 color = material.hasColor ? material.color : glm::vec3(1.0f);
		Image combinedImage(maskMap.width, maskMap.height, 4);
		for(uint y = 0; y < maskMap.height; ++y) {
			for(uint x = 0; x < maskMap.width; ++x) {
				combinedImage.rgba(x, y) = glm::vec4(color, maskMap.r(x, y));
			}
		}
		combinedImage.save(outputColorPath, false);

	} else if(material.hasColor) {
		// Save a small color texture.
		saveColor(outputColorPath, material.color);

	} else {
		outMaterial.colorName = "default_color";
	}

	// Normal export. Possible cases:
	// 		- normal map
	// 		- none

	if(hasTextureNormal) {
		// Copy normal map.
		Image normalMap;
		normalMap.load(material.normalTexturePath, 3, false, true);
		normalMap.save(outputNormalPath, false);
	} else {
		outMaterial.normalName = "default_normal";
	}

	// Roughness/metalness/ambient occlusion export. Possible cases:
	//		- roughness map and metalness map
	//		- roughness map
	//		- metalness map
	//		- none

	if(hasTextureRough || hasTextureSpec) {

		// First, build the roughness map from existing roughness map or specular map.
		Image roughImage;
		if(hasTextureRough) {
			roughImage.load(material.roughTexturePath, 1, false, true);
		} else {
			// Load specular RGB, compute roughness as reverse of average.
			Image specImage;
			specImage.load(material.specTexturePath, 3, false, true);
			roughImage = Image(specImage.width, specImage.height, 1);
			for(uint y = 0; y < specImage.height; ++y) {
				for(uint x = 0; x < specImage.width; ++x) {
					const glm::vec3 & spec = specImage.rgb(x, y);
					roughImage.r(x, y)	 = 1.0f - (spec.x + spec.y + spec.z) / 3.0f;
				}
			}
		}

		// If possible, load metalness map if it has the same size as the roughness.
		Image metalImage;
		bool scalarMetal = !hasTextureMetal;
		if(hasTextureMetal) {
			metalImage.load(material.metalTexturePath, 1, false, true);
			// Safety check, fallback to scalar metal.
			if(metalImage.width != roughImage.width || metalImage.height != roughImage.height) {
				Log::Warning() << "Roughness/specular and metalness images have different sizes, using 0.0 metalness." << std::endl;
				scalarMetal = true;
			}
		}
		const float defaultMetal = material.hasMetal ? material.metal : 0.0f;

		// Merge the roughness map and the metal map/scalar.
		Image roughMetAo(roughImage.width, roughImage.height, 3, 0.0f);
		for(uint y = 0; y < roughImage.height; ++y) {
			for(uint x = 0; x < roughImage.width; ++x) {
				const float metalness = scalarMetal ? defaultMetal : metalImage.r(x, y);
				roughMetAo.rgb(x, y)  = glm::vec3(roughImage.r(x, y), metalness, 1.0f);
			}
		}
		roughMetAo.save(outputRmaoPath, false);

	} else if(hasTextureMetal) {
		// Load metal image.
		Image metalImage;
		metalImage.load(material.metalTexturePath, 1, false, true);
		// Fill in with material/default roughness.
		const float scalarRoughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
		Image roughMetAo(metalImage.width, metalImage.height, 3, 0.0f);
		for(uint y = 0; y < metalImage.height; ++y) {
			for(uint x = 0; x < metalImage.width; ++x) {
				roughMetAo.rgb(x, y) = glm::vec3(scalarRoughness, metalImage.r(x, y), 1.0f);
			}
		}
		roughMetAo.save(outputRmaoPath, false);

	} else if(material.hasRough || material.hasSpec || material.hasMetal) {
		const float roughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
		const float metalness = material.hasMetal ? material.metal : 0.0f;
		saveColor(outputRmaoPath, {roughness, metalness, 1.0f});

	} else {
		outMaterial.roughMetAoName = "default_rough_met_ao";
	}

	// Depth map export (for parallax mapping). Possible cases:
	// 	- depth map
	// 	- none

	if(hasTextureDisplacement) {
		Image depthMap;
		depthMap.load(material.displacementTexturePath, 1, false, true);
		depthMap.save(outputDepthPath, false);
	}

	return outMaterial;
}

int saveDescription(const std::vector<CompositeObj::Object> & objects, const std::map<std::string, Material> & materials, const std::string & outputPath) {

	std::ofstream sceneFile(outputPath);
	if(!sceneFile.is_open()) {
		Log::Error() << "Unable to generate scene file." << std::endl;
		return 1;
	}
	// Scene environment infos.
	sceneFile << "* scene:" << std::endl;
	sceneFile << "\tprobe: rgbcube: default_cube" << std::endl;
	sceneFile << "\tirradiance: default_shcoeffs" << std::endl;
	sceneFile << "* background:" << std::endl;
	sceneFile << "\tcolor: 0.0,0.0,0.0" << std::endl;
	sceneFile << std::endl;

	// Objects.
	for(const auto & object : objects) {
		sceneFile << "* object:" << std::endl;
		sceneFile << "\tmesh: " << object.name << std::endl;
		sceneFile << "\tshadows: true" << std::endl;

		// Material infos.
		if(materials.count(object.material) > 0) {
			const auto & materialDetails = materials.at(object.material);
			// Pick the type based on available infos.
			std::string typeName = "Regular";
			if(!materialDetails.depthName.empty()) {
				typeName = "Parallax";
			}

			sceneFile << "\ttype: " << typeName << std::endl;
			sceneFile << "\tmasked: " << (materialDetails.hasAlpha ? "true" : "false") << std::endl;
			sceneFile << "\tskipuvs: " << (object.mesh.texcoords.empty() ? "true" : "false") << std::endl;
			sceneFile << "\ttextures:" << std::endl;
			sceneFile << "\t\t- srgb: " << materialDetails.colorName << std::endl;
			sceneFile << "\t\t- rgb: " << materialDetails.normalName << std::endl;
			sceneFile << "\t\t- rgb: " << materialDetails.roughMetAoName << std::endl;
		}
		sceneFile << std::endl;
	}
	sceneFile.close();
	return 0;
}

}
