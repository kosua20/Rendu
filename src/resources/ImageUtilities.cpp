#include "ImageUtilities.hpp"
#include "ResourcesManager.hpp"

#include <vector>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include <miniz/miniz.h>
#define TINYEXR_USE_MINIZ (0)
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

bool ImageUtilities::isHDR(const std::string & path){
	return path.substr(path.size()-4,4) == ".exr";
}

int ImageUtilities::loadImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, void **data, const bool flip, const bool externalFile){
	int ret = 0;
	if(isHDR(path)){
		ret = ImageUtilities::loadHDRImage(path, width, height, channels, (float**)data, flip, externalFile);
	} else {
		ret = ImageUtilities::loadLDRImage(path, width, height, channels, (unsigned char**)data, flip, externalFile);
	}
	return ret;
}

int ImageUtilities::loadLDRImage(const std::string &path, unsigned int & width, unsigned int & height, unsigned int & channels, unsigned char **data, const bool flip, const bool externalFile){
	
	size_t rawSize = 0;
	unsigned char * rawData;
	if(externalFile){
		rawData = (unsigned char*)(Resources::loadRawDataFromExternalFile(path, rawSize));
	} else {
		rawData = (unsigned char*)(Resources::manager().getRawData(path, rawSize));
	}
	
	if(rawData == NULL || rawSize == 0){
		return 1;
	}
	
	stbi_set_flip_vertically_on_load(flip);
	
	channels = 4;
	int localWidth = 0;
	int localHeight = 0;
	// Beware: the size has to be cast to int, imposing a limit on big file sizes.
	*data = stbi_load_from_memory(rawData, (int)rawSize, &localWidth, &localHeight, NULL, channels);
	free(rawData);
	
	if(*data == NULL){
		return 1;
	}
	
	width = (unsigned int)localWidth;
	height = (unsigned int)localHeight;
	
	return 0;
}

int ImageUtilities::loadHDRImage(const std::string &path, unsigned int & width, unsigned int & height, unsigned int & channels, float **data, const bool flip, const bool externalFile){
	
	// Code adapted from tinyEXR deprecated loadEXR.
	EXRVersion exr_version;
	EXRImage exr_image;
	EXRHeader exr_header;
	InitEXRHeader(&exr_header);
	InitEXRImage(&exr_image);
	
	size_t rawSize = 0;
	unsigned char * rawData;
	if(externalFile){
		rawData = (unsigned char*)(Resources::loadRawDataFromExternalFile(path, rawSize));
	} else {
		rawData = (unsigned char*)(Resources::manager().getRawData(path, rawSize));
	}
	
	if(rawData == NULL || rawSize == 0){
		return 1;
	}
	
	{
		int ret = ParseEXRVersionFromMemory(&exr_version, rawData, tinyexr::kEXRVersionSize);
		if (ret != TINYEXR_SUCCESS) {
			return ret;
		}
		
		if (exr_version.multipart || exr_version.non_image) {
			return TINYEXR_ERROR_INVALID_DATA;
		}
	}
	{
		int ret = ParseEXRHeaderFromMemory(&exr_header, &exr_version, rawData, rawSize, NULL);
		if (ret != TINYEXR_SUCCESS) {
			return ret;
		}
	}
	
	// Read HALF channel as FLOAT.
	for (int i = 0; i < exr_header.num_channels; i++) {
		if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
			exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}
	{
		int ret = LoadEXRImageFromMemory(&exr_image, &exr_header, rawData, rawSize, NULL);
		if (ret != TINYEXR_SUCCESS) {
			return ret;
		}
	}
	free(rawData);
	
	// RGBA
	int idxR = -1;
	int idxG = -1;
	int idxB = -1;
	int idxA = -1;
	for (int c = 0; c < exr_header.num_channels; c++) {
		if (strcmp(exr_header.channels[c].name, "R") == 0) {
			idxR = c;
		} else if (strcmp(exr_header.channels[c].name, "G") == 0) {
			idxG = c;
		} else if (strcmp(exr_header.channels[c].name, "B") == 0) {
			idxB = c;
		} else if (strcmp(exr_header.channels[c].name, "A") == 0) {
			idxA = c;
		}
	}
	
	if (idxR == -1 || idxG == -1 || idxB == -1) {
		// @todo { free exr_image }
		return TINYEXR_ERROR_INVALID_DATA;
	}
	
	width = exr_image.width;
	height = exr_image.height;
	channels = 3;
	
	*data = reinterpret_cast<float *>(malloc(channels * sizeof(float) * static_cast<size_t>(width) *
												  static_cast<size_t>(height)));
	
	for(int y = 0; y < exr_image.height; ++y){
		for (int x = 0; x < exr_image.width; ++x){
			const int destIndex = y * width + x;
			const int sourceIndex = flip ? ((height-1-y)*width+x) : destIndex;
			
			(*data)[channels * destIndex + 0] = reinterpret_cast<float **>(exr_image.images)[idxR][sourceIndex];
			(*data)[channels * destIndex + 1] = reinterpret_cast<float **>(exr_image.images)[idxG][sourceIndex];
			(*data)[channels * destIndex + 2] = reinterpret_cast<float **>(exr_image.images)[idxB][sourceIndex];
			// Ignore alpha.
		}
	}
	
	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);
	
	return 0;
}

