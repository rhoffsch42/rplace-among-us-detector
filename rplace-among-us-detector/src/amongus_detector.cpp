#include "amongus_detector.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

static void	linkStructs(MatchProcess* previous, MatchProcess* next) {
	previous->next = next;
	next->previous = previous;
}

void	MatchProcess::work(AmongUsDetector& detector) {
	if (this->matchShape) {
		detector.matchIter(this->previous->shapes[FULLY_R], 0, this->matchFuncRight, &this->shapes[FULLY_R], &this->shapes[MOSTLY_R], this->deflect ? &this->deflect->shapes[FULLY_R] : nullptr);
		detector.matchIter(this->previous->shapes[FULLY_L], 0, this->matchFuncLeft, &this->shapes[FULLY_L], &this->shapes[MOSTLY_L], this->deflect ? &this->deflect->shapes[FULLY_L] : nullptr);
		detector.matchIter(this->previous->shapes[MOSTLY_R], ERRORS_THRESHOLD, this->matchFuncRight, &this->shapes[FULLY_R], &this->shapes[MOSTLY_R], this->deflect ? &this->deflect->shapes[MOSTLY_R] : nullptr);
		detector.matchIter(this->previous->shapes[MOSTLY_L], ERRORS_THRESHOLD, this->matchFuncLeft, &this->shapes[FULLY_L], &this->shapes[MOSTLY_L], this->deflect ? &this->deflect->shapes[MOSTLY_L] : nullptr);
	} else {
		detector.unmatchIter(this->previous->shapes[FULLY_R], this->maxErrors, this->toleranceRatio, this->matchFuncRight, &this->shapes[FULLY_R], &this->shapes[MOSTLY_R], this->deflect ? &this->deflect->shapes[FULLY_R] : nullptr);
		detector.unmatchIter(this->previous->shapes[FULLY_L], this->maxErrors, this->toleranceRatio, this->matchFuncLeft, &this->shapes[FULLY_L], &this->shapes[MOSTLY_L], this->deflect ? &this->deflect->shapes[FULLY_L] : nullptr);
		detector.unmatchIter(this->previous->shapes[MOSTLY_R], this->maxErrors, this->toleranceRatio, this->matchFuncRight, &this->shapes[FULLY_R], &this->shapes[MOSTLY_R], this->deflect ? &this->deflect->shapes[MOSTLY_R] : nullptr);
		detector.unmatchIter(this->previous->shapes[MOSTLY_L], this->maxErrors, this->toleranceRatio, this->matchFuncLeft, &this->shapes[FULLY_L], &this->shapes[MOSTLY_L], this->deflect ? &this->deflect->shapes[MOSTLY_L] : nullptr);
	}
	//std::cout << "process results: " << this->shapes[FULLY_R].size() << " "
	//						<< this->shapes[FULLY_L].size() << " "
	//						<< this->shapes[MOSTLY_R].size() << " "
	//						<< this->shapes[MOSTLY_L].size() << std::endl;
}

