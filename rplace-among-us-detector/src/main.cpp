#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include "image_loader.hpp"
#include "amongus_detector.hpp"


struct ColorSmall {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
	//unsigned char a = 0;
};
struct ColorFull {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
	unsigned char a = 0;
};

int	main(unsigned int argc, char **argv) {
//#define TESTTT
#ifdef TESTTT
	
	ColorFull	c0 = { 0, 0, 0, 0 };
	ColorSmall	c1 = { 255, 255, 255 };
	ColorFull	c2 = { 255, 255, 255, 255 };
	ColorFull	c3 = { 255, 255, 255, 255 };
	
	//std::cout << ((*(unsigned int*)(&c0)) >> 0) << std::endl;
	std::cout << ((*(unsigned int*)(&c1)) << (8) >> 8) << std::endl;
	std::cout << ((*(unsigned int*)(&c1)) & 0x00ffffff) << std::endl;
	//std::cout << ((*(unsigned int*)(&c2)) >> 8) << std::endl;
	//std::cout << ((*(unsigned int*)(&c3)) >> 0) << std::endl;

	std::cout << (256*256*255 + 256*255 + 255) << std::endl;

	std::exit(0);
#endif

	std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

#define USE_ARG
#ifdef USE_ARG
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <path to image.png>\n";
		return (1);
	}
	std::filesystem::path	input(argv[1]);
	if (!std::filesystem::exists(argv[1])) {
		std::cerr << "File does not exist: " << input << "\n";
		return (1);
	}
#else
	std::filesystem::path	input("asset/rplace2022.png");
	//std::string	input("asset/rplace2022-small.png");
#endif
	std::filesystem::path output = std::filesystem::path("D:/DATA/rplace2022_TwitchFR_vs_ES-US/dl/detector-out/");
	output.concat("amongus-" + std::filesystem::path(input).filename().string());
	std::filesystem::path outputAmount = std::filesystem::path("D:/DATA/rplace2022_TwitchFR_vs_ES-US/dl/detector-out/amounts/").concat(std::filesystem::path(input).filename().string());
	outputAmount.replace_extension("amount.txt");
	std::cout << "input: " << input << "\n";
	std::cout << "output: " << output << "\n";
	if (std::filesystem::exists(output)) {
		std::cout << "Ouput image already exists: " << output << "\n";
		return (0);
	}
	
	AmongUsDetector detector(input.string());
	detector.search(0, detector.getImageHeight());
	unsigned int amount = detector.getAmount(outputAmount.string().c_str());
	std::cout << "Total amount: " << amount << ", logged in file: " << outputAmount << "\n";

	detector.applyAllResults(DRAW_FULL | CONTAIN_MODE | APPLY_ALL);
	detector.generateImage(output.string());

	return (0);
}