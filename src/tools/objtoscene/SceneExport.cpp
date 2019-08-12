#include "SceneExport.hpp"
#include "resources/Image.hpp"


namespace SceneExport {

	const int textureSize = 8;
	
	int saveColor(const std::string & outputPath, const glm::vec3 & color){
		Image combinedImage(textureSize, textureSize, 3);
		for(int y = 0; y < textureSize; ++y){
			for(int x = 0; x < textureSize; ++x){
				combinedImage.rgb(x, y) = color;
			}
		}
		return ImageUtilities::saveLDRImage(outputPath, combinedImage, false);
	}
	
	Material saveMaterial(const std::string & baseName, const CompositeObj::Material & material, const std::string & outputDirPath){
		
		// Check which textures are available.
		const bool hasTextureColor  = !material.colorTexturePath.empty();
		const bool hasTextureNormal = !material.normalTexturePath.empty();
		const bool hasTextureRough  = !material.roughTexturePath.empty();
		const bool hasTextureSpec   = !material.specTexturePath.empty();
		const bool hasTextureMetal  = !material.metalTexturePath.empty();
		const bool hasTextureAlpha  = !material.alphaTexturePath.empty();
		const bool hasTextureDisplacement = !material.displacementTexturePath.empty();
		
		// Basic material info.
		Material outMaterial;
		outMaterial.colorName      = baseName + "_texture_color";
		outMaterial.normalName     = baseName + "_texture_normal";
		outMaterial.roughMetAoName = baseName + "_texture_rough_met_ao";
		outMaterial.depthName      = hasTextureDisplacement ? (baseName + "_texture_depth") : "";
		outMaterial.hasAlpha = hasTextureAlpha;
		
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
		
		if(hasTextureColor && hasTextureAlpha){
			// Load both images, which should have the same size.
			Image colorMap, maskMap;
			ImageUtilities::loadImage(material.colorTexturePath, 3, false, true, colorMap);
			ImageUtilities::loadImage(material.alphaTexturePath, 1, false, true, maskMap);
			// Safety check.
			if(colorMap.width != maskMap.width || colorMap.height != maskMap.height){
				Log::Warning() << "Mask and color images have different sizes, keeping only color." << std::endl;
				ImageUtilities::saveLDRImage(outputColorPath, colorMap, false);
			} else {
				// Combine both.
				Image combinedImage(colorMap.width, colorMap.height, 4);
				for(unsigned int y = 0; y < colorMap.height; ++y){
					for(unsigned int x = 0; x < colorMap.width; ++x){
						combinedImage.rgba(x, y) = glm::vec4(colorMap.rgb(x,y), maskMap.r(x,y));
					}
				}
				ImageUtilities::saveLDRImage(outputColorPath, combinedImage, false);
			}
			
		} else if(hasTextureColor){
			// Just copy the image.
			Image colorMap;
			ImageUtilities::loadImage(material.colorTexturePath, 3, false, true, colorMap);
			ImageUtilities::saveLDRImage(outputColorPath, colorMap, false);
			
		} else if(hasTextureAlpha){
			// Load alpha.
			Image maskMap;
			ImageUtilities::loadImage(material.alphaTexturePath, 1, false, true, maskMap);
			// Fill in with material/default color.
			const glm::vec3 color = material.hasColor ? material.color : glm::vec3(1.0f);
			Image combinedImage(maskMap.width, maskMap.height, 4);
			for(unsigned int y = 0; y < maskMap.height; ++y){
				for(unsigned int x = 0; x < maskMap.width; ++x){
					combinedImage.rgba(x, y) = glm::vec4(color, maskMap.r(x,y));
				}
			}
			ImageUtilities::saveLDRImage(outputColorPath, combinedImage, false);
			
		} else if(material.hasColor){
			// Save a small color texture.
			saveColor(outputColorPath, material.color);
			
		} else {
			outMaterial.colorName = "default_color";
		}
		
		
		// Normal export. Possible cases:
		// 		- normal map
		// 		- none
		
		if(hasTextureNormal){
			// Copy normal map.
			Image normalMap;
			ImageUtilities::loadImage(material.normalTexturePath, 3, false, true, normalMap);
			ImageUtilities::saveLDRImage(outputNormalPath, normalMap, false);
		} else {
			outMaterial.normalName = "default_normal";
		}
		
		
		// Roughness/metalness/ambient occlusion export. Possible cases:
		//		- roughness map and metalness map
		//		- roughness map
		//		- metalness map
		//		- none
		
		if((hasTextureRough || hasTextureSpec)){
			
			// First, build the roughness map from existing roughness map or specular map.
			Image roughImage;
			if(hasTextureRough){
				ImageUtilities::loadImage(material.roughTexturePath, 1, false, true, roughImage);
			} else {
				// Load specular RGB, compute roughness as reverse of average.
				Image specImage;
				ImageUtilities::loadImage(material.specTexturePath, 3, false, true, specImage);
				roughImage = Image(specImage.width, specImage.height, 1);
				for(unsigned int y = 0; y < specImage.height; ++y){
					for(unsigned int x = 0; x < specImage.width; ++x){
						const glm::vec3 & spec = specImage.rgb(x,y);
						roughImage.r(x, y) = 1.0f-(spec.x+spec.y+spec.z)/3.0f;
					}
				}
			}
			
			// If possible, load metalness map if it has the same size as the roughness.
			Image metalImage;
			bool scalarMetal = !hasTextureMetal;
			if(hasTextureMetal){
				ImageUtilities::loadImage(material.metalTexturePath, 1, false, true, metalImage);
				// Safety check, fallback to scalar metal.
				if(metalImage.width != roughImage.width || metalImage.height != roughImage.height){
					Log::Warning() << "Roughness/specular and metalness images have different sizes, using 0.0 metalness." << std::endl;
					scalarMetal = true;
				}
			}
			const float defaultMetal = material.hasMetal ? material.metal : 0.0f;
			
			// Merge the roughness map and the metal map/scalar.
			Image roughMetAo(roughImage.width, roughImage.height, 3, 0.0f);
			for(unsigned int y = 0; y < roughImage.height; ++y){
				for(unsigned int x = 0; x < roughImage.width; ++x){
					const float metalness = scalarMetal ? defaultMetal : metalImage.r(x,y);
					roughMetAo.rgb(x, y) = glm::vec3(roughImage.r(x,y), metalness, 1.0f);
				}
			}
			ImageUtilities::saveLDRImage(outputRmaoPath, roughMetAo, false);
			
		} else if(hasTextureMetal){
			// Load metal image.
			Image metalImage;
			ImageUtilities::loadImage(material.metalTexturePath, 1, false, true, metalImage);
			// Fill in with material/default roughness.
			const float scalarRoughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
			Image roughMetAo(metalImage.width, metalImage.height, 3, 0.0f);
			for(unsigned int y = 0; y < metalImage.height; ++y){
				for(unsigned int x = 0; x < metalImage.width; ++x){
					roughMetAo.rgb(x, y) = glm::vec3(scalarRoughness, metalImage.r(x,y), 1.0f);
				}
			}
			ImageUtilities::saveLDRImage(outputRmaoPath, roughMetAo, false);
			
		} else if(material.hasRough || material.hasSpec || material.hasMetal){
			const float roughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
			const float metalness = material.hasMetal ? material.metal : 0.0f;
			saveColor(outputRmaoPath, {roughness, metalness, 1.0f});
			
		} else {
			outMaterial.roughMetAoName = "default_rough_met_ao";
		}
		
		
		// Depth map export (for parallax mapping). Possible cases:
		// 	- depth map
		// 	- none
		
		if(hasTextureDisplacement){
			Image depthMap;
			ImageUtilities::loadImage(material.displacementTexturePath, 1, false, true, depthMap);
			ImageUtilities::saveLDRImage(outputDepthPath, depthMap, false);
		}
		
		return outMaterial;
	}

	
	int saveDescription(const std::vector<CompositeObj::Object> & objects, const std::map<std::string, Material> & materials, const std::string & outputPath){
		
		std::ofstream sceneFile(outputPath);
		if(!sceneFile.is_open()){
			Log::Error() << "Unable to generate scene file." << std::endl;
			return 1;
		}
		// Scene environment infos.
		sceneFile << "* scene:" << std::endl;
		sceneFile << "\tprobe: rgbcube: default_cube" << std::endl;
		sceneFile << "\tirradiance: default_shcoeffs" << std::endl ;
		sceneFile << "* background:" << std::endl;
		sceneFile << "\tcolor: 0.0,0.0,0.0" << std::endl;
		sceneFile << std::endl;
		
		// Objects.
		for(const auto & object : objects){
			sceneFile << "* object:" << std::endl;
			sceneFile << "\tmesh: " << object.name << std::endl;
			sceneFile << "\tshadows: true" << std::endl;
			
			// Material infos.
			if(materials.count(object.material) > 0){
				const auto & materialDetails = materials.at(object.material);
				// Pick the type based on available infos.
				std::string typeName = "PBRRegular";
				if(!materialDetails.depthName.empty()){
					typeName = "PBRParallax";
				} else if(object.mesh.texcoords.empty()){
					typeName = "PBRNoUVs";
				}
				sceneFile << "\ttype: " << typeName << std::endl;
				sceneFile << "\tmasked: " << (materialDetails.hasAlpha ? "true" : "false") << std::endl;
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