AmongUsDetector::AmongUsDetector(std::string filename) : _filename(filename) {
	this->_image.loadImage(filename);
	this->_lockMap.resize(this->_image.height);
	for (auto& line : this->_lockMap)
		line.resize(this->_image.width);
	//every bools are false by default

	//init process
	this->_process[HEAD] = { nullptr, nullptr };//base
	this->_process[BODY] = { &AmongUsDetector::matchLegs, &AmongUsDetector::matchLegs };
	this->_process[AMONGUS] = { &AmongUsDetector::matchBagOnRight, &AmongUsDetector::matchBagOnLeft };
	this->_process[CLR_AMONGUS] = { &AmongUsDetector::matchSurroundingLeft, &AmongUsDetector::matchSurroundingRight, false, SURROUNDING_AMONGUS_MAX };
	this->_process[MINIHEAD] = { nullptr, nullptr };//deflect
	this->_process[MINIBODY] = { &AmongUsDetector::matchMiniLegs, &AmongUsDetector::matchMiniLegs };
	this->_process[MINIMONGUS] = { &AmongUsDetector::matchBagOnRight, &AmongUsDetector::matchBagOnLeft };
	this->_process[CLR_MINIMONGUS] = { &AmongUsDetector::matchMiniSurroundingLeft, &AmongUsDetector::matchMiniSurroundingRight, false, SURROUNDING_MINIMONGUS_MAX };
	this->_process[AMONGUS_NOBAG] = { nullptr, nullptr };//deflect
	this->_process[CLR_AMONGUS_NOBAG] = { &AmongUsDetector::matchNormalNobagSurroundingLeft, &AmongUsDetector::matchNormalNobagSurroundingRight, false, SURROUNDING_AMONGUS_NOBAG_MAX };
	this->_process[MINIMONGUS_NOBAG] = { nullptr, nullptr };//deflect
	this->_process[CLR_MINIMONGUS_NOBAG] = { &AmongUsDetector::matchMiniNobagSurroundingLeft, &AmongUsDetector::matchMiniNobagSurroundingRight, false, SURROUNDING_MINIMONGUS_NOBAG_MAX };
	
	//drawFlags (HEAD will be drawn as BODY)
	this->_process[HEAD].drawFlags = APPLY_NORMAL | APPLY_NOBAG;
	this->_process[BODY].drawFlags = APPLY_NORMAL | APPLY_NOBAG;
	this->_process[AMONGUS].drawFlags = APPLY_NORMAL | APPLY_BAG;
	this->_process[CLR_AMONGUS].drawFlags = APPLY_NORMAL | APPLY_BAG;
	this->_process[MINIHEAD].drawFlags = APPLY_MINI | APPLY_NOBAG;
	this->_process[MINIBODY].drawFlags = APPLY_MINI | APPLY_NOBAG;
	this->_process[MINIMONGUS].drawFlags = APPLY_MINI | APPLY_BAG;
	this->_process[CLR_MINIMONGUS].drawFlags = APPLY_MINI | APPLY_BAG;
	this->_process[AMONGUS_NOBAG].drawFlags = APPLY_NORMAL | APPLY_NOBAG;
	this->_process[CLR_AMONGUS_NOBAG].drawFlags = APPLY_NORMAL | APPLY_NOBAG;
	this->_process[MINIMONGUS_NOBAG].drawFlags = APPLY_MINI | APPLY_NOBAG;
	this->_process[CLR_MINIMONGUS_NOBAG].drawFlags = APPLY_MINI | APPLY_NOBAG;

	//deflects
	this->_process[BODY].deflect = &this->_process[MINIHEAD];
	this->_process[AMONGUS].deflect = &this->_process[AMONGUS_NOBAG];
	this->_process[MINIMONGUS].deflect = &this->_process[MINIMONGUS_NOBAG];
	this->_process[CLR_AMONGUS].deflect = &this->_process[AMONGUS_NOBAG];

	//chaining amongus
	linkStructs(&this->_process[HEAD], &this->_process[BODY]);
	linkStructs(&this->_process[BODY], &this->_process[AMONGUS]);
	linkStructs(&this->_process[AMONGUS], &this->_process[CLR_AMONGUS]);
	//chaining minimongus
	linkStructs(&this->_process[MINIHEAD], &this->_process[MINIBODY]);
	linkStructs(&this->_process[MINIBODY], &this->_process[MINIMONGUS]);
	linkStructs(&this->_process[MINIMONGUS], &this->_process[CLR_MINIMONGUS]);
	//chaining amongus_nobag
	linkStructs(&this->_process[AMONGUS_NOBAG], &this->_process[CLR_AMONGUS_NOBAG]);
	//chaining minimongus_nobag
	linkStructs(&this->_process[MINIMONGUS_NOBAG], &this->_process[CLR_MINIMONGUS_NOBAG]);
}
AmongUsDetector::~AmongUsDetector() {}

