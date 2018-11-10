#pragma once

#include "bitmap.h"
#include "paknode.h"

typedef unsigned short PIXVAL;

const PIXVAL SIMU_SPECIALMASK = 0x8000;
const PIXVAL SIMU_BLUEMASK    = 0x001F;
const PIXVAL SIMU_GREENMASK   = 0x03E0;
const PIXVAL SIMU_REDMASK     = 0x7C00;

const PIXVAL SIMU_TRANSPARENT = 0xFFFF;

// 00000000RRRRRrrrGGGGGgggBBBBBbbb ⇒ 0RRRRRGGGGGGBBBBBB
#define To555(col) ((PIXVAL)(col >> 3) & SIMU_BLUEMASK | (PIXVAL)(col >> 6) & SIMU_GREENMASK |(PIXVAL)(col >> 9) & SIMU_REDMASK)

extern const PIXVAL rgbtable[];

inline PIXVAL toRGB(PIXVAL col)
{
	return (col & SIMU_SPECIALMASK) ? rgbtable[col & 0x7FFF] : col;
}


class SimuImage
{
private:
	std::string info(int dataLen) const;
	int loadHeader(const std::vector<char> &buffer, int &len);
public:
	int version;
	int x, y, width, height;
	bool zoomable;
	std::vector<PIXVAL> data;

	static std::string getInfo(const std::vector<char> &buffer);
	std::string info() const;

	void load(const std::vector<char> &buffer);
	void save(std::vector<char> &buffer);

	/// 画像データからビットマップのサイズ・オフセットを計算する
	void getBounds(int &offsetX, int &offsetY, int &width, int &height) const;
	/// 画像データをビットマップに展開する。
	void drawTo(int bx, int by, Bitmap<PIXVAL> &bmp) const;
	/// ビットマップから画像データを作る。
	void encodeFrom(Bitmap<PIXVAL> &bmp, int offsetX, int offsetY,
		bool canEmpty);
};

/// ビトマップの上下左右の余白を計算する。
/// 全て透過色なら、top=left=0,botom=height, right=width
void calcBitmapMargin(const Bitmap<PIXVAL> &bmp, int &top, int &bottom, int &left, int &right);
/// 画像データから左右の余白幅を計算する
void calcImageColMargin(int height, std::vector<PIXVAL>::const_iterator it, int &left, int &right);


