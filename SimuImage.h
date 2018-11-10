#pragma once

#include "bitmap.h"
#include "paknode.h"

typedef unsigned short PIXVAL;

enum PIXVAL_TYPE {
	PVT_TRANSPARENT,
	PVT_OPAQUE_COLOR,
	PVT_ALPHA_COLOR,
	PVT_OPAQUE_SPECIAL_COLOR,
	PVI_ALPHA_SPECIAL_COLOR,
};

const PIXVAL SIMU_SPECIALMASK = 0x8000;
const PIXVAL SIMU_BLUEMASK    = 0x001F;
const PIXVAL SIMU_GREENMASK   = 0x03E0;
const PIXVAL SIMU_REDMASK     = 0x7C00;

const PIXVAL SIMU_TRANSPARENT = 0xFFFF;
const PIXVAL SIMU_RUNLEN_MASK = 0x7FFF;

// 00000000RRRRRrrrGGGGGgggBBBBBbbb ⇒ 0RRRRRGGGGGGBBBBBB
#define To555(col) ((PIXVAL)(col >> 3) & SIMU_BLUEMASK | (PIXVAL)(col >> 6) & SIMU_GREENMASK |(PIXVAL)(col >> 9) & SIMU_REDMASK)

struct RGBA {
	int red;
	int green;
	int blue;
	int alpha;
};

inline PIXVAL_TYPE GetColorType(PIXVAL col)
{
	if (col < 0x8000)
	{
		return PVT_OPAQUE_COLOR;
	}
	else if (col == SIMU_TRANSPARENT)
	{
		return PVT_TRANSPARENT;
	}
	else if (col < 0x8020) {
		return PVT_OPAQUE_SPECIAL_COLOR;
	}
	else if (col < 0x8020 + 31 * 31) {
		return PVI_ALPHA_SPECIAL_COLOR;
	}
	else
	{
		return PVT_ALPHA_COLOR;
	}
}

inline bool isSpecialColor(PIXVAL col) {
	PIXVAL_TYPE type = GetColorType(col);
	return (type == PVT_OPAQUE_SPECIAL_COLOR) || (type == PVI_ALPHA_SPECIAL_COLOR);
}

inline bool isAlphaColor(PIXVAL col) {
	PIXVAL_TYPE type = GetColorType(col);
	return (type == PVT_ALPHA_COLOR) || (type == PVI_ALPHA_SPECIAL_COLOR);
}

inline int getSpecialColorIndex(PIXVAL col)
{
	switch (GetColorType(col)) {
	case PVT_OPAQUE_SPECIAL_COLOR:
		return col & 0x7FFF;
	case PVI_ALPHA_SPECIAL_COLOR:
		return (col - 0x8020) / 31;
	default:
		return 0;
	}
}

void unpackColorChannels(PIXVAL col, RGBA &result);
PIXVAL packColorChannels(RGBA &channels);


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
void getBitmapMargin(const Bitmap<PIXVAL> &bmp, int &top, int &bottom, int &left, int &right);
/// 画像データから左右の余白幅を計算する
void getImageColumnMargin(int height, std::vector<PIXVAL>::const_iterator it, int &left, int &right);