void	AmongUsDetector::search(unsigned int start, unsigned int end){//end is exclusive
	this->searchHeads(start, end);
	
	std::vector<MatchProcess*>	startProcess;
	//2nd process of each branch, the 1st is (or will be) already done
	startProcess.push_back(&this->_process[BODY]);
	startProcess.push_back(&this->_process[MINIBODY]);
	startProcess.push_back(&this->_process[CLR_AMONGUS_NOBAG]);
	startProcess.push_back(&this->_process[CLR_MINIMONGUS_NOBAG]);
	for (MatchProcess* current : startProcess) {
		//std::cout << "\nnew process chain\n";
		while (current) {
			current->work(*this);
			current = current->next;
		}
	}
}

void	AmongUsDetector::searchHeads(unsigned int start, unsigned int end) {
	start = std::max(start, (unsigned int)1);
	end = std::min(end, this->_image.height - 2);
	int	errors;
	uint8_t	refColor;

	//end is exclusive
	for (unsigned int y = start; y < end; y++) {
		//std::cout << y << " ";
		for (unsigned int x = 1; x < this->_image.width - 2; x++) {
			this->matchHeadRight(x, y, &errors, &refColor);
			if (!errors)
				this->_process[HEAD].shapes[FULLY_R].push_back(Coo(x, y, refColor));
			else if (errors == ERRORS_THRESHOLD)
				this->_process[HEAD].shapes[MOSTLY_R].push_back(Coo(x, y, refColor));
			this->matchHeadLeft(x, y, &errors, &refColor);
			if (!errors)
				this->_process[HEAD].shapes[FULLY_L].push_back(Coo(x, y, refColor));
			else if (errors == ERRORS_THRESHOLD)
				this->_process[HEAD].shapes[MOSTLY_L].push_back(Coo(x, y, refColor));
		}
	}
}

void	AmongUsDetector::matchIter(const std::vector<Coo>& potentialAmongus, uint8_t error_malus, matchFunc match,
	std::vector<Coo>* fully, std::vector<Coo>* mostly, std::vector<Coo>* deflect)
{
	int errors;
	for (const Coo& amon : potentialAmongus) {
		errors = match(*this, amon);
		if (errors != -1) {//is not out of bound of the image
			errors += error_malus;
			if (errors == 0) {
				fully->push_back(amon);
			}
			else if (errors <= ERRORS_THRESHOLD) {
				mostly->push_back(amon);
			}
			else if (deflect) {
				deflect->push_back(amon);
			}
		}
	}
}
void	AmongUsDetector::unmatchIter(const std::vector<Coo>& potentialAmongus, unsigned int maxErrors, float toleranceRatio, matchFunc match,
	std::vector<Coo>* fully, std::vector<Coo>* mostly, std::vector<Coo>* deflect)
{
	int	errors;
	for (const auto& amongus : potentialAmongus) {
		errors = match(*this, amongus);
		if (errors != -1) {//between legs is different
			if (errors == maxErrors)
				fully->push_back(amongus);
			else if (float(errors) / float(maxErrors) >= toleranceRatio)
				mostly->push_back(amongus);
			else if (deflect)
				deflect->push_back(amongus);
		}
	}
}

