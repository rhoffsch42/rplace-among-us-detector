#pragma once
#include <vector>
#include <map>
#include <string>

#define COLOR_ID_FOLDER "D:/DATA/rplace2022_TwitchFR_vs_ES-US/dl/detector-color-id/"
#define FILE_EXT ".color_id"

struct Color {
	Color(unsigned char red, unsigned char green, unsigned char blue) : r(red), g(green), b(blue) {}
	bool operator<(const Color& rhs) const {
		return std::tie(this->r, this->g, this->b) < std::tie(rhs.r, rhs.g, rhs.b);
	}
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
	//unsigned char a = 0;
};
//see README.md for more info
#define MAGIC_DIVIDER	16395
#define MAGIC_ID_ALLOC	1024

#define COLOR_UINT_CAST(X) ((*reinterpret_cast<unsigned int*>(&(X))) & 0x00ffffff)
#define QUICK_AND_DIRTY_ACCESS
#define PREBUILT_COLORS
class ImageLoader
{
public:
	static void createImage(const std::string& filename, std::vector<uint8_t>& image, unsigned width, unsigned height);

	ImageLoader();
	~ImageLoader();
	void loadImage(const std::string& filename);
	
	unsigned int width = 0;
	unsigned int height = 0;
	std::vector< std::vector<uint8_t> >		data;
	std::vector<Color>						colors;
	#ifdef QUICK_AND_DIRTY_ACCESS
	std::vector<unsigned int>				colorsId;
	#else
	std::map<Color, uint8_t>				colorsId;
	#endif
private:
	void _convertImage(std::vector<uint8_t>& dst, const std::string& filename, unsigned int& width, unsigned int& height);
	void _writeIdImage(const std::vector<uint8_t>& dst, const std::string& filename, unsigned int& width, unsigned int& height);
	void _loadIdImage(std::vector<uint8_t>& src, const std::string& filename, unsigned int& width, unsigned int& height);
	void _buildColors(const std::vector<unsigned char>& image_data);
};