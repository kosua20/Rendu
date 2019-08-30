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

Image::Image() {
	width = height = 0;
	components	 = 0;
	pixels.clear();
}

Image::Image(unsigned int awidth, unsigned int aheight, unsigned int acomponents, float value) :
	width(int(awidth)), height(int(aheight)), components(acomponents) {
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

glm::vec3 Image::rgbn(float x, float y) const {
	const float xi = x * float(width);
	const float yi = y * float(height);
	const float xb = std::round(xi);
	const float yb = std::round(yi);
	const int x0   = int(xb) % width;
	const int y0   = int(yb) % height;
	return rgb(x0, y0);
}

glm::vec3 Image::rgbl(float x, float y) const {
	const float xi = x * float(width);
	const float yi = y * float(height);
	const float xb = std::floor(xi);
	const float yb = std::floor(yi);
	const float dx = xi - xb;
	const float dy = yi - yb;

	const int x0 = int(xb) % width;
	const int x1 = (int(xb) + 1) % width;
	const int y0 = int(yb) % height;
	const int y1 = (int(yb) + 1) % height;

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

	const int x0 = int(xb) % width;
	const int x1 = (int(xb) + 1) % width;
	const int y0 = int(yb) % height;
	const int y1 = (int(yb) + 1) % height;

	// Fetch four pixels.
	const glm::vec4 & p00 = rgba(x0, y0);
	const glm::vec4 & p01 = rgba(x0, y1);
	const glm::vec4 & p10 = rgba(x1, y0);
	const glm::vec4 & p11 = rgba(x1, y1);

	return (1.0f - dx) * ((1.0f - dy) * p00 + dy * p01) + dx * ((1.0f - dy) * p10 + dy * p11);
}

bool Image::isFloat(const std::string & path) {
	return path.substr(path.size() - 4, 4) == ".exr";
}

int Image::loadImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image) {
	if(isFloat(path)) {
		return Image::loadHDRImage(path, channels, flip, externalFile, image);
	}
	return Image::loadLDRImage(path, channels, flip, externalFile, image);
}

int Image::loadLDRImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image) {
	const unsigned int finalChannels = channels > 0 ? channels : 4;

	image.pixels.clear();
	image.width = image.height = 0;
	image.components		   = 0;

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

	image.width		 = uint(localWidth);
	image.height	 = uint(localHeight);
	image.components = finalChannels;
	// Transform data from chars to float.
	const size_t totalSize = image.width * image.height * image.components;
	image.pixels.resize(totalSize);
	for(size_t pid = 0; pid < totalSize; ++pid) {
		image.pixels[pid] = float(data[pid]) / 255.0f;
	}
	free(data);
	return 0;
}

