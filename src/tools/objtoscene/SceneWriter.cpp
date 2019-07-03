#include "SceneWriter.hpp"
#include "resources/ImageUtilities.hpp"

void saveMesh(Mesh & mesh, const std::string & filePath){
	std::ofstream objFile(filePath);
	if(!objFile.is_open()){
		Log::Error() << "Unable to create file at path \"" << filePath << "\"." << std::endl;
		return;
	}
	
	if(mesh.normals.empty()){
		MeshUtilities::computeNormals(mesh);
	}
	
	
	
	for(size_t pid = 0; pid < mesh.positions.size(); ++pid){
		const glm::vec3 & v = mesh.positions[pid];
		objFile << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}
	for(size_t pid = 0; pid < mesh.texcoords.size(); ++pid){
		const glm::vec2 & t = mesh.texcoords[pid];
		objFile << "vt " << t.x << " " << t.y << std::endl;
	}
	for(size_t pid = 0; pid < mesh.normals.size(); ++pid){
		const glm::vec3 & n = mesh.normals[pid];
		objFile << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
	}
	
	// If the mesh has no UVs, it's probably using a uniform color material. Set the UV for all vertices to 0.5,0.5.
	const bool hasTexCoords = !mesh.texcoords.empty();
	if(!hasTexCoords){
		objFile << "vt 0.5 0.5" << std::endl;
	}
	
	for(size_t tid = 0; tid < mesh.indices.size(); tid += 3){
		const std::string t0 = std::to_string(mesh.indices[tid+0] + 1);
		const std::string t1 = std::to_string(mesh.indices[tid+1] + 1);
		const std::string t2 = std::to_string(mesh.indices[tid+2] + 1);
		objFile << "f";
		objFile << " " << t0 << "/" << (hasTexCoords ? t0 : "1") << "/" << t0;
		objFile << " "  << t1 << "/" << (hasTexCoords ? t1 : "1") << "/" << t1;
		objFile << " "  << t2 << "/" << (hasTexCoords ? t2 : "1") << "/" << t2;
		objFile << std::endl;
	}
	
	objFile.close();
}



