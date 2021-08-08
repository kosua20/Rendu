#include "resources/Image.hpp"
#include "resources/ResourcesManager.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#ifdef _WIN32
#	define STBI_MSC_SECURE_CRT
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include <miniz/miniz.h>
#define TINYEXR_USE_MINIZ (0)
#ifdef _WIN32
#	pragma warning(disable : 4996)
#endif
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

void write_stbi_to_disk(void * context, void * data, int size) {
	const std::string * path = static_cast<std::string *>(context);
	Resources::saveRawDataToExternalFile(*path, static_cast<char *>(data), size);
}

Image::Image(unsigned int awidth, unsigned int aheight, unsigned int acomponents, float value) :
	width(awidth), height(aheight), components(acomponents) {
	pixels.resize(width * height * components, value);
}

glm::vec4 & Image::rgba(int x, int y) {
	return reinterpret_cast<glm::vec4 *>(&pixels[(y * width + x) * components])[0];
}

glm::vec3 & Image::rgb(int x, int y) {
	return reinterpret_cast<glm::vec3 *>(&pixels[(y * width + x) * components])[0];
}

float & Image::r(int x, int y) {
	return pixels[(y * width + x) * components];
}

const glm::vec4 & Image::rgba(int x, int y) const {
	return reinterpret_cast<const glm::vec4 *>(&pixels[(y * width + x) * components])[0];
}

const glm::vec3 & Image::rgb(int x, int y) const {
	return reinterpret_cast<const glm::vec3 *>(&pixels[(y * width + x) * components])[0];
}

const float & Image::r(int x, int y) const {
	return pixels[(y * width + x) * components];
}

glm::vec3 Image::rgbn(float x, float y) const {
	const float xi = x * float(width);
	const float yi = y * float(height);
	const float xb = std::round(xi);
	const float yb = std::round(yi);
	const int x0   = modPos(int(xb), int(width) );
	const int y0   = modPos(int(yb), int(height));
	return rgb(x0, y0);
}

glm::vec3 Image::rgbl(float x, float y) const {
	const float xi = x * float(width);
	const float yi = y * float(height);
	const float xb = std::floor(xi);
	const float yb = std::floor(yi);
	const float dx = xi - xb;
	const float dy = yi - yb;

	const int x0 = modPos(int(xb), int(width));
	const int y0 = modPos(int(yb), int(height));
	const int x1 = modPos((int(xb) + 1), int(width));
	const int y1 = modPos((int(yb) + 1), int(height));

	// Fetch four pixels.
	const glm::vec3 & p00 = rgb(x0, y0);
	const glm::vec3 & p01 = rgb(x0, y1);
	const glm::vec3 & p10 = rgb(x1, y0);
	const glm::vec3 & p11 = rgb(x1, y1);

	return (1.0f - dx) * ((1.0f - dy) * p00 + dy * p01) + dx * ((1.0f - dy) * p10 + dy * p11);
}

glm::vec4 Image::rgbal(float x, float y) const {
	const float xi = x * float(width);
	const float yi = y * float(height);
	const float xb = std::floor(xi);
	const float yb = std::floor(yi);
	const float dx = xi - xb;
	const float dy = yi - yb;

	const int x0 = modPos(int(xb), int(width));
	const int y0 = modPos(int(yb), int(height));
	const int x1 = modPos((int(xb) + 1), int(width));
	const int y1 = modPos((int(yb) + 1), int(height));

	// Fetch four pixels.
	const glm::vec4 & p00 = rgba(x0, y0);
	const glm::vec4 & p01 = rgba(x0, y1);
	const glm::vec4 & p10 = rgba(x1, y0);
	const glm::vec4 & p11 = rgba(x1, y1);

	return (1.0f - dx) * ((1.0f - dy) * p00 + dy * p01) + dx * ((1.0f - dy) * p10 + dy * p11);
}

