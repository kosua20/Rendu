#include "../Config.hpp"
#include "../helpers/ImageUtilities.hpp"
#include "../helpers/Logger.hpp"
#include <stdio.h>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <fstream>

/// Spherical harmonics coefficients extractor for a HDR cubemap.

/// The main function

int main(int argc, char** argv) {
	
	// Arguments parsing.
	std::map<std::string, std::string> arguments;
	Config::parseFromArgs(argc, argv, arguments);
	if(arguments.count("map") == 0){
		Log::Error() << Log::Utilities << "Specify path to envmap." << std::endl;
		return 3;
	}
	const std::string rootPath = arguments["map"];

	// Paths for each side.
	const std::vector<std::string> paths { rootPath + "_px.exr", rootPath + "_nx.exr", rootPath + "_py.exr", rootPath + "_ny.exr", rootPath + "_pz.exr", rootPath + "_nz.exr" };
	
	/// Load cubemap sides.
	Log::Info() << Log::Utilities << "Loading envmap at path " << rootPath << " ..." << std::endl;
	
	float* sides[6];
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int channels = 3;
	for(size_t side = 0; side < 6; ++side){
		
		if(!ImageUtilities::isHDR(paths[side])){
			Log::Error() << Log::Resources << "Non HDR image at path " << paths[side] << "." << std::endl;
			return 4;
		}
		int ret = ImageUtilities::loadImage(paths[side].c_str(), width, height, channels, (void**)&(sides[side]), false);
		if (ret != 0) {
			Log::Error() << Log::Resources << "Unable to load the texture at path " << paths[side] << "." << std::endl;
			return 1;
		}
		
	}
	
	/// Indices conversions from cubemap UVs to direction.
	const std::vector<int> axisIndices = { 0, 0, 1, 1, 2, 2 };
	const std::vector<float> axisMul = { 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f};
	
	const std::vector<int> horizIndices = { 2, 2, 0, 0, 0, 0};
	const std::vector<float> horizMul = { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	
	const std::vector<int> vertIndices = { 1, 1, 2, 2, 1, 1};
	const std::vector<float> vertMul = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f};
	

	/// Spherical harmonics coefficients.
	Log::Info() << Log::Utilities << "Computing SH coefficients." << std::endl;
	glm::vec3 LCoeffs[9];
	for(int i = 0; i < 9; ++i){
		LCoeffs[i] = glm::vec3(0.0f,0.0f,0.0f);
	}
	
	const float y0 = 0.282095f;
	const float y1 = 0.488603f;
	const float y2 = 1.092548f;
	const float y3 = 0.315392f;
	const float y4 = 0.546274f;
	
	float denom = 0.0f;
	for(int i = 0; i < 6; ++i){
		for(unsigned int y = 0; y < height; ++y){
			for(unsigned int x = 0; x < width; ++x){
				
				const float v = -1.0f + 1.0f/float(width) + float(y) * 2.0f/float(width);
				const float u = -1.0f + 1.0f/float(width) + float(x) * 2.0f/float(width);

				glm::vec3 pos = glm::vec3(0.0f,0.0f,0.0f);
				pos[axisIndices[i]] = axisMul[i];
				pos[horizIndices[i]] = horizMul[i] * u;
				pos[vertIndices[i]] = vertMul[i] * v;
				pos = glm::normalize(pos);
	
				// Normalization factor.
				const float fTmp = 1.0f + u*u + v*v;
				const float weight = 4.0f/(sqrt(fTmp)*fTmp);
				denom += weight;
				
				// HDR color.
				const glm::vec3 hdr = weight * glm::vec3(sides[i][(y*width+x)*3+0],
														 sides[i][(y*width+x)*3+1],
														 sides[i][(y*width+x)*3+2]);
				
				// Y0,0  = 0.282095
				LCoeffs[0] += hdr * y0;
				// Y1,-1 = 0.488603 y
				LCoeffs[1] += hdr * (y1 * pos[1]);
				// Y1,0  = 0.488603 z
				LCoeffs[2] += hdr * (y1 * pos[2]);
				// Y1,1  = 0.488603 x
				LCoeffs[3] += hdr * (y1 * pos[0]);
				// Y2,-2 = 1.092548 xy
				LCoeffs[4] += hdr * (y2 * (pos[0] * pos[1]));
				// Y2,-1 = 1.092548 yz
				LCoeffs[5] += hdr * (y2 * pos[1] * pos[2]);
				// Y2,0  = 0.315392 (3z^2 - 1)
				LCoeffs[6] += hdr * (y3 * (3.0f * pos[2] * pos[2] - 1.0f));
				// Y2,1  = 1.092548 xz
				LCoeffs[7] += hdr * (y2 * pos[0] * pos[2]);
				// Y2,2  = 0.546274 (x^2 - y^2)
				LCoeffs[8] += hdr * (y4 * (pos[0] * pos[0] - pos[1] * pos[1]));
				
			}
		}
	}
	
	// Normalization.
	for(int i = 0; i < 9; ++i){
		LCoeffs[i] *= 4.0/denom;
	}
	
	/// Final coefficients.
	Log::Info() << Log::Utilities << "Computing final coefficients." << std::endl;
	
	// To go from radiance to irradiance, we need to apply a cosine lobe convolution on the sphere in spatial domain.
	// This can be expressed as a product in frequency (on the SH basis) domain, with constant pre-computed coefficients.
	// See:	Ramamoorthi, Ravi, and Pat Hanrahan. "An efficient representation for irradiance environment maps."
	//		Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.
	
	const float c1 = 0.429043f;
	const float c2 = 0.511664f;
	const float c3 = 0.743125f;
	const float c4 = 0.886227f;
	const float c5 = 0.247708f;
	
	glm::vec3 SCoeffs[9];
	SCoeffs[0] = c4 * LCoeffs[0] - c5 * LCoeffs[6];
	SCoeffs[1] = 2.0f * c2 * LCoeffs[1];
	SCoeffs[2] = 2.0f * c2 * LCoeffs[2];
	SCoeffs[3] = 2.0f * c2 * LCoeffs[3];
	SCoeffs[4] = 2.0f * c1 * LCoeffs[4];
	SCoeffs[5] = 2.0f * c1 * LCoeffs[5];
	SCoeffs[6] = c3 * LCoeffs[6];
	SCoeffs[7] = 2.0f * c1 * LCoeffs[7];
	SCoeffs[8] = c1 * LCoeffs[8];
	
	Log::Info() << Log::Utilities << "Done. " << std::endl;
	
	// Output.
	const std::string destinationPath = rootPath + "_shcoeffs.txt";
	std::ofstream outputFile(destinationPath);
	if(!outputFile.is_open()){
		Log::Error() << Log::Utilities << "Unable to open output file at path " << destinationPath << "." << std::endl;
		return 2;
	}
	
	for(int i = 0; i < 9; ++i){
		outputFile << SCoeffs[i][0] << " " << SCoeffs[i][1] << " " << SCoeffs[i][2] << std::endl;
 	}
	outputFile.close();
	return 0;
}


