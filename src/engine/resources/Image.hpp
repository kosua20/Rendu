#pragma once
#include "Common.hpp"

/**
 \brief Represents an image composed of pixels with values in [0,1]. Provide image loading/saving utilities, for both LDR and HDR images.
 \ingroup Resources
 */
class Image {

public:

	/** \brief Options for saving an image to disk. */
	enum class Save : uint {
		NONE = 0, ///< No specific options.
		FLIP = 1 << 0, ///< Flip the image vertically.
		IGNORE_ALPHA = 1 << 1, ///< Force alpha to 1.
		SRGB_LDR = 1 << 2 ///< Apply gamma sRGB correction before saving, ignored for HDR images.
	};
	
	/** Default constructor. */
	Image() = default;

	/**
	 Constructor that allocates an empty image with the given dimensions.
	 \param awidth the width of the image
	 \param aheight the height of the image
	 \param acomponents the number of components of the image
	 \param value the default value to use
	 */
	Image(unsigned int awidth, unsigned int aheight, unsigned int acomponents, float value = 0.0f);

	/** Accessor to a RGBA pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel
	 \warning no access or component check is done
	 */
	glm::vec4 & rgba(int x, int y);

	/** Accessor to the RGB part of a pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel
	 \warning no access or component check is done
	 */
	glm::vec3 & rgb(int x, int y);

	/** Accessor to the red/first component of a pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel first component
	 \warning no access or component check is done
	 */
	float & r(int x, int y);

	/** Const accessor to a RGBA pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel
	 \warning no access or component check is done
	 */
	const glm::vec4 & rgba(int x, int y) const;

	/** Const accessor to the RGB part of a pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel
	 \warning no access or component check is done
	 */
	const glm::vec3 & rgb(int x, int y) const;

	/** Const accessor to the red/first component of a pixel
	 \param x horizontal coordinate
	 \param y vertical coordinate
	 \return reference to the given pixel first component
	 \warning no access or component check is done
	 */
	const float & r(int x, int y) const;

	/** Bilinear UV image read.
	 \param x horizontal unit float coordinate
	 \param y vertical unit float coordinate
	 \return the bilinearly interpolated color value
	 \note Wrapping is applied on both axis.
	 */
	glm::vec3 rgbl(float x, float y) const;

	/** Nearest-neighbour UV image read.
	 \param x horizontal unit float coordinate
	 \param y vertical unit float coordinate
	 \return the color of the nearest pixel
	 \note Wrapping is applied on both axis.
	 */
	glm::vec3 rgbn(float x, float y) const;

	/** Bilinear UV image read.
	 \param x horizontal unit float coordinate
	 \param y vertical unit float coordinate
	 \return the bilinearly interpolated color value
	 \note Wrapping is applied on both axis.
	 */
	glm::vec4 rgbal(float x, float y) const;

	/** Load an image from disk. Will contain the image raw data as [0,1] floats.
	 \param path the path to the image
	 \param channels the number of channels to load from the image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	int load(const std::string & path, unsigned int channels, bool flip, bool externalFile);
	
	/** Save an image to disk, either in HDR (when using "exr" extension) or in LDR (any other extension).
	 \param path the path to the image
	 \param options options to use when writing the image on disk
	 \return a success/error flag
	 */
	int save(const std::string & path, Save options) const;
	
	/** Query if a path points to an image loaded in floating point, based on the extension.
	 \param path the path to the image
	 \return true if the file is loaded as a floating point numbers image
	 \note Extensions checked: .exr
	 */
	static bool isFloat(const std::string & path);
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Image & operator=(const Image &) = delete;
	
	/** Copy constructor (disabled). */
	Image(const Image &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	Image & operator=(Image &&) = default;
	
	/** Move constructor. */
	Image(Image &&) = default;
	
	unsigned int width = 0;		 ///< The width of the image
	unsigned int height = 0;	 ///< The height of the image
	unsigned int components = 0; ///< Number of components/channels
	std::vector<float> pixels;	 ///< The pixels values of the image
	
private:
	
	/** Save a LDR image to disk using stb_image.
	 \param path the path to the image
	 \param options options to use when writing the image on disk
	 \return a success/error flag
	 */
	int saveAsLDR(const std::string & path, Save options) const;
	
	/** Save a HDR image to disk using tiny_exr.
	 \param path the path to the image
	 \param options options to use when writing the image on disk
	 \return a success/error flag
	 */
	int saveAsHDR(const std::string & path, Save options) const;
	
	/** Load a LDR image from disk using stb_image.
	 \param path the path to the image
	 \param channels the number of channels to load from the image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	int loadLDR(const std::string & path, unsigned int channels, bool flip, bool externalFile);

	/** Load a HDR image from disk using tiny_exr, assuming 3-channels.
	 \param path the path to the image
	 \param channels will contain the number of channels of the loaded image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	int loadHDR(const std::string & path, unsigned int channels, bool flip, bool externalFile);
	
};

/** Compute the integral modulo, ensuring that the result is positive.
 \param x value to decompose
 \param w the divisor
 \return the positive remainder
 */
inline int modPos(int x, int w){
	return ((x%w)+w)%w;
}

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return the combination of both flags.
 */
inline Image::Save operator|( Image::Save t0,  Image::Save t1) {
	return static_cast< Image::Save>(static_cast<uint>(t0) | static_cast<uint>(t1));
}

/** Extracting operator for TextureShape.
 \param t0 reference flag
 \param t1 flag to extract
 \return true if t0 'contains' t1
 */
inline bool operator&( Image::Save t0,  Image::Save t1) {
	return bool(static_cast<uint>(t0) & static_cast<uint>(t1));
}

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return reference to the first flag after combination with the second flag.
 */
inline  Image::Save & operator|=( Image::Save & t0,  Image::Save & t1) {
	return t0 = t0 | t1;
}