int Image::loadHDRImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image) {
	const unsigned int finalChannels = channels > 0 ? channels : 3;
	image.pixels.clear();
	image.width = image.height = 0;
	image.components		   = 0;

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

	{
		const int ret = ParseEXRVersionFromMemory(&exr_version, rawData, tinyexr::kEXRVersionSize);
		if(ret != TINYEXR_SUCCESS) {
			return ret;
		}

		if(exr_version.multipart || exr_version.non_image) {
			return TINYEXR_ERROR_INVALID_DATA;
		}
	}
	{
		const int ret = ParseEXRHeaderFromMemory(&exr_header, &exr_version, rawData, rawSize, nullptr);
		if(ret != TINYEXR_SUCCESS) {
			FreeEXRHeader(&exr_header);
			return ret;
		}
	}

	// Read HALF channel as FLOAT.
	for(int i = 0; i < exr_header.num_channels; i++) {
		if(exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
			exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}

	{
		const int ret = LoadEXRImageFromMemory(&exr_image, &exr_header, rawData, rawSize, nullptr);
		if(ret != TINYEXR_SUCCESS) {
			FreeEXRHeader(&exr_header);
			return ret;
		}
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

	image.width		 = uint(exr_image.width);
	image.height	 = uint(exr_image.height);
	image.components = finalChannels;
	image.pixels.resize(image.width * image.height * image.components);

	for(int y = 0; y < exr_image.height; ++y) {
		for(int x = 0; x < exr_image.width; ++x) {
			const int destIndex   = y * int(image.width) + x;
			const int sourceIndex = flip ? ((int(image.height) - 1 - y) * int(image.width) + x) : destIndex;

			for(unsigned int cid = 0; cid < finalChannels; ++cid) {
				const int chanIdx = idxsRGBA[cid];
				if(chanIdx > -1) {
					image.pixels[finalChannels * destIndex + cid] = reinterpret_cast<float **>(exr_image.images)[chanIdx][sourceIndex];
				} else {
					image.pixels[finalChannels * destIndex + cid] = cid == 3 ? 1.0f : 0.0f;
				}
			}
		}
	}

	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);

	return 0;
}

void write_stbi_to_disk(void * context, void * data, int size) {
	const std::string * path = static_cast<std::string *>(context);
	Resources::saveRawDataToExternalFile(*path, static_cast<char *>(data), size);
}

int Image::saveLDRImage(const std::string & path, const Image & image, bool flip, bool ignoreAlpha) {
	const unsigned int width	= image.width;
	const unsigned int height   = image.height;
	const unsigned int channels = image.components;

	stbi_flip_vertically_on_write(flip);

	const int strideInBytes = int(width) * int(channels);
	std::string pathCopy(path);

	unsigned char * newData = new unsigned char[width * height * channels];
	for(unsigned int pid = 0; pid < width * height; ++pid) {
		for(unsigned int cid = 0; cid < channels; ++cid) {
			const unsigned int currentPix = channels * pid + cid;
			const float newValue		  = std::min(255.0f, std::max(0.0f, 255.0f * image.pixels[currentPix]));
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

int Image::saveHDRImage(const std::string & path, const Image & image, bool flip, bool ignoreAlpha) {

	const unsigned int width	= image.width;
	const unsigned int height   = image.height;
	const unsigned int channels = image.components;

	// Assume at least 16x16 pixels.
	if(width < 16)
		return TINYEXR_ERROR_INVALID_ARGUMENT;
	if(height < 16)
		return TINYEXR_ERROR_INVALID_ARGUMENT;

	EXRHeader header;
	InitEXRHeader(&header);

	EXRImage exr_image;
	InitEXRImage(&exr_image);

	// Components: 1, 3, 3, 4
	const int components   = int(channels == 2 ? 3 : channels);
	exr_image.num_channels = components;

	std::vector<float> images[4];

	if(components == 1) {
		images[0].resize(static_cast<size_t>(width * height));
		for(size_t y = 0; y < height; y++) {
			for(size_t x = 0; x < width; x++) {
				const size_t destIndex   = y * width + x;
				const size_t sourceIndex = flip ? ((height - 1 - y) * width + x) : destIndex;
				images[0][destIndex]	 = image.pixels[sourceIndex];
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
				for(unsigned int j = 0; j < channels; ++j) {
					images[j][destIndex] = image.pixels[static_cast<size_t>(channels) * sourceIndex + j];
				}
				for(unsigned int j = channels; j < 3; ++j) {
					images[j][destIndex] = 0.0f;
				}
				if(components == 4) {
					images[3][destIndex] = ignoreAlpha ? 1.0f : image.pixels[static_cast<size_t>(channels) * sourceIndex + 3];
				}
			}
		}
	}

	float * image_ptr[4] = {nullptr, nullptr, nullptr, nullptr};
	if(components == 4) {
		image_ptr[0] = &(images[3].at(0)); // A
		image_ptr[1] = &(images[2].at(0)); // B
		image_ptr[2] = &(images[1].at(0)); // G
		image_ptr[3] = &(images[0].at(0)); // R
	} else if(components == 3) {
		image_ptr[0] = &(images[2].at(0)); // B
		image_ptr[1] = &(images[1].at(0)); // G
		image_ptr[2] = &(images[0].at(0)); // R
	} else if(components == 1) {
		image_ptr[0] = &(images[0].at(0)); // A
	}

	exr_image.images = reinterpret_cast<unsigned char **>(image_ptr);
	exr_image.width  = int(width);
	exr_image.height = int(height);

	header.num_channels = components;
	header.channels		= static_cast<EXRChannelInfo *>(malloc(sizeof(EXRChannelInfo) * static_cast<size_t>(header.num_channels)));

	// Must be (A)BGR order, since most of EXR viewers expect this channel order.
	if(components == 4) {
		header.channels[0].name[0] = 'A';
		header.channels[1].name[0] = 'B';
		header.channels[2].name[0] = 'G';
		header.channels[3].name[0] = 'R';
		header.channels[0].name[1] = '\0';
		header.channels[1].name[1] = '\0';
		header.channels[2].name[1] = '\0';
		header.channels[3].name[1] = '\0';
	} else if(components == 3) {
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

glm::vec3 Image::sampleCubemap(const std::vector<Image> & images, const glm::vec3 & dir) {
	// Images are stored in the following order:
	// px, nx, py, ny, pz, nz
	const glm::vec3 abs = glm::abs(dir);
	int side			= 0;
	float x = 0.0f, y = 0.0f;
	float denom = 1.0f;
	if(abs.x >= abs.y && abs.x >= abs.z) {
		denom = abs.x;
		y	 = dir.y;
		// X faces.
		if(dir.x >= 0.0f) {
			side = 0;
			x	= -dir.z;
		} else {
			side = 1;
			x	= dir.z;
		}

	} else if(abs.y >= abs.x && abs.y >= abs.z) {
		denom = abs.y;
		x	 = dir.x;
		// Y faces.
		if(dir.y >= 0.0f) {
			side = 2;
			y	= -dir.z;
		} else {
			side = 3;
			y	= dir.z;
		}
	} else if(abs.z >= abs.x && abs.z >= abs.y) {
		denom = abs.z;
		y	 = dir.y;
		// Z faces.
		if(dir.z >= 0.0f) {
			side = 4;
			x	= dir.x;
		} else {
			side = 5;
			x	= -dir.x;
		}
	}
	x = 0.5f * (x / denom) + 0.5f;
	y = 0.5f * (-y / denom) + 0.5f;
	// Ensure seamless borders between faces by never sampling closer than one pixel to the edge.
	const float eps = 1.0f / float(std::min(images[side].width, images[side].height));
	x				= glm::clamp(x, 0.0f + eps, 1.0f - eps);
	y				= glm::clamp(y, 0.0f + eps, 1.0f - eps);
	return images[side].rgbl(x, y);
}