void	AmongUsDetector::matchHeadRight(unsigned int x, unsigned int y, int* errors, uint8_t* refColor) const {
	const std::vector< std::vector<uint8_t> >& data = this->_image.data;
	uint8_t colors[2] = { data[y][x - 1], data[y - 1][x - 1] };
	int err[2] = { 0, 0 };

	int i = 0;
	while (i < 2) {
		//eyes must be differents
		if (colors[i] == data[y][x])			err[i]++;
		if (colors[i] == data[y][x + 1])		err[i]++;
		//head mid must be equal
		if (colors[i] != data[y][x - 1])		err[i]++;
		//head top must be equal
		if (colors[i] != data[y - 1][x - 1])	err[i]++;
		if (colors[i] != data[y - 1][x])		err[i]++;
		if (colors[i] != data[y - 1][x + 1])	err[i]++;
		//head bot must be equal
		if (colors[i] != data[y + 1][x - 1])	err[i]++;
		if (colors[i] != data[y + 1][x])		err[i]++;
		if (colors[i] != data[y + 1][x + 1])	err[i]++;

		if (err[i] <= 1) {// matching, no need to check the 2nd ref color
			*errors = err[i];
			*refColor = colors[i];
			return;
		}
		i++;
	}
	if (err[0] < err[1]) {
		*errors = err[0];
		*refColor = colors[0];
	}
	else {
		*errors = err[1];
		*refColor = colors[1];
	}
}
void	AmongUsDetector::matchHeadLeft(unsigned int x, unsigned int y, int* errors, uint8_t* refColor) const {
	const std::vector< std::vector<uint8_t> >& data = this->_image.data;
	uint8_t colors[2] = { data[y][x + 1], data[y - 1][x + 1] };
	int err[2] = { 0, 0 };

	int i = 0;
	while (i < 2) {
		//eyes must be differents
		if (colors[i] == data[y][x])			err[i]++;
		if (colors[i] == data[y][x - 1])		err[i]++;
		//head mid must be equal
		if (colors[i] != data[y][x + 1])		err[i]++;
		//head top must be equal
		if (colors[i] != data[y - 1][x - 1])	err[i]++;
		if (colors[i] != data[y - 1][x])		err[i]++;
		if (colors[i] != data[y - 1][x + 1])	err[i]++;
		//head bot must be equal
		if (colors[i] != data[y + 1][x - 1])	err[i]++;
		if (colors[i] != data[y + 1][x])		err[i]++;
		if (colors[i] != data[y + 1][x + 1])	err[i]++;

		if (errors[i] <= 1) {// matching, no need to check the 2nd ref color
			*errors = err[i];
			*refColor = colors[i];
			return;
		}
		i++;
	}
	if (err[0] < err[1]) {
		*errors = err[0];
		*refColor = colors[0];
	} else {
		*errors = err[1];
		*refColor = colors[1];
	}
}
int		AmongUsDetector::matchLegs(const Coo& c) const {
	if (c.y + 3 >= this->_image.height)
		return -1;
	int errors = 0;

	if (c.refColor != this->_image.data[c.y + 2][c.x - 1])		errors++;
	if (c.refColor != this->_image.data[c.y + 2][c.x])			errors++;
	if (c.refColor != this->_image.data[c.y + 2][c.x + 1])		errors++;
	//feets
	if (c.refColor != this->_image.data[c.y + 3][c.x - 1])		errors++;
	if (c.refColor != this->_image.data[c.y + 3][c.x + 1])		errors++;

	return errors;
}
int		AmongUsDetector::matchBagOnLeft(const Coo& c) const {
	if (int(c.x) - 2 < 0)
		return -1;
	
	int errors = 0;
	if (c.refColor != this->_image.data[c.y][c.x - 2])			errors++;
	if (c.refColor != this->_image.data[c.y + 1][c.x - 2])		errors++;

	return errors;
}
int		AmongUsDetector::matchBagOnRight(const Coo& c) const {
	if (!(c.x + 2 < this->_image.width))
		return -1;

	int errors = 0;
	if (c.refColor != this->_image.data[c.y][c.x + 2])			errors++;
	if (c.refColor != this->_image.data[c.y + 1][c.x + 2])		errors++;

	return errors;
}
int		AmongUsDetector::matchMiniLegs(const Coo& c) const {
	if (!(c.y + 2 < this->_image.height))
		return -1;

	int errors = 0;
	//feets
	if (c.refColor != this->_image.data[c.y + 2][c.x - 1])		errors++;
	if (c.refColor != this->_image.data[c.y + 2][c.x + 1])		errors++;

	return errors;
}
int		AmongUsDetector::matchSurroundingRight(const Coo& c) const {
	return this->matchSurrounding(c, BAG_ON_LEFT);
}
int		AmongUsDetector::matchSurroundingLeft(const Coo& c) const {
	return this->matchSurrounding(c, BAG_ON_RIGHT);
}
int		AmongUsDetector::matchSurrounding(const Coo& c, bool bagOnLeft) const {
	const std::vector< std::vector<uint8_t> >& data = this->_image.data;
	const unsigned int maxWidth = this->_image.width;
	const unsigned int maxHeight = this->_image.height;
	const int x = c.x;//conversion to int to allow negative index checks
	const int y = c.y;//same
	const int x_headSide = bagOnLeft ? 2 : -2;
	const int x_bagSide = bagOnLeft ? -3 : 3;
	int errors = 0;

	//top of head
	if (y - 2 >= 0) {
		if (c.refColor != data[y - 2][x - 1])								{ errors++; }
		if (c.refColor != data[y - 2][x])									{ errors++; }
		if (c.refColor != data[y - 2][x + 1])								{ errors++; }
	}
	//each side of the forehead
	if (x - 2 >= 0 && c.refColor != data[y - 1][x - 2])						{ errors++; }
	if (x + 2 < maxWidth && c.refColor != data[y - 1][x + 2])				{ errors++; }
	//side of face
	if (x + x_headSide >= 0 && x + x_headSide < maxWidth) {
		if (data[y][x + (x_headSide)] == data[y][x + (x_headSide/2)])		{ return -1;}//exterior eye
		if (c.refColor != data[y][x + x_headSide])							{ errors++; }
		if (c.refColor != data[y + 1][x + x_headSide])						{ errors++; }
	}
	//side of bag
	if (x + x_bagSide >= 0 && x + x_bagSide < maxWidth) {
		if (c.refColor != data[y][x + x_bagSide])							{ errors++; }
		if (c.refColor != data[y + 1][x + x_bagSide])						{ errors++; }
	}
	if (y + 2 < maxHeight) {
		//side of left leg
		if (x - 2 >= 0) {
			if (c.refColor != data[y + 2][x - 2])							{ errors++; }
			if (y + 3 < maxHeight && c.refColor != data[y + 3][x - 2])		{ errors++; }
		}
		//side of right leg
		if (x + 2 < maxWidth) {
			if (c.refColor != data[y + 2][x + 2])							{ errors++; }
			if (y + 3 < maxHeight && c.refColor != data[y + 3][x + 2])		{ errors++; }
		}
		if (y + 3 < maxHeight) {
			//between legs
			if (c.refColor != data[y + 3][x])								{ errors++; }
			//under legs
			if (y + 4 < maxHeight) {
				if (c.refColor != data[y + 4][x - 1])						{ errors++; }
				if (c.refColor != data[y + 4][x + 1])						{ errors++; }
			}
		}
	}
	return errors;
}
int		AmongUsDetector::matchMiniSurroundingRight(const Coo& c) const {
	return this->matchMiniSurrounding(c, BAG_ON_LEFT);
}
int		AmongUsDetector::matchMiniSurroundingLeft(const Coo& c) const {
	return this->matchMiniSurrounding(c, BAG_ON_RIGHT);
}
int		AmongUsDetector::matchMiniSurrounding(const Coo& c, bool bagOnLeft) const {
	const std::vector< std::vector<uint8_t> >& data = this->_image.data;
	const unsigned int maxWidth = this->_image.width;
	const unsigned int maxHeight = this->_image.height;
	const int x = c.x;//conversion to int to allow negative index checks
	const int y = c.y;//same
	const int x_headSide = bagOnLeft ? 2 : -2;
	const int x_bagSide = bagOnLeft ? -3 : 3;
	unsigned int errors = 0;

	//top of head
	if (y - 2 >= 0) {
		if (c.refColor != data[y - 2][x - 1])					{ errors++; }
		if (c.refColor != data[y - 2][x])						{ errors++; }
		if (c.refColor != data[y - 2][x + 1])					{ errors++; }
	}
	//each side of the forehead
	if (x - 2 >= 0 && c.refColor != data[y - 1][x - 2])			{ errors++; }
	if (x + 2 < maxWidth && c.refColor != data[y - 1][x + 2])	{ errors++; }
	//side of face
	if (x + x_headSide >= 0 && x + x_headSide < maxWidth) {
		if (data[y][x + x_headSide] == data[y][x + (x_headSide/2)]) { return -1; }//exterior eye
		if (c.refColor != data[y][x + x_headSide])				{ errors++; }
		if (c.refColor != data[y + 1][x + x_headSide])			{ errors++; }
	}
	//side of bag
	if (x + x_bagSide >= 0 && x + x_bagSide < maxWidth) {
		if (c.refColor != data[y][x + x_bagSide])				{ errors++; }
		if (c.refColor != data[y + 1][x + x_bagSide])			{ errors++; }
	}
	//side of left leg
	if (x - 2 >= 0) {
		if (c.refColor != data[y + 2][x - 2])					{ errors++; }
	}
	//side of right leg
	if (x + 2 < maxWidth) {
		if (c.refColor != data[y + 2][x + 2])					{ errors++; }
	}
	//between legs
	if (c.refColor != data[y + 2][x])							{ errors++; }
	//under legs
	if (y + 3 < maxHeight) {
		if (c.refColor != data[y + 3][x - 1])					{ errors++; }
		if (c.refColor != data[y + 3][x + 1])					{ errors++; }
	}

	return errors;
}
int		AmongUsDetector::matchNobagSurrounding(const Coo& c, bool isLookingRight, bool isMini) const {
	const std::vector< std::vector<uint8_t> >& data = this->_image.data;
	const unsigned int maxWidth = this->_image.width;
	const unsigned int maxHeight = this->_image.height;
	const int x = c.x;//conversion to int to allow negative index checks
	int y = c.y;//same
	int errors = 0;

	// 'A' in pixel art : special case for minimongus no bag, they must have both eyes != refColor, no error tolerance accepted.
	if (isMini && (c.refColor == data[y][x] || c.refColor == data[y][x+(isLookingRight?1:-1)])) { return -1; }

	//top of head
	if (y - 2 >= 0) {
		if (c.refColor != data[y - 2][x - 1])	{ errors++; }
		if (c.refColor != data[y - 2][x])		{ errors++; }
		if (c.refColor != data[y - 2][x + 1])	{ errors++; }
	}
	//left side
	if (x - 2 >= 0) {
		if (!isLookingRight && data[y][x - 1] == data[y][x - 2]) { return -1; }//exterior eye
		if (c.refColor != data[y - 1][x - 2])	{ errors++; }
		if (c.refColor != data[y][x - 2])		{ errors++; }
		if (c.refColor != data[y + 1][x - 2])	{ errors++; }
		if (c.refColor != data[y + 2][x - 2])	{ errors++; }
		if (!isMini && c.refColor != data[y + 3][x - 2])	{ errors++; }
	}
	//right side
	if (x + 2 < maxWidth) {
		if (isLookingRight && data[y][x + 1] == data[y][x + 2]) { return -1; }//exterior eye
		if (c.refColor != data[y - 1][x + 2])	{ errors++; }
		if (c.refColor != data[y][x + 2])		{ errors++; }
		if (c.refColor != data[y + 1][x + 2])	{ errors++; }
		if (c.refColor != data[y + 2][x + 2])	{ errors++; }
		if (!isMini && c.refColor != data[y + 3][x + 2])	{ errors++; }
	}
	if (isMini)
		y--;
	//between legs
	if (c.refColor != data[y + 3][x])			{ errors++; }
	else { return -1; }
	//under legs
	if (y + 4 < maxHeight) {
		if (c.refColor != data[y + 4][x - 1])	{ errors++; }
		if (c.refColor != data[y + 4][x + 1])	{ errors++; }
	}
	
	return errors;
}
int		AmongUsDetector::matchNormalNobagSurroundingLeft(const Coo& c) const {
	return this->matchNobagSurrounding(c, LOOKING_LEFT, IS_NORMAL);
}
int		AmongUsDetector::matchNormalNobagSurroundingRight(const Coo& c) const {
	return this->matchNobagSurrounding(c, LOOKING_RIGHT, IS_NORMAL);
}
int		AmongUsDetector::matchMiniNobagSurroundingLeft(const Coo& c) const {
	return this->matchNobagSurrounding(c, LOOKING_LEFT, IS_MINI);
}
int		AmongUsDetector::matchMiniNobagSurroundingRight(const Coo& c) const {
	return this->matchNobagSurrounding(c, LOOKING_RIGHT, IS_MINI);
}