int Image::load(const std::string & path, unsigned int channels, bool flip, bool externalFile) {
	if(isFloat(path)) {
		return loadHDR(path, channels, flip, externalFile);
	}
	return loadLDR(path, channels, flip, externalFile);
}

int Image::save(const std::string & path, Image::Save options) const {
	if(isFloat(path)) {
		return saveAsHDR(path, options);
	}
	return saveAsLDR(path, options);
}

bool Image::isFloat(const std::string & path) {
	return path.substr(path.size() - 4, 4) == ".exr";
}

int Image::saveAsLDR(const std::string & path, Image::Save options) const {

	const bool ignoreAlpha = options & Save::IGNORE_ALPHA;
	const bool flip = options & Save::FLIP;
	const bool gammaCorrect = options & Save::SRGB_LDR;
	const unsigned int channels = components;
	
	stbi_flip_vertically_on_write(flip);
	
	const int strideInBytes = int(width) * int(channels);
	std::string pathCopy(path);
	
	unsigned char * newData = new unsigned char[width * height * channels];
	for(unsigned int pid = 0; pid < width * height; ++pid) {
		for(unsigned int cid = 0; cid < channels; ++cid) {
			const unsigned int currentPix = channels * pid + cid;
			float value = pixels[currentPix];
			// Apply gamma correction if requested, except on alpha channel.
			if(gammaCorrect && (cid != 3)){
				value = std::pow(value, 1.0f / 2.2f);
			}
			const float newValue		  = std::min(255.0f, std::max(0.0f, 255.0f * value));
			newData[currentPix]			  = static_cast<unsigned char>(newValue);
			if(cid == 3 && ignoreAlpha) {
				newData[currentPix] = 255;
			}
		}
	}
	// Write to an array in memory, then to the disk.
	const int ret = stbi_write_png_to_func(write_stbi_to_disk, static_cast<void *>(&pathCopy), int(width), int(height), int(channels), static_cast<const void *>(newData), strideInBytes);
	delete[] newData;
	return ret == 0;
}

