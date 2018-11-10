#pragma once

#include "paknode.h"
#include "bitmap.h"
#include "SimuImage.h"

enum SCConvMode{
	scmNONE,
	scmTOPLEFT,
	scmTWO,
};

class ImgConverter
{
public:
	static const int MAX_ALPHA = 100;

	void convertAddon(PakNode *node) const{convertNodeTree(node); };

	int alpha() const{ return m_alpha; }
	void alpha(int value){ m_alpha = value; }
	SCConvMode specialColorMode() const{return m_specialColorMode; }
	void specialColorMode(SCConvMode value){ m_specialColorMode = value; }
private:
	int m_alpha;
	SCConvMode m_specialColorMode;

	/// IMGノードを探してconvertImageを、FSMOノードを探してconvertFactorySmokeを呼ぶ。
	bool convertNodeTree(PakNode *node) const;
	/// 画像を変換する。zoomableなら縮小。でなければ余白切り捨て
	void convertImage(PakNode *node) const;
	/// 画像を縮小してタイルサイズを半減する。
	void shrinkImage(SimuImage &data) const;
	/// 右・下にTileSize/2以上余白がある場合はその余白を切り捨ててタイルサイズを半減する。
	/// おもにアイコン画像用
	bool cutImageMargin(SimuImage &image) const;
	/// 産業施設の煙の位置を調整する……筈だった
	void convertFactorySmoke(PakNode *node) const;
	/// 4ピクセルを混合した1ピクセルを計算する
	PIXVAL mixPixel(PIXVAL cols[]) const;
};
