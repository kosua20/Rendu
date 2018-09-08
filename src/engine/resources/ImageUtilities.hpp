#ifndef ImageUtilities_h
#define ImageUtilities_h
#include "../Common.hpp"

/**
 \brief Provide image loading/saving utilities, for both LDR and HDR images.
 \ingroup Resources
 */
class ImageUtilities {

public:
	
	/** Query if a path points to a HDR image, based on the extension.
	 \param path the path to the image
	 \return true if the file is a HDR image
	 \note Extensions checked: .exr
	 */
	static bool isHDR(const std::string & path);
	
	/** Load an image from disk.
	 \param path the path to the image
	 \param width will contain the width of the loaded image
	 \param height will contain the height of the loaded image
	 \param channels will contain the number of channels of the loaded image
	 \param data will contain the image raw data (unsigned char for LDR, float for HDR)
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	static int loadImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, void **data, const bool flip, const bool externalFile = false);
	
	/** Save a LDR image to disk using stb_image.
	 \param path the path to the image
	 \param width the width of the image
	 \param height the height of the image
	 \param channels the number of channels of the image
	 \param data the image raw data
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveLDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const unsigned char *data, const bool flip, const bool ignoreAlpha = false);
	
	/** Save a HDR image to disk using tiny_exr.
	 \param path the path to the image
	 \param width the width of the image
	 \param height the height of the image
	 \param channels the number of channels of the image
	 \param data the image raw data
	 \param flip should the image be vertically flipped
	 \param ignoreAlpha if true, the alpha channel will be ignored
	 \return a success/error flag
	 */
	static int saveHDRImage(const std::string & path, const unsigned int width, const unsigned int height, const unsigned int channels, const float *data, const bool flip, const bool ignoreAlpha = false);
	
private:
	
	/** Load a LDR image from disk using stb_image.
	 \param path the path to the image
	 \param width will contain the width of the loaded image
	 \param height will contain the height of the loaded image
	 \param channels will contain the number of channels of the loaded image
	 \param data will contain the image raw data
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	static int loadLDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, unsigned char **data, const bool flip, const bool externalFile);
	
	/** Load a HDR image from disk using tiny_exr.
	 \param path the path to the image
	 \param width will contain the width of the loaded image
	 \param height will contain the height of the loaded image
	 \param channels will contain the number of channels of the loaded image
	 \param data will contain the image raw data
	 \param flip should the image be vertically flipped
	 \param externalFile if true, skip the resources manager and load directly from disk
	 \return a success/error flag
	 */
	static int loadHDRImage(const std::string & path, unsigned int & width, unsigned int & height, unsigned int & channels, float **data, const bool flip, const bool externalFile);
	
};


#endif