int Image::saveAsHDR(const std::string & path, Image::Save options) const {
	
	
	// Assume at least 16x16 pixels.
	if(width < 16)
		return TINYEXR_ERROR_INVALID_ARGUMENT;
	if(height < 16)
		return TINYEXR_ERROR_INVALID_ARGUMENT;
	
	EXRHeader header;
	InitEXRHeader(&header);
	
	EXRImage exr_image;
	InitEXRImage(&exr_image);

	const bool ignoreAlpha = options & Save::IGNORE_ALPHA;
	const bool flip = options & Save::FLIP;

	// Components: 1, 3, 3, 4
	const int channels   = int(components == 2 ? 3 : components);
	exr_image.num_channels = channels;
	
	std::vector<float> images[4];
	
	if(channels == 1) {
		images[0].resize(static_cast<size_t>(width * height));
		for(size_t y = 0; y < height; y++) {
			for(size_t x = 0; x < width; x++) {
				const size_t destIndex   = y * width + x;
				const size_t sourceIndex = flip ? ((height - 1 - y) * width + x) : destIndex;
				images[0][destIndex]	 = pixels[sourceIndex];
			}
		}
		
	} else {
		images[0].resize(static_cast<size_t>(width * height));
		images[1].resize(static_cast<size_t>(width * height));
		images[2].resize(static_cast<size_t>(width * height));
		images[3].resize(static_cast<size_t>(width * height));
		
		// Split RGB(A)RGB(A)RGB(A)... into R, G and B(and A) layers
		// By default we try to always fill at least three channels.
		for(size_t y = 0; y < height; y++) {
			for(size_t x = 0; x < width; x++) {
				const size_t destIndex   = y * width + x;
				const size_t sourceIndex = flip ? ((height - 1 - y) * width + x) : destIndex;
				for(unsigned int j = 0; j < components; ++j) {
					images[j][destIndex] = pixels[static_cast<size_t>(components) * sourceIndex + j];
				}
				for(unsigned int j = components; j < 3; ++j) {
					images[j][destIndex] = 0.0f;
				}
				if(components == 4) {
					images[3][destIndex] = ignoreAlpha ? 1.0f : pixels[static_cast<size_t>(components) * sourceIndex + 3];
				}
			}
		}
	}
	
	float * image_ptr[4] = {nullptr, nullptr, nullptr, nullptr};
	if(channels == 4) {
		image_ptr[0] = &(images[3].at(0)); // A
		image_ptr[1] = &(images[2].at(0)); // B
		image_ptr[2] = &(images[1].at(0)); // G
		image_ptr[3] = &(images[0].at(0)); // R
	} else if(channels == 3) {
		image_ptr[0] = &(images[2].at(0)); // B
		image_ptr[1] = &(images[1].at(0)); // G
		image_ptr[2] = &(images[0].at(0)); // R
	} else if(channels == 1) {
		image_ptr[0] = &(images[0].at(0)); // A
	}
	
	exr_image.images = reinterpret_cast<unsigned char **>(image_ptr);
	exr_image.width  = int(width);
	exr_image.height = int(height);
	
	header.num_channels = channels;
	header.channels		= static_cast<EXRChannelInfo *>(malloc(sizeof(EXRChannelInfo) * static_cast<size_t>(header.num_channels)));
	
	// Must be (A)BGR order, since most of EXR viewers expect this channel order.
	if(channels == 4) {
		header.channels[0].name[0] = 'A';
		header.channels[1].name[0] = 'B';
		header.channels[2].name[0] = 'G';
		header.channels[3].name[0] = 'R';
		header.channels[0].name[1] = '\0';
		header.channels[1].name[1] = '\0';
		header.channels[2].name[1] = '\0';
		header.channels[3].name[1] = '\0';
	} else if(channels == 3) {
		header.channels[0].name[0] = 'B';
		header.channels[1].name[0] = 'G';
		header.channels[2].name[0] = 'R';
		header.channels[0].name[1] = '\0';
		header.channels[1].name[1] = '\0';
		header.channels[2].name[1] = '\0';
	} else {
		header.channels[0].name[0] = 'A';
		header.channels[0].name[1] = '\0';
	}
	
	header.pixel_types			 = static_cast<int *>(malloc(sizeof(int) * static_cast<size_t>(header.num_channels)));
	header.requested_pixel_types = static_cast<int *>(malloc(sizeof(int) * static_cast<size_t>(header.num_channels)));
	
	int ret = 0;
	for(int i = 0; i < header.num_channels; i++) {
		header.pixel_types[i]			= TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;  // pixel type of output image to be stored in .EXR
	}
	// Here
	if(header.compression_type == TINYEXR_COMPRESSIONTYPE_ZFP) {
		// Not supported.
		ret = 1;
	}
	unsigned char * exrData = nullptr;
	const size_t exrSize	= SaveEXRImageToMemory(&exr_image, &header, &exrData, nullptr);
	if(exrSize > 0 && exrData) {
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

int Image::loadLDR(const std::string & path, unsigned int channels, bool flip, bool externalFile) {
	const unsigned int finalChannels = channels > 0 ? channels : 4;

	pixels.clear();
	width = height = 0;
	components	   = 0;

	size_t rawSize = 0;
	unsigned char * rawData;
	if(externalFile) {
		rawData = reinterpret_cast<unsigned char *>(Resources::loadRawDataFromExternalFile(path, rawSize));
	} else {
		rawData = reinterpret_cast<unsigned char *>(Resources::manager().getRawData(path, rawSize));
	}

	if(rawData == nullptr || rawSize == 0) {
		return 1;
	}

	stbi_set_flip_vertically_on_load(flip);

	int localWidth  = 0;
	int localHeight = 0;
	// Beware: the size has to be cast to int, imposing a limit on big file sizes.
	unsigned char * data = stbi_load_from_memory(rawData, int(rawSize), &localWidth, &localHeight, nullptr, int(finalChannels));
	free(rawData);

	if(data == nullptr) {
		return 1;
	}

	width	   = uint(localWidth);
	height	   = uint(localHeight);
	components = finalChannels;
	// Transform data from chars to float.
	const size_t totalSize = width * height * components;
	pixels.resize(totalSize);
	for(size_t pid = 0; pid < totalSize; ++pid) {
		pixels[pid] = float(data[pid]) / 255.0f;
	}
	free(data);
	return 0;
}

int Image::loadHDR(const std::string & path, unsigned int channels, bool flip, bool externalFile) {
	const unsigned int finalChannels = channels > 0 ? channels : 3;
	pixels.clear();
	width = height = 0;
	components	   = 0;

	// Code adapted from tinyEXR deprecated loadEXR.
	EXRVersion exr_version;
	EXRImage exr_image;
	EXRHeader exr_header;
	InitEXRHeader(&exr_header);
	InitEXRImage(&exr_image);

	size_t rawSize = 0;
	unsigned char * rawData;
	if(externalFile) {
		rawData = reinterpret_cast<unsigned char *>(Resources::loadRawDataFromExternalFile(path, rawSize));
	} else {
		rawData = reinterpret_cast<unsigned char *>(Resources::manager().getRawData(path, rawSize));
	}

	if(rawData == nullptr || rawSize == 0) {
		return 1;
	}

	int ret = ParseEXRVersionFromMemory(&exr_version, rawData, tinyexr::kEXRVersionSize);
	if(ret != TINYEXR_SUCCESS) {
		return ret;
	}
	if(exr_version.multipart || exr_version.non_image) {
		return TINYEXR_ERROR_INVALID_DATA;
	}
	ret = ParseEXRHeaderFromMemory(&exr_header, &exr_version, rawData, rawSize, nullptr);
	if(ret != TINYEXR_SUCCESS) {
		FreeEXRHeader(&exr_header);
		return ret;
	}
	// Read HALF channel as FLOAT.
	for(int i = 0; i < exr_header.num_channels; i++) {
		if(exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
			exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}
	ret = LoadEXRImageFromMemory(&exr_image, &exr_header, rawData, rawSize, nullptr);
	if(ret != TINYEXR_SUCCESS) {
		FreeEXRHeader(&exr_header);
		return ret;
	}
	free(rawData);

	// RGBA
	int idxsRGBA[] = {-1, -1, -1, -1};
	for(int c = 0; c < exr_header.num_channels; c++) {
		if(strcmp(exr_header.channels[c].name, "R") == 0) {
			idxsRGBA[0] = c;
		} else if(strcmp(exr_header.channels[c].name, "G") == 0) {
			idxsRGBA[1] = c;
		} else if(strcmp(exr_header.channels[c].name, "B") == 0) {
			idxsRGBA[2] = c;
		} else if(strcmp(exr_header.channels[c].name, "A") == 0) {
			idxsRGBA[3] = c;
		}
	}

	width	   = uint(exr_image.width);
	height	   = uint(exr_image.height);
	components = finalChannels;
	pixels.resize(width * height * components);

	for(int y = 0; y < exr_image.height; ++y) {
		for(int x = 0; x < exr_image.width; ++x) {
			const int destIndex   = y * int(width) + x;
			const int sourceIndex = flip ? ((int(height) - 1 - y) * int(width) + x) : destIndex;

			for(unsigned int cid = 0; cid < finalChannels; ++cid) {
				const int chanIdx = idxsRGBA[cid];
				if(chanIdx > -1) {
					pixels[finalChannels * destIndex + cid] = reinterpret_cast<float **>(exr_image.images)[chanIdx][sourceIndex];
				} else {
					pixels[finalChannels * destIndex + cid] = cid == 3 ? 1.0f : 0.0f;
				}
			}
		}
	}

	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);
	return 0;
}