void	AmongUsDetector::_applyCoordinates(const std::vector<Coo>& coos, uint16_t flags) {
	for (const auto& coo : coos) {
		if (FLAG_HAS(flags, DRAW_FULL))
			this->_applyFullAmongus(coo, flags);
		else
			this->_lockMap[coo.y][coo.x] = true;
	}
}
void	AmongUsDetector::_applyFullAmongus(const Coo& coo, uint16_t flags) {
	//std::cout << "applyFullAmongus: " << coo.x << ":" << coo.y << std::endl;
	int feetsYoffset = FLAG_HAS(flags, APPLY_NORMAL) ? 3 : 2;
	Coo start{ std::max(0, int(coo.x) - 1),
				std::max(0, int(coo.y) - 1) };
	Coo end{ std::min(int(this->_image.width), int(coo.x) + 1),
				std::min(int(this->_image.height), int(coo.y) + feetsYoffset)};
	//body without feets
	for (unsigned int y = start.y; y < end.y; y++) {
		for (unsigned int x = start.x; x <= end.x; x++) {
			this->_lockMap[y][x] = true;
		}
	}
	//feets
	this->_lockMap[coo.y + feetsYoffset][coo.x + 1] = true;
	this->_lockMap[coo.y + feetsYoffset][coo.x - 1] = true;

	//bag
	if (FLAG_HAS(flags, APPLY_BAG)) {
		if (FLAG_HAS(flags, APPLY_RIGHT) && coo.x - 2 >= 0) {
			this->_lockMap[coo.y][coo.x - 2] = true;
			this->_lockMap[coo.y + 1][coo.x - 2] = true;
		} else if (FLAG_HAS(flags, APPLY_LEFT) && coo.x + 2 < this->_image.width) {
			this->_lockMap[coo.y][coo.x + 2] = true;
			this->_lockMap[coo.y + 1][coo.x + 2] = true;
		}
	}
}
void	AmongUsDetector::applyProcessResults(const MatchProcess& process, uint16_t flags) {
	//cleaning wrong flags
	flags = flags & (APPLY_LEFT | APPLY_RIGHT | APPLY_FULLY | APPLY_MOSTLY | CONTAIN_MODE | DRAW_FULL);

	if (FLAG_CHECK(flags, APPLY_RIGHT | APPLY_FULLY))
		this->_applyCoordinates(process.shapes[FULLY_R], (flags | process.drawFlags) & ~APPLY_LEFT);
	if (FLAG_CHECK(flags, APPLY_LEFT | APPLY_FULLY))
		this->_applyCoordinates(process.shapes[FULLY_L], (flags | process.drawFlags) & ~APPLY_RIGHT);
	if (FLAG_CHECK(flags, APPLY_RIGHT | APPLY_MOSTLY))
		this->_applyCoordinates(process.shapes[MOSTLY_R], (flags | process.drawFlags) & ~APPLY_LEFT);
	if (FLAG_CHECK(flags, APPLY_LEFT | APPLY_MOSTLY))
		this->_applyCoordinates(process.shapes[MOSTLY_L], (flags | process.drawFlags) & ~APPLY_RIGHT);
}
void	AmongUsDetector::applyAllResults(const uint16_t flags) {
	this->applyProcessResults(this->_process[CLR_AMONGUS], flags);
	this->applyProcessResults(this->_process[CLR_MINIMONGUS], flags);
	this->applyProcessResults(this->_process[CLR_AMONGUS_NOBAG], flags);
	this->applyProcessResults(this->_process[CLR_MINIMONGUS_NOBAG], flags);
}

