#include "ImageUtilities.hpp"
#include "ResourcesManager.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include <miniz/miniz.h>
#define TINYEXR_USE_MINIZ (0)
#ifdef _WIN32
#pragma warning(disable:4996)
#endif
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

bool ImageUtilities::isFloat(const std::string & path){
	return path.substr(path.size()-4,4) == ".exr";
}

int ImageUtilities::loadImage(const std::string & path, unsigned int & width, unsigned int & height, void **data, const unsigned int channels, const bool flip, const bool externalFile){
	int ret = 0;
	if(isFloat(path)){
		ret = ImageUtilities::loadHDRImage(path, width, height, (float**)data, channels, flip, externalFile);
	} else {
		ret = ImageUtilities::loadLDRImage(path, width, height, (unsigned char**)data, channels, flip, externalFile);
	}
	return ret;
}

int ImageUtilities::loadLDRImage(const std::string &path, unsigned int & width, unsigned int & height, unsigned char **data, const unsigned int channels, const bool flip, const bool externalFile){
	const int finalChannels = channels > 0 ? channels : 4;
	
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
	
	int localWidth = 0;
	int localHeight = 0;
	// Beware: the size has to be cast to int, imposing a limit on big file sizes.
	*data = stbi_load_from_memory(rawData, (int)rawSize, &localWidth, &localHeight, NULL, finalChannels);
	free(rawData);
	
	if(*data == NULL){
		return 1;
	}
	
	width = (unsigned int)localWidth;
	height = (unsigned int)localHeight;
	
	return 0;
}

int ImageUtilities::loadHDRImage(const std::string &path, unsigned int & width, unsigned int & height, float **data, const unsigned int channels, const bool flip, const bool externalFile){
	const int finalChannels = channels > 0 ? channels : 3;
	
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
			FreeEXRHeader(&exr_header);
			return ret;
		}
	}
	free(rawData);
	
	// RGBA
	int idxsRGBA[] = {-1, -1, -1, -1};
	for (int c = 0; c < exr_header.num_channels; c++) {
		if (strcmp(exr_header.channels[c].name, "R") == 0) {
			idxsRGBA[0] = c;
		} else if (strcmp(exr_header.channels[c].name, "G") == 0) {
			idxsRGBA[1] = c;
		} else if (strcmp(exr_header.channels[c].name, "B") == 0) {
			idxsRGBA[2] = c;
		} else if (strcmp(exr_header.channels[c].name, "A") == 0) {
			idxsRGBA[3] = c;
		}
	}
	
	width = (unsigned int)exr_image.width;
	height = (unsigned int)exr_image.height;
	
	// Allocate final storage.
	*data = reinterpret_cast<float *>(malloc(finalChannels * sizeof(float) * static_cast<size_t>(width) *
												  static_cast<size_t>(height)));
	
	for(int y = 0; y < exr_image.height; ++y){
		for (int x = 0; x < exr_image.width; ++x){
			const int destIndex = y * (int)width + x;
			const int sourceIndex = flip ? (((int)height-1-y)*(int)width+x) : destIndex;
			
			for(int cid = 0; cid < finalChannels; ++cid){
				const int chanIdx = idxsRGBA[cid];
				if(chanIdx > -1){
					(*data)[finalChannels * destIndex + cid] = reinterpret_cast<float **>(exr_image.images)[chanIdx][sourceIndex];
				} else {
					(*data)[finalChannels * destIndex + cid] = cid == 3 ? 1.0f : 0.0f;
				}
			}
			
		}
	}
	
	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);
	
	return 0;
}

void write_stbi_to_disk(void *context, void *data, int size){
	const std::string * path = static_cast<std::string *>(context);
	Resources::saveRawDataToExternalFile(*path, static_cast<char *>(data), size);
}

int ImageUtilities::saveLDRImage(const std::string &path, const unsigned int width, const unsigned int height, const unsigned int channels, const unsigned char * data, const bool flip, const bool ignoreAlpha){
	
	stbi_flip_vertically_on_write(flip);
	
	int stride_in_bytes = (int)width*(int)channels;
	std::string pathCopy(path);
	
	int ret = 0;
	if(ignoreAlpha && channels == 4){
		unsigned char * newData = new unsigned char[width*height*4];
		for(unsigned int i = 0; i < width*height; ++i){
			newData[4*i+0] = data[4*i+0];
			newData[4*i+1] = data[4*i+1];
			newData[4*i+2] = data[4*i+2];
			newData[4*i+3] = 255;
		}
		
		// Write to an array in memory, then to the disk.
		stbi_write_png_to_func(write_stbi_to_disk, static_cast<void*>(&pathCopy), (int)width, (int)height, (int)channels, static_cast<const void*>(newData), stride_in_bytes);
		delete [] newData;
	} else {
		// Write to an array in memory, then to the disk.
		stbi_write_png_to_func(write_stbi_to_disk, static_cast<void*>(&pathCopy), (int)width, (int)height, (int)channels, static_cast<const void*>(data), stride_in_bytes);
	}
	return ret;
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
	int components = int(channels == 2 ? 3 : channels);
	image.num_channels = components;
	
	std::vector<float> images[4];
	
	if (components == 1) {
		images[0].resize(static_cast<size_t>(width * height));
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				const size_t destIndex = y * width + x;
				const size_t sourceIndex = flip ? ((height-1-y)*width+x) : destIndex;
				images[0][destIndex] = data[sourceIndex];
			}
		}
		
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
	image.width = (int)width;
	image.height = (int)height;
	
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
	
	int ret = 0;
	for (int i = 0; i < header.num_channels; i++) {
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;  // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;  // pixel type of output image to be stored in .EXR
	}
	// Here
	if (header.compression_type == TINYEXR_COMPRESSIONTYPE_ZFP) {
		// Not supported.
		ret = 1;
	}
	unsigned char *exrData = NULL;
	size_t exrSize = SaveEXRImageToMemory(&image, &header, &exrData, NULL);
	if(exrSize > 0 && exrData){
		Resources::saveRawDataToExternalFile(path, reinterpret_cast<char *>(exrData), exrSize);
		free(exrData);
	} else {
		ret = 1;
	}
	
	free(header.channels);
	free(header.pixel_types);
	free(header.requested_pixel_types);
	
	return ret;
}


