#pragma once

#include "paknode.h"
#include "SimuImage.h"
#include "ShrinkConverter.h"

class TileConverter
{
private:
	bool m_noAnimation;
	ImgConverter *m_ic;
protected:
	void convertNodeTree(PakNode *node) const;
	void convertFactorySmoke(PakNode *node) const;
	void convertBuil(PakNode *node) const;
	void convertTile(int layout, int x, int y,
		PakNode *tiles[], PakNode *srcTile, int width, int height) const;
	void convertImg2(int x, int y, PakNode *img2s[], PakNode *srcImg2, int phasen) const;
	void encodeImg2(PakNode *img2, std::vector<Bitmap<PIXVAL>*> &bitmaps,
		int bx, int by, int maxHeight, int width, int height, int version) const;

	void convertSmokeTreeImage(PakNode *node) const;
public:
	// アニメーションを無効化する
	void setNoAnimation(bool val) { m_noAnimation = val; };
	bool noAnimation() { return m_noAnimation; }
	// カーソル画像・フィールド縮小用
	void imgConverter(ImgConverter *ic) { m_ic = ic; };

	// アドオンを変換する
	void convertAddon(PakNode *node) const { convertNodeTree(node); };
};

/// bmpの領域[x,y, +width, + height]にSIMU_TRANSPARENT以外のドットがあれば真を返す。
bool hasOpaquePixel(const Bitmap<PIXVAL> &bmp, int x, int y, int width, int height);

/// bmpの領域[x,y  +width, +height*maxHeight]から一番上の非空白マスを探して、タイルの高さを決定する。
/// 全て空白マスの場合なら1となる。
int getTileHeight(const Bitmap<PIXVAL> &bmp, int x, int y, int tileWidth, int tileHeight, int maxHeight);