#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include "image_loader.hpp"
#include "lodepng.h"

ImageLoader::ImageLoader() {
#ifdef PREBUILT_COLORS
	this->colors.reserve(32);
	this->colors.push_back(Color(0, 0, 0));
	this->colors.push_back(Color(0, 117, 111));
	this->colors.push_back(Color(0, 158, 170));
	this->colors.push_back(Color(0, 163, 104));
	this->colors.push_back(Color(0, 204, 120));
	this->colors.push_back(Color(0, 204, 192));
	this->colors.push_back(Color(36, 80, 164));
	this->colors.push_back(Color(54, 144, 234));
	this->colors.push_back(Color(73, 58, 193));
	this->colors.push_back(Color(81, 82, 82));
	this->colors.push_back(Color(81, 233, 244));
	this->colors.push_back(Color(106, 92, 255));
	this->colors.push_back(Color(109, 0, 26));
	this->colors.push_back(Color(109, 72, 47));
	this->colors.push_back(Color(126, 237, 86));
	this->colors.push_back(Color(129, 30, 159));
	this->colors.push_back(Color(137, 141, 144));
	this->colors.push_back(Color(148, 179, 255));
	this->colors.push_back(Color(156, 105, 38));
	this->colors.push_back(Color(180, 74, 192));
	this->colors.push_back(Color(190, 0, 57));
	this->colors.push_back(Color(212, 215, 217));
	this->colors.push_back(Color(222, 16, 127));
	this->colors.push_back(Color(228, 171, 255));
	this->colors.push_back(Color(255, 56, 129));
	this->colors.push_back(Color(255, 69, 0));
	this->colors.push_back(Color(255, 153, 170));
	this->colors.push_back(Color(255, 168, 0));
	this->colors.push_back(Color(255, 180, 112));
	this->colors.push_back(Color(255, 214, 53));
	this->colors.push_back(Color(255, 248, 184));
	this->colors.push_back(Color(255, 255, 255));

	#ifdef QUICK_AND_DIRTY_ACCESS // O(1) access of colors by id
	this->colorsId.resize(MAGIC_ID_ALLOC);
	for (int i = 0; i < this->colors.size(); i++) {
		this->colorsId[COLOR_UINT_CAST(this->colors[i])/MAGIC_DIVIDER] = i;
	}
	#else // very slow map
	for (int i = 0; i < this->colors.size(); i++) {
		this->colorsId[this->colors[i]] = i;
	}
	#endif
#endif // PREBUILT_COLORS
}

ImageLoader::~ImageLoader() {

}

void ImageLoader::createImage(const std::string& filename, std::vector<uint8_t>& image, unsigned width, unsigned height) {
	//Encode the image
	unsigned error = lodepng_encode24_file(filename.c_str(), image.data(), width, height);

	//if there's an error, display it
	if (error) {
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		std::exit(2);
	}
}

void ImageLoader::loadImage(const std::string& filename) {
	std::vector<uint8_t> color_ids;
	unsigned int size;
	std::filesystem::path idfile = std::filesystem::path(filename).filename();
	idfile = COLOR_ID_FOLDER + idfile.string() + FILE_EXT;

	//check if our version of the file exists
	if (!std::filesystem::exists(idfile)) {
		std::cout << "Converted file does not exist, creating it : " << idfile << "\n";
		this->_convertImage(color_ids, filename, this->width, this->height);
		this->_writeIdImage(color_ids, idfile.string(), this->width, this->height);
	}
	else {
		std::cout << "Converted file exists, loading it : " << idfile << "\n";
		this->_loadIdImage(color_ids, idfile.string(), this->width, this->height);
	}
	size = color_ids.size();
	std::cout << "size : " << color_ids.size() << "\n";
	std::cout << "width : " << this->width << "\n";
	std::cout << "height : " << this->height << "\n";

	//reserve space
	this->data.resize(this->height);
	for (unsigned int j = 0; j < this->height; j++) {
		this->data[j].resize(this->width);
	}

	//fill 2d vector with id of colors
	for (unsigned int x = 0; x < size; x++) {
		this->data[x / this->width][x % this->width] = color_ids[x];
	}

	std::cout << "Data fully loaded.\n";
}
void ImageLoader::_loadIdImage(std::vector<uint8_t>& dst, const std::string& filename, unsigned int& width, unsigned int& height) {
	//open binary file
	std::ifstream file(filename, std::ios::binary);
	//read width, height, data
	file.read((char*)&width, sizeof(unsigned int));
	file.read((char*)&height, sizeof(unsigned int));
	dst.resize(width * height);
	file.read((char*)dst.data(), width * height);
	std::cout << filename << " loaded\n";
}

void ImageLoader::_convertImage(std::vector<uint8_t>& dst, const std::string& filename, unsigned int& width, unsigned int& height) {
	std::vector<unsigned char> image_data; //the raw pixels

	//decode
	unsigned error = lodepng::decode(image_data, width, height, filename);

	//if there's an error, display it
	if (error) {
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		std::exit(1);
	}

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
	std::cout << "width: " << width << " height: " << height << std::endl;

#ifndef PREBUILT_COLORS
	this->_buildColors(image_data);
#endif

	dst.resize(width * height);
	//convert the pixel rgb as color id
	for (unsigned int x = 0; x < image_data.size(); x += 4) {
		Color c(image_data[x + 0],
			image_data[x + 1],
			image_data[x + 2]);
		#ifdef QUICK_AND_DIRTY_ACCESS
		dst[x / 4] = this->colorsId[COLOR_UINT_CAST(c)/MAGIC_DIVIDER];
		#else
		dst[x / 4] = this->colorsId[c];
		#endif
	}

	std::cout << "conversion done\n";
}

void ImageLoader::_writeIdImage(const std::vector<uint8_t>& src, const std::string& filename, unsigned int& width, unsigned int& height) {
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	file.write((char*)&width, sizeof(unsigned int));
	file.write((char*)&height, sizeof(unsigned int));
	file.write((char*)src.data(), src.size());
	file.close();
	std::cout << filename << " written\n";
}

void ImageLoader::_buildColors(const std::vector<unsigned char>& image_data) {
	//store every pixel in a set
	std::set<Color> colors;
	unsigned int size = image_data.size();
	for (unsigned int i = 0; i < size; i += 4) {
		colors.insert(
			Color(image_data[i + 0],
				image_data[i + 1],
				image_data[i + 2])
		);
		if (colors.size() == 32) {
			break;
		}
	}
	//print size of set
	std::cout << "colors: " << colors.size() << "\n";
	if (colors.size() != 32) {
		std::cout << "wrong amount of colors, should be 32\n";
		std::exit(2);
	}
	//prebuilt code (not up to date)
	int i = 0;
	for (auto c : colors) {
		this->colors.push_back(Color(c.r, c.g, c.b));
		std::cout << "this->colors.push_back(Color(" << (int)c.r << ", " << (int)c.g << " ," << (int)c.b << ")); \n";
		//std::cout << "this->colorsId[Color(" << (int)c.r << ", " << (int)c.g << " ," << (int)c.b << ")] = " << i << "; \n";
		std::cout << "this->colorsId[this->colors[" << i << "]] = " << i << "; \n";
		i++;
	}
	std::exit(0);
}