FinalMaterialInfos saveMaterial(const std::string & prefix, const ObjMaterial & material, const std::string & outputDirPath){
	
	FinalMaterialInfos finalMaterial;
	
	const bool hasTextureColor = !material.colorTexturePath.empty();
	const bool hasTextureNormal = !material.normalTexturePath.empty();
	const bool hasTextureRough = !material.roughTexturePath.empty();
	const bool hasTextureSpec = !material.specTexturePath.empty();
	const bool hasTextureMetal = !material.metalTexturePath.empty();
	const bool hasTextureAlpha = !material.alphaTexturePath.empty();
	const bool hasTextureDisplacement = !material.displacementTexturePath.empty();
	
	// We need 4 textures in the end: color+alpha, normal, rought_met_ao, and optionally depth.
	
	// Color
	// Possible cases: texcolor/no mask, texcolor/mask, color/mask, color, no color/mask.
	const std::string colorName = prefix + "_texture_color";
	if(hasTextureColor && hasTextureAlpha){
		Image colorMap, maskMap;
		ImageUtilities::loadImage(material.colorTexturePath, 3, false, true, colorMap);
		ImageUtilities::loadImage(material.alphaTexturePath, 1, false, true, maskMap);
		if(colorMap.width != maskMap.width || colorMap.height != maskMap.height){
			Log::Warning() << "Mask and color images have different sizes, keeping only color." << std::endl;
			ImageUtilities::saveLDRImage(outputDirPath + colorName + ".png", colorMap, false);
			finalMaterial.colorName = colorName;
		} else {
			Image combinedImage(colorMap.width, colorMap.height, 4);
			for(int y = 0; y < colorMap.height; ++y){
				for(int x = 0; x < colorMap.width; ++x){
					combinedImage.rgba(x, y) = glm::vec4(colorMap.rgb(x,y), maskMap.r(x,y));
				}
			}
			ImageUtilities::saveLDRImage(outputDirPath + colorName + ".png", combinedImage, false);
			finalMaterial.colorName = colorName;
		}
	} else if(hasTextureColor){
		Image colorMap;
		ImageUtilities::loadImage(material.colorTexturePath, 3, false, true, colorMap);
		ImageUtilities::saveLDRImage(outputDirPath + colorName + ".png", colorMap, false);
		finalMaterial.colorName = colorName;
	} else if(hasTextureAlpha){
		Image maskMap;
		ImageUtilities::loadImage(material.alphaTexturePath, 1, false, true, maskMap);
		const glm::vec3 color = material.hasColor ? material.color : glm::vec3(1.0f);
		Image combinedImage(maskMap.width, maskMap.height, 4);
		for(int y = 0; y < maskMap.height; ++y){
			for(int x = 0; x < maskMap.width; ++x){
				combinedImage.rgba(x, y) = glm::vec4(color, maskMap.r(x,y));
			}
		}
		ImageUtilities::saveLDRImage(outputDirPath + colorName + ".png", combinedImage, false);
		finalMaterial.colorName = colorName;
	} else if(material.hasColor){
		const glm::vec3 color = material.color;
		Image combinedImage(32, 32, 4);
		for(int y = 0; y < 32; ++y){
			for(int x = 0; x < 32; ++x){
				combinedImage.rgba(x, y) = glm::vec4(color, 1.0f);
			}
		}
		ImageUtilities::saveLDRImage(outputDirPath + colorName + ".png", combinedImage, false);
		finalMaterial.colorName = colorName;
	} else {
		finalMaterial.colorName = "default_color";
	}
	
	
	// Normal:
	// Possible cases: normal, no normal
	if(hasTextureNormal){
		Image normalMap;
		ImageUtilities::loadImage(material.normalTexturePath, 3, false, true, normalMap);
		const std::string name = prefix + "_texture_normal";
		ImageUtilities::saveLDRImage(outputDirPath + name + ".png", normalMap, false);
		finalMaterial.normalName = name;
		finalMaterial.hasNormal = true;
	} else {
		finalMaterial.normalName = "default_normal";
	}
	
	// Rough met ao:
	// if no roughness, use 1-specular.
	// leave ao blank
	// metal: has or has not.
	const std::string roughMetAoName = prefix + "_texture_rough_met_ao";
	if((hasTextureRough || hasTextureSpec)){
		Image roughImage;
		if(hasTextureRough){
			ImageUtilities::loadImage(material.roughTexturePath, 1, false, true, roughImage);
		} else {
			Image specImage;
			ImageUtilities::loadImage(material.specTexturePath, 3, false, true, specImage);
			roughImage = Image(specImage.width, specImage.height, 1);
			for(int y = 0; y < specImage.height; ++y){
				for(int x = 0; x < specImage.width; ++x){
					const glm::vec3 & spec = specImage.rgb(x,y);
					roughImage.r(x, y) = 1.0f-(spec.x+spec.y+spec.z)/3.0f;
				}
			}
		}
		Image metalImage;
		bool scalarMetal = !hasTextureMetal;
		if(hasTextureMetal){
			ImageUtilities::loadImage(material.metalTexturePath, 1, false, true, metalImage);
			if(metalImage.width != roughImage.width || metalImage.height != roughImage.height){
				Log::Warning() << "Roughness/specular and metalness images have different sizes, using 0 metalness." << std::endl;
				scalarMetal = true;
			}
		}
		const float defaultMetalness = material.hasMetal ? material.metal : 0.0f;
		
		Image roughMetAo(roughImage.width, roughImage.height, 3, 0.0f);
		for(int y = 0; y < roughImage.height; ++y){
			for(int x = 0; x < roughImage.width; ++x){
				const float metalness = scalarMetal ? defaultMetalness : metalImage.r(x,y);
				roughMetAo.rgb(x, y) = glm::vec3(roughImage.r(x,y), metalness, 1.0f);
			}
		}
		ImageUtilities::saveLDRImage(outputDirPath + roughMetAoName + ".png", roughMetAo, false);
		finalMaterial.roughMetAoName = roughMetAoName;
	} else if(hasTextureMetal){
		const float scalarRoughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
		Image metalImage;
		ImageUtilities::loadImage(material.metalTexturePath, 1, false, true, metalImage);
		Image roughMetAo(metalImage.width, metalImage.height, 3, 0.0f);
		for(int y = 0; y < metalImage.height; ++y){
			for(int x = 0; x < metalImage.width; ++x){
				roughMetAo.rgb(x, y) = glm::vec3(scalarRoughness, metalImage.r(x,y), 1.0f);
			}
		}
		ImageUtilities::saveLDRImage(outputDirPath + roughMetAoName + ".png", roughMetAo, false);
		finalMaterial.roughMetAoName = roughMetAoName;
	} else if(material.hasRough || material.hasSpec || material.hasMetal){
		const float scalarRoughness = material.hasRough ? material.rough : (material.hasSpec ? (1.0f - material.spec) : 0.5f);
		const float defaultMetalness = material.hasMetal ? material.metal : 0.0f;
		Image roughMetAo(32, 32, 3, 0.0f);
		for(int y = 0; y < 32; ++y){
			for(int x = 0; x < 32; ++x){
				roughMetAo.rgb(x, y) = glm::vec3(scalarRoughness, defaultMetalness, 1.0f);
			}
		}
		ImageUtilities::saveLDRImage(outputDirPath + roughMetAoName + ".png", roughMetAo, false);
		finalMaterial.roughMetAoName = roughMetAoName;
	} else {
		finalMaterial.roughMetAoName = "default_rough_met_ao";
	}
	
	// Depth
	if(hasTextureDisplacement){
		Image depthMap;
		ImageUtilities::loadImage(material.displacementTexturePath, 1, false, true, depthMap);
		const std::string name = prefix + "_texture_depth";
		ImageUtilities::saveLDRImage(outputDirPath + name + ".png", depthMap, false);
		finalMaterial.depthName = name;
		finalMaterial.hasDepth = true;
	}
	return finalMaterial;
}

