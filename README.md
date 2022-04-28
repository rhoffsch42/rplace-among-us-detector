### Final timelapse: https://www.youtube.com/watch?v=ljDQlrhlVp4
#
# Procedure
The aim was to highlight the work of the AmongUs community, hence these specific criterias when searching amongis which allowed to recognize imperfect individuals :
- The eyes could have different colors, as long as they were different from the rest of the individual.
- Tolerance for the complete individual: 1 different pixel.
- Tolerance for the background: at least 65% of adjacent pixels had to be different, corner pixels are not checked.
- The side pixel of the exterior eye had to be different from the reference color, this allowed to filter out many false positives with texts.
- The pixel between legs during the background check had to be different from The reference color, this allowed to filter out many false positives with thin boxes.
- Special case for minimongus without backpack : they had to have both eyes of the same color. There were too many cases where it matched a 'A' in pixel art when the exterior eye had the reference color.

These criterias can be tweeked to have more or less tolerant results. The background tolerance allowed to find a lot more individuals, but leaded to false positive during contested areas where there was a pixel soup. As these wars were temporary and easily recognizable, this didn't impact the overall appreciation of amongis presences. 

[tmp](display difference with tolerance% gifs)

# MatchProcess structure
The structure represents a node in the procedure, a node contains results for its searched pattern. Nodes are linked together to represents the full procedure of finding an amongi on the canvas. Some nodes can be deflected to another chain with a shared pattern if the criterias would not be met.

Full chains, the starting node is HEAD :
```
HEAD   ┌────┼───────────────────┼─► MINIHEAD     │
 ▼     │    │                   │      ▼         │
BODY ──┘    │                   │   MINIBODY     │
 ▼          │                   │      ▼         │
AMONGUS ────┼─► AMONGUS_NOBAG   │  MINIMONGUS ───┼─► MINIMONGUS_NOBAG
 ▼          │        ▼          │      ▼         │          ▼
CLR_AMONGUS │ CLR_AMONGUS_NOBAG │ CLR_MINIMONGUS │ CLR_MINIMONGUS_NOBAG
```
For each node, 4 patterns were searched: facing right or left, having a full match or a partial match with a tolerance of 1 different pixel.

For the last node of a chain (CLR_...), the surrounding (ie background) of the pattern had to meet the SURROUNDING_THRESHOLD criteria (65% of different pixels).

# Comparing the pixels
A pixel is represented with 3 colors (RGB) :
```cpp
Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    //...
}
```
Comparing each color for every pixel multiple times is obviously a waste of time. For each analysed image, a converted image was created (and cached for debug reasons), with colors stored as IDs. There were 32 different colors.

An important part to gain performance was to not store the RGB and IDs as key:value in a std::map as such :
```cpp
std::map<Color, uint8_t> colorsId;
```
With Color as key, accessing the IDs was very slow due to the [red-black tree](https://www.bigocheatsheet.com/) implementation of std::map (40sec to convert a 2000x2000 image). 

Instead, the RGB structure was reinterpreted to a uint32_t and used as an index to access a std::vector of IDs, allowing for O(1) access.
```cpp
std::vector<uint8_t> colorsId;
```
As the highest converted color was white, the vector had a size of 16 777 216 elements (0xffffff + 1).
Dividing every index by the smallest non zero color ID, 17919 for Color(255,69,0), would reduce this size to 936 elements, but would result in duplicates in the indexes, so we must reduce the dividing number until there are no duplicates. The magic dividing number is 16395, resulting in a vector of 1024 elements.

This is a (not so) huge allocation to only access 32 IDs, but this allowed to convert the image in 1sec instead of 40sec.

Another performance gain was to not use grids of pattern for the match functions as it would lead to redundant runtime checks of indexes (image bounds and eligible indexes). I preferred to have redundant (ugly) code which checks directly the right pixels, to have better runtime performance.

In the end, a 2000x2000 image was analysed in 15sec on a single thread, with an Intel Core i5 10400 @ 2.90GHz.

# Creating the output images and timelapse video
A 2d std::vector of bool of the size of the image was filled with the corresponding patterns of each individual, then used to build the final image by copying the original pixel for the individuals, or the original pixel darkened for the rest.
The python script launches multiple processes to use every core.

Images were then assembled to build the full timelapse with [DaVinci Resolve](https://www.blackmagicdesign.com/products/davinciresolve/).

