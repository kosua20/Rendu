#pragma once

#include "Common.hpp"
#include "resources/ImageUtilities.hpp"
#include "resources/MeshUtilities.hpp"
#include "helpers/Interface.hpp"
#include "helpers/TextUtilities.hpp"
#include "MultiObjLoader.hpp"

// For each material, export the existing textures.
struct FinalMaterialInfos {
	std::string colorName;
	std::string normalName;
	std::string roughMetAoName;
	std::string depthName;
	bool hasDepth = false;
	bool hasNormal = false;
};

void saveMesh(Mesh & mesh,const std::string & filePath);

FinalMaterialInfos saveMaterial(const std::string & prefix, const ObjMaterial & material, const std::string & outputDirPath);

int saveSceneFile(const std::vector<ObjMaterialMesh> & objects, const std::map<std::string, FinalMaterialInfos> & materials, const std::string & outputFile);