int saveSceneFile(const std::vector<ObjMaterialMesh> & objects, const std::map<std::string, FinalMaterialInfos> & materials, const std::string & outputPath)
{
	std::ofstream sceneFile(outputPath);
	if(!sceneFile.is_open()){
		Log::Error() << "Unable to generate scene file." << std::endl;
		return 6;
	}
	sceneFile << "scene:" << std::endl;
	sceneFile << "\tbgcolor: 0.0,0.0,0.0" << std::endl;
	sceneFile << "\tprobe: rgbcube: default_cube" << std::endl;
	sceneFile << "\tirradiance: default_shcoeffs" << std::endl;
	for(const auto & object : objects){
		sceneFile << "object:" << std::endl;
		sceneFile << "\tmesh: " << object.name << std::endl;
		sceneFile << "\tshadows: true" << std::endl;
		if(materials.count(object.material) > 0){
			const auto & materialDetails = materials.at(object.material);
			std::string typeName = "PBRRegular";
			if(materialDetails.hasDepth){
				typeName = "PBRParallax";
			} else if(object.mesh.texcoords.empty()){
				typeName = "PBRNoUVs";
			}
			sceneFile << "\ttype: " << typeName << std::endl;
			sceneFile << "\ttextures:" << std::endl;
			sceneFile << "\t\tsrgb: " << materialDetails.colorName << std::endl;
			sceneFile << "\t\trgb: " << materialDetails.normalName << std::endl;
			sceneFile << "\t\trgb: " << materialDetails.roughMetAoName << std::endl;
			
		}
		sceneFile << std::endl;
	}
	sceneFile.close();
	return 0;
}
