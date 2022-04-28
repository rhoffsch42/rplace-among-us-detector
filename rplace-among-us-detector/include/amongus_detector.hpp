#pragma once
#include <vector>
#include <string>
#include <functional>
#include "image_loader.hpp"




//// CONSTANTS MATCH FUNCS //////////////////
// [0, 1] (0 no surrounding pixels have different colors, 1 is full surrounding have different colors)
#define SURROUNDING_THRESHOLD 0.65
#define SURROUNDING_AMONGUS_MAX 16
#define SURROUNDING_AMONGUS_NOBAG_MAX 16
#define SURROUNDING_MINIMONGUS_MAX 14
#define SURROUNDING_MINIMONGUS_NOBAG_MAX 14
#define ERRORS_THRESHOLD 1
#define BAG_ON_LEFT true
#define BAG_ON_RIGHT false
#define LOOKING_LEFT false
#define LOOKING_RIGHT true
#define IS_MINI true
#define IS_NORMAL false

//// CONSTANTS IMAGE ////////////////////////
#define HIGHLIGH_ORIGINAL
#define HIGHLIGH_WITH_RED	255
#define HIGHLIGH_WITH_GREEN	0
#define HIGHLIGH_WITH_BLUE	0

//// INDEX VECTOR 2D ////////////////////////
#define USE_INDEX

#define FULLY_R			0
#define FULLY_L			1
#define MOSTLY_R		2
#define MOSTLY_L		3
#define I_VERSIONS		4

#define HEAD			0
#define BODY			1
#define AMONGUS			2
#define CLR_AMONGUS		3
#define MINIHEAD		4
#define MINIBODY		5
#define MINIMONGUS		6
#define CLR_MINIMONGUS	7
#define AMONGUS_NOBAG			8
#define CLR_AMONGUS_NOBAG		9
#define MINIMONGUS_NOBAG		10
#define CLR_MINIMONGUS_NOBAG	11
#define I_EVOLUTIONS			12

//// FLAGS IMAGE ////////////////////////
#define APPLY_RIGHT			0b0000000001
#define APPLY_LEFT			0b0000000010
#define APPLY_MOSTLY		0b0000000100
#define APPLY_FULLY			0b0000001000
#define APPLY_MINI			0b0000010000
#define APPLY_NORMAL		0b0000100000
#define APPLY_BAG			0b0001000000
#define APPLY_NOBAG			0b0010000000
#define APPLY_ALL			0b0011111111

#define CONTAIN_MODE		0b0100000000
#define DRAW_FULL			0b1000000000
#define EVERYTHING			0b1111111111111111

#define FLAG_IS(X, Y)		(((X) & (Y)) == (X))
#define FLAG_HAS(X, Y)		(((X) & (Y)) != 0)
#define FLAG_CHECK(X, Y)	( FLAG_IS(X,Y) || (FLAG_HAS(X, CONTAIN_MODE) && FLAG_HAS(X,Y)) )


struct Coo {
	unsigned int x;
	unsigned int y;
	uint8_t	refColor = 255;
	/*
					...          ...		+ : an eye (coordinate of the entire body)
					.000. 		.000.		* : the other eye
	minimongus		.*+02.	   .20+*.		0 : head
	dont have		.0002.	   .2000.		1 : legs
	this line --->	.111.	    .111.		2 : bag2(optionnal)
					.1.1. 		.1.1.		3 : bag3 (optionnal)
					 . .         . .		. : surrounding
	*/
};

class AmongUsDetector;
typedef std::function<int (const AmongUsDetector&, const Coo&) > matchFunc;

struct MatchProcess {
	matchFunc		matchFuncLeft;
	matchFunc		matchFuncRight;
	bool			matchShape = true;
	uint8_t			maxErrors = 0;//not used if matchShape is false
	float			toleranceRatio = SURROUNDING_THRESHOLD;//not used if matchShape is false

	MatchProcess*	deflect = nullptr;
	MatchProcess*	previous = nullptr;
	MatchProcess*	next = nullptr;

	uint16_t		drawFlags = 0;
	std::vector<Coo>	shapes[I_VERSIONS];
	void			work(AmongUsDetector& detector);
};

class AmongUsDetector
{
public:
	AmongUsDetector(std::string filename);
	~AmongUsDetector();
	
	void	search(unsigned int start, unsigned int end);//end is exclusive
	void	matchIter(
		const std::vector<Coo>& potentialAmongus,
		uint8_t error_malus,
		matchFunc match,
		std::vector<Coo>* fully,
		std::vector<Coo>* mostly,
		std::vector<Coo>* deflect = nullptr);
	void	unmatchIter(
		const std::vector<Coo>& potentialAmongus,
		unsigned int maxErrors,
		float toleranceRatio,
		matchFunc match,
		std::vector<Coo>* fully,
		std::vector<Coo>* mostly,
		std::vector<Coo>* deflect = nullptr);


	void	searchHeads(unsigned int start, unsigned int end);//end is exclusive
	void	matchHeadRight(unsigned int x, unsigned int y, int* errors, uint8_t* refColor) const;
	void	matchHeadLeft(unsigned int x, unsigned int y, int* errors, uint8_t* refColor) const;

	int		matchLegs(const Coo& c) const;
	int		matchBagOnLeft(const Coo& c) const;
	int		matchBagOnRight(const Coo& c) const;
	int		matchMiniLegs(const Coo& c) const;
	int		matchSurrounding(const Coo& c, bool bagOnLeft) const;
	int		matchSurroundingRight(const Coo& c) const;
	int		matchSurroundingLeft(const Coo& c) const;
	int		matchMiniSurrounding(const Coo& c, bool bagOnLeft) const;
	int		matchMiniSurroundingRight(const Coo& c) const;
	int		matchMiniSurroundingLeft(const Coo& c) const;
	int		matchNobagSurrounding(const Coo& c, bool isLookingRight, bool isMini) const;
	int		matchNormalNobagSurroundingRight(const Coo& c) const;
	int		matchNormalNobagSurroundingLeft(const Coo& c) const;
	int		matchMiniNobagSurroundingRight(const Coo& c) const;
	int		matchMiniNobagSurroundingLeft(const Coo& c) const;

	void	applyProcessResults(const MatchProcess& process, uint16_t flags);
	void	applyAllResults(const uint16_t flags);
	void	reset();
	void	resetMap();
	void	generateImage(const std::string& filename);

	unsigned int	getAmount(const char* logfile = nullptr) const;
	unsigned int	getImageWidth() const;
	unsigned int	getImageHeight() const;
private:
	std::vector< std::vector<bool> >	_lockMap;
	MatchProcess		_process[I_EVOLUTIONS];
	ImageLoader			_image;
	std::string			_filename;

	void				_applyCoordinates(const std::vector<Coo>& coos, uint16_t flags);
	void				_applyFullAmongus(const Coo& coo, uint16_t flags);
};