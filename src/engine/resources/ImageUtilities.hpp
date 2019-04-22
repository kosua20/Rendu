#ifndef ImageUtilities_h
#define ImageUtilities_h
#include "Common.hpp"

/**
 \brief Represents an image composed of pixels with values in [0,1].
 \ingroup Resources
 */
struct Image {
	
	/** Default constructor. */
	Image();
	
	/**
	 Constructor that allocates an empty image with the given dimensions.
	 \param awidth the width of the image
	 \param aheight the height of the image
	 \param acomponents the number of components of the image
	 */
	Image(int awidth, int aheight, int acomponents);
	
	unsigned int width; //< The width of the image
	unsigned int height; //< The height of the image
	unsigned int components; //< Number of components/channels
	std::vector<float> pixels; //< The pixels values of the image
	
};


/**
 \brief Provide image loading/saving utilities, for both LDR and HDR images.
 \ingroup Resources
 */
class ImageUtilities {

public:
	
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
	static int loadImage(const std::string & path,  const unsigned int channels, const bool flip, const bool externalFile, Image & image);
	
	/** Save a LDR image to disk using stb_image.
	 \param path the path to the image
	 \param image the image data in [0,1] and infos
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveLDRImage(const std::string & path, const Image & image, const bool flip, const bool ignoreAlpha = false);
	
	/** Save a HDR image to disk using tiny_exr.
	 \param path the path to the image
	 \param image the image data in [0,1] and infos
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveHDRImage(const std::string & path, const Image & image, const bool flip, const bool ignoreAlpha = false);
	
private:
	
	/** Load a LDR image from disk using stb_image.
	 \param path the path to the image
	 \param channels the number of channels to load from the image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \param image will contain the image raw data as [0,1] floats
	 \return a success/error flag
	 */
	static int loadLDRImage(const std::string & path, const unsigned int channels, const bool flip, const bool externalFile, Image & image);
	
	/** Load a HDR image from disk using tiny_exr, assuming 3-channels.
	 \param path the path to the image
	 \param channels will contain the number of channels of the loaded image
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \param image will contain the image raw data as [0,1] floats
	 \return a success/error flag
	 */
	static int loadHDRImage(const std::string & path, const unsigned int channels, const bool flip, const bool externalFile, Image & image);
	
};


#endif