void	AmongUsDetector::reset() {
	this->resetMap();
	//clear all process
	for (int i = 0; i < I_EVOLUTIONS; i++) {
		this->_process[i].shapes[FULLY_R].clear();
		this->_process[i].shapes[FULLY_L].clear();
		this->_process[i].shapes[MOSTLY_R].clear();
		this->_process[i].shapes[MOSTLY_L].clear();
	}
}
void	AmongUsDetector::resetMap() {
	for (unsigned int j = 0; j < this->_image.height; j++)
		for (unsigned int i = 0; i < this->_image.width; i++)
			this->_lockMap[j][i] = false;
}

void	AmongUsDetector::generateImage(const std::string& filename) {
	std::vector<uint8_t>	dataRGB;
	dataRGB.resize(this->_image.width * this->_image.height * 3);

	//build data based on the map
	std::cout << "Building image data..." << std::endl;
	for (unsigned int y = 0; y < this->_image.height; y++) {
		const unsigned int sizeline = y * this->_image.width;
		for (unsigned int x = 0; x < this->_image.width; x++) {
			Color c = this->_image.colors[this->_image.data[y][x]];
			if (this->_lockMap[y][x]) {
				#ifdef HIGHLIGH_ORIGINAL
				dataRGB[3 * (sizeline + x) + 0] = c.r;
				dataRGB[3 * (sizeline + x) + 1] = c.g;
				dataRGB[3 * (sizeline + x) + 2] = c.b;
				#else
				dataRGB[3 * (sizeline + x) + 0] = HIGHLIGH_WITH_RED;
				dataRGB[3 * (sizeline + x) + 1] = HIGHLIGH_WITH_GREEN;
				dataRGB[3 * (sizeline + x) + 2] = HIGHLIGH_WITH_BLUE;
				#endif
			} else {
				dataRGB[3 * (sizeline + x) + 0] = c.r / 6;
				dataRGB[3 * (sizeline + x) + 1] = c.g / 6;
				dataRGB[3 * (sizeline + x) + 2] = c.b / 6;
			}
		}
	}
	std::cout << "Writing image to " << filename << std::endl;
	this->_image.createImage(filename, dataRGB, this->_image.width, this->_image.height);
	std::cout << "Done.\n";
}

