#pragma once
#include "Common.hpp"

/**
 \brief Represents an image composed of pixels with values in [0,1]. Provide image loading/saving utilities, for both LDR and HDR images.
 \ingroup Resources
 */
class Image {
	
public:
	
	/** Default constructor. */
	Image();
	
	/**
	 Constructor that allocates an empty image with the given dimensions.
	 \param awidth the width of the image
	 \param aheight the height of the image
	 \param acomponents the number of components of the image
	 \param value the default value to use
	 */
	Image(int awidth, int aheight, int acomponents, float value = 0.0f);
	
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
	
	int width; //< The width of the image
	int height; //< The height of the image
	unsigned int components; //< Number of components/channels
	std::vector<float> pixels; //< The pixels values of the image
	
	/** Query if a path points to an image loaded in floating point, based on the extension.
	 \param path the path to the image
	 \return true if the file is loaded as a floating point numbers image
	 \note Extensions checked: .exr
	 */
	static bool isFloat(const std::string & path);
	
	/** Load an image from disk.
	 \param path the path to the image
	 \param channels the number of channels to load from the image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	  \param image will contain the image raw data as [0,1] floats
	 \return a success/error flag
	 */
	static int loadImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image);
	
	/** Save a LDR image to disk using stb_image.
	 \param path the path to the image
	 \param image the image data in [0,1] and infos
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveLDRImage(const std::string & path, const Image & image, bool flip, bool ignoreAlpha = false);
	
	/** Save a HDR image to disk using tiny_exr.
	 \param path the path to the image
	 \param image the image data in [0,1] and infos
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveHDRImage(const std::string & path, const Image & image, bool flip, bool ignoreAlpha = false);
	
	/** Bilinearly sample a cubemap in a given direction.
	 \param images the six cubemap faces, in standard Rendu order (px, nx, py, ny, pz, nz)
	 \param dir the direction to sample
	 \return the sampled color.
	 */
	static glm::vec3 sampleCubemap(const std::vector<Image> & images, const glm::vec3 & dir);
	
private:
	
	/** Load a LDR image from disk using stb_image.
	 \param path the path to the image
	 \param channels the number of channels to load from the image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \param image will contain the image raw data as [0,1] floats
	 \return a success/error flag
	 */
	static int loadLDRImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image);
	
	/** Load a HDR image from disk using tiny_exr, assuming 3-channels.
	 \param path the path to the image
	 \param channels will contain the number of channels of the loaded image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \param image will contain the image raw data as [0,1] floats
	 \return a success/error flag
	 */
	static int loadHDRImage(const std::string & path, unsigned int channels, bool flip, bool externalFile, Image & image);
	
};