int ImageUtilities::saveLDRImage(const std::string &path, const unsigned int width, const unsigned int height, const unsigned int channels, const unsigned char * data, const bool flip, const bool ignoreAlpha){
	
	stbi_flip_vertically_on_write(flip);
	// Temporary fix for stb_image_write issue with flipped PNGs when computing filters>2.
	stbi_write_force_png_filter = 1;
	
	int stride_in_bytes = width*channels;
	
	int ret = 1;
	if(ignoreAlpha && channels == 4){
		unsigned char * newData = new unsigned char[width*height*4];
		for(unsigned int i = 0; i < width*height; ++i){
			newData[4*i+0] = data[4*i+0];
			newData[4*i+1] = data[4*i+1];
			newData[4*i+2] = data[4*i+2];
			newData[4*i+3] = 255;
		}
		ret = stbi_write_png(path.c_str(), (int)width, (int)height, (int)channels, (const void*)newData, stride_in_bytes);
		delete [] newData;
	} else {
		ret = stbi_write_png(path.c_str(), (int)width, (int)height, (int)channels, (const void*)data, stride_in_bytes);
	}
	return ret == 0 ? 1 : 0; //...
}

int ImageUtilities::saveHDRImage(const std::string &path, const unsigned int width, const unsigned int height, const unsigned int channels, const float *data, const bool flip, const bool ignoreAlpha){
	
	// Assume at least 16x16 pixels.
	if (width < 16) return TINYEXR_ERROR_INVALID_ARGUMENT;
	if (height < 16) return TINYEXR_ERROR_INVALID_ARGUMENT;
	
	EXRHeader header;
	InitEXRHeader(&header);
	
	EXRImage image;
	InitEXRImage(&image);
	
	// Components: 1, 3, 3, 4
	int components = channels == 2 ? 3 : channels;
	image.num_channels = components;
	
	std::vector<float> images[4];
	
	if (components == 1) {
		images[0].resize(static_cast<size_t>(width * height));
		memcpy(images[0].data(), data, sizeof(float) * size_t(width * height));
		// TODO: support flipping.
	} else {
		images[0].resize(static_cast<size_t>(width * height));
		images[1].resize(static_cast<size_t>(width * height));
		images[2].resize(static_cast<size_t>(width * height));
		images[3].resize(static_cast<size_t>(width * height));
		
		// Split RGB(A)RGB(A)RGB(A)... into R, G and B(and A) layers
		// By default we try to always fill at least three channels.
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				const size_t destIndex = y * width + x;
				const size_t sourceIndex = flip ? ((height-1-y)*width+x) : destIndex;
				//images[0][destIndex] = data[static_cast<size_t>(components) * sourceIndex + 0];
				//images[1][destIndex] = data[static_cast<size_t>(components) * sourceIndex + 1];
				//images[2][destIndex] = data[static_cast<size_t>(components) * sourceIndex + 2];
				for(unsigned int j = 0; j < channels; ++j){
					images[j][destIndex] = data[static_cast<size_t>(channels) * sourceIndex + j];
				}
				for(unsigned int j = channels; j < 3; ++j){
					images[j][destIndex] = 0.0f;
				}
				if (components == 4) {
					images[3][destIndex] = ignoreAlpha ? 1.0f : data[static_cast<size_t>(channels) * sourceIndex + 3];
				}
			}
		}
	}
	
	float *image_ptr[4] = {0, 0, 0, 0};
	if (components == 4) {
		image_ptr[0] = &(images[3].at(0));  // A
		image_ptr[1] = &(images[2].at(0));  // B
		image_ptr[2] = &(images[1].at(0));  // G
		image_ptr[3] = &(images[0].at(0));  // R
	} else if (components == 3) {
		image_ptr[0] = &(images[2].at(0));  // B
		image_ptr[1] = &(images[1].at(0));  // G
		image_ptr[2] = &(images[0].at(0));  // R
	} else if (components == 1) {
		image_ptr[0] = &(images[0].at(0));  // A
	}
	
	image.images = reinterpret_cast<unsigned char **>(image_ptr);
	image.width = width;
	image.height = height;
	
	header.num_channels = components;
	header.channels = static_cast<EXRChannelInfo *>(malloc(sizeof(EXRChannelInfo) * static_cast<size_t>(header.num_channels)));

	// Must be (A)BGR order, since most of EXR viewers expect this channel order.
	if (components == 4) {
		#ifdef _WIN32
		strncpy_s(header.channels[0].name, "A", 255);
		strncpy_s(header.channels[1].name, "B", 255);
		strncpy_s(header.channels[2].name, "G", 255);
		strncpy_s(header.channels[3].name, "R", 255);
		#else
		strncpy(header.channels[0].name, "A", 255);
		strncpy(header.channels[1].name, "B", 255);
		strncpy(header.channels[2].name, "G", 255);
		strncpy(header.channels[3].name, "R", 255);
		#endif
		header.channels[0].name[strlen("A")] = '\0';
		header.channels[1].name[strlen("B")] = '\0';
		header.channels[2].name[strlen("G")] = '\0';
		header.channels[3].name[strlen("R")] = '\0';
	} else if (components == 3) {
		#ifdef _WIN32
		strncpy_s(header.channels[0].name, "B", 255);
		strncpy_s(header.channels[1].name, "G", 255);
		strncpy_s(header.channels[2].name, "R", 255);
		#else
		strncpy(header.channels[0].name, "B", 255);
		strncpy(header.channels[1].name, "G", 255);
		strncpy(header.channels[2].name, "R", 255);
		#endif
		header.channels[0].name[strlen("B")] = '\0';
		header.channels[1].name[strlen("G")] = '\0';
		header.channels[2].name[strlen("R")] = '\0';
	} else {
		#ifdef _WIN32
		strncpy_s(header.channels[0].name, "A", 255);
		#else
		strncpy(header.channels[0].name, "A", 255);
		#endif
		header.channels[0].name[strlen("A")] = '\0';
	}
	
	header.pixel_types = static_cast<int *>( malloc(sizeof(int) * static_cast<size_t>(header.num_channels)));
	header.requested_pixel_types = static_cast<int *>( malloc(sizeof(int) * static_cast<size_t>(header.num_channels)));
	for (int i = 0; i < header.num_channels; i++) {
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;  // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;  // pixel type of output image to be stored in .EXR
	}
	
	int ret = SaveEXRImageToFile(&image, &header, path.c_str(), NULL);
	if (ret != TINYEXR_SUCCESS) {
		return ret;
	}
	
	free(header.channels);
	free(header.pixel_types);
	free(header.requested_pixel_types);
	
	return ret;
}