unsigned int AmongUsDetector::getImageWidth() const { return this->_image.width; }
unsigned int AmongUsDetector::getImageHeight() const { return this->_image.height; }

unsigned int AmongUsDetector::getAmount(const char *logfile) const {
	unsigned int clr_amongus =
		this->_process[CLR_AMONGUS].shapes[FULLY_R].size() +
		this->_process[CLR_AMONGUS].shapes[FULLY_L].size() +
		this->_process[CLR_AMONGUS].shapes[MOSTLY_R].size() +
		this->_process[CLR_AMONGUS].shapes[MOSTLY_L].size();
	unsigned int clr_minimongus =
		this->_process[CLR_MINIMONGUS].shapes[FULLY_R].size() +
		this->_process[CLR_MINIMONGUS].shapes[FULLY_L].size() +
		this->_process[CLR_MINIMONGUS].shapes[MOSTLY_R].size() +
		this->_process[CLR_MINIMONGUS].shapes[MOSTLY_L].size();
	unsigned int clr_amongus_nobag =
		this->_process[CLR_AMONGUS_NOBAG].shapes[FULLY_R].size() +
		this->_process[CLR_AMONGUS_NOBAG].shapes[FULLY_L].size() +
		this->_process[CLR_AMONGUS_NOBAG].shapes[MOSTLY_R].size() +
		this->_process[CLR_AMONGUS_NOBAG].shapes[MOSTLY_L].size();
	unsigned int clr_minimongus_nobag =
		this->_process[CLR_MINIMONGUS_NOBAG].shapes[FULLY_R].size() +
		this->_process[CLR_MINIMONGUS_NOBAG].shapes[FULLY_L].size() +
		this->_process[CLR_MINIMONGUS_NOBAG].shapes[MOSTLY_R].size() +
		this->_process[CLR_MINIMONGUS_NOBAG].shapes[MOSTLY_L].size();
	unsigned int total = clr_amongus + clr_minimongus + clr_amongus_nobag + clr_minimongus_nobag;
	if (logfile) {
		std::stringstream ss;
		ss << "AmongUs " << clr_amongus << "\n"
			<< "Minimongus " << clr_minimongus << "\n"
			<< "AmongUsNoBag " << clr_amongus_nobag << "\n"
			<< "MinimongusNoBag " << clr_minimongus_nobag << "\n"
			<< "Total " << total << std::endl;
		std::ofstream	log(logfile, std::ios::trunc);
		log << ss.str();
		log.close();
	}
	return total;
}