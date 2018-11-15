#include "stdafx.h"
#include "ImgConverter.h"
#include "utils.h"

bool ImgConverter::convertNodeTree(PakNode *node) const
{
	bool result = false;

	if (node->type() == "IMG")
	{
		convertImage(node);
		result = true;
	}

	for (PakNode::iterator it = node->begin(); it != node->end(); it++)
		result = convertNodeTree(*it) || result;

	return result;
}

void ImgConverter::convertImage(PakNode *node) const
{
	SimuImage image;
	image.load(*node->data());

	if (image.data.size() > 0)
	{
		if (image.zoomable)
		{
			shrinkImage(image);
			image.save(*node->data());
		}
		else
		{
			if (cutImageMargin(image))
				image.save(*node->data());
		}
	}
}

bool ImgConverter::cutImageMargin(SimuImage &image) const
{
	// IMG ver2以降では右余白を記録しなくなったので、変換の必要性なし。
	if (image.version >= 2) return false;

	int l, r;
	getImageColumnMargin(image.height, image.data.begin(), l, r);

	// 下側・右側にtileSize/2以上の余白があれば……
	if (image.height <= newTileSize() && r >= newTileSize())
	{
		int x, y, w, h;
		// 一度ビットマップに展開して……
		image.getBounds(x, y, w, h);
		MemoryBitmap<PIXVAL> bitmap(w, h);
		bitmap.clear(SIMU_TRANSPARENT);
		image.drawTo(0, 0, bitmap);
		// 右余白を切り捨てる。下余白は元から存在しないから気にしない。
		Bitmap<PIXVAL> newBitmap(bitmap, 0, 0, newTileSize(), bitmap.height());
		// 再エンコード
		image.encodeFrom(newBitmap, x, y, false);
		return true;
	}
	else
	{
		return false;
	}
}

void ImgConverter::shrinkImage(SimuImage &data) const
{
	int offsetX128, offsetY128, srcImgWidth, srcImgHeight;
	data.getBounds(offsetX128, offsetY128, srcImgWidth, srcImgHeight);

	// 画像の位置・サイズを偶数にそろえる為のパディングを計算する
	int leftPadding = offsetX128 & 1;
	int rightPadding = (offsetX128 + srcImgWidth) & 1;
	int topPadding = offsetY128 & 1;
	int bottomPadding = (offsetY128 + srcImgHeight) & 1;

	// パディングを加えたサイズでビットマップを用意し、そこに展開
	MemoryBitmap<PIXVAL> bmp128(leftPadding + srcImgWidth + rightPadding, topPadding + srcImgHeight + bottomPadding);
	bmp128.clear(SIMU_TRANSPARENT);
	data.drawTo(leftPadding, topPadding, bmp128);

	// 画像を縮小
	MemoryBitmap<PIXVAL> bmp64(bmp128.width() / 2, bmp128.height() / 2);
	PIXVAL cols[4];
	for (int iy = 0; iy < bmp64.height(); iy++)
	{
		for (int ix = 0; ix < bmp64.width(); ix++)
		{
			cols[0] = bmp128.pixel(ix * 2    , iy * 2);
			cols[1] = bmp128.pixel(ix * 2 + 1, iy * 2);
			cols[2] = bmp128.pixel(ix * 2    , iy * 2 + 1);
			cols[3] = bmp128.pixel(ix * 2 + 1, iy * 2 + 1);

			bmp64.pixel(ix, iy) = mixPixels(cols);
		}
	}

	// 画像をエンコード
	int offsetX64 = (offsetX128 - leftPadding) / 2;
	int offsetY64 = (offsetY128 - topPadding) / 2;
	data.encodeFrom(bmp64, offsetX64, offsetY64, false);
}


PIXVAL getModeSpecialColor(PIXVAL cols[])
{
	for (int i = 0; i < 3; i++)
	{
		PIXVAL c = cols[i];
		if (isSpecialColor(c))
		{
			int index = getSpecialColorIndex(c);
			for (int j = i + 1; j < 4; j++)
			{
				if (isSpecialColor(cols[j]) && (index == getSpecialColorIndex(cols[j])))
				{
					return c;
				}
			}
		}
	}
	return 0;
}

PIXVAL ImgConverter::mixOpaquePixels(PIXVAL cols[]) const
{
	RGBA sum = { 0, 0, 0, 0 };
	RGBA base;

	unpackColorChannels(cols[0], base);

	for (int i = 0; i < 4; i++)
	{
		RGBA channels;
		unpackColorChannels(cols[i], channels);
		//sum.red += channels.red * channels.alpha;
		//sum.green += channels.green * channels.alpha;
		//sum.blue += channels.blue * channels.alpha;
		sum.red += (channels.red & 0xF8) * channels.alpha;
		sum.green += (channels.green & 0xF8) * channels.alpha;
		sum.blue += (channels.blue & 0xF8) * channels.alpha;
		sum.alpha += channels.alpha;
	}
	if (sum.alpha == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			RGBA channels;
			unpackColorChannels(cols[i], channels);
			printf("%X => %i %i %i %i\n", cols[i], channels.red, channels.green, channels.blue, channels.alpha);
		}
	}
	sum.red /= sum.alpha;
	sum.green /= sum.alpha;
	sum.blue /= sum.alpha;

	if (m_alpha != MAX_ALPHA)
	{

		sum.red = (base.red * (MAX_ALPHA - m_alpha) + sum.red   * m_alpha) / MAX_ALPHA;
		sum.green = (base.green * (MAX_ALPHA - m_alpha) + sum.green * m_alpha) / MAX_ALPHA;
		sum.blue = (base.blue  * (MAX_ALPHA - m_alpha) + sum.blue  * m_alpha) / MAX_ALPHA;
	}

	sum.alpha = base.alpha;

	return packColorChannels(sum);
}


PIXVAL ImgConverter::mixPixels(PIXVAL cols[]) const
{
	if ((cols[0] == SIMU_TRANSPARENT) || (m_alpha == 0))
	{
		// 左上が透過色の場合・アンチエイリアスなしの場合は左上の値をそのまま利用する
		return cols[0];
	}

	switch (m_specialColorMode)
	{
	case scmTOPLEFT:
		if (isSpecialColor(cols[0]))
		{
			return cols[0];
		}
		break;
	case scmTWO:
		if (getModeSpecialColor(cols))
		{
			return getModeSpecialColor(cols);
		}
		break;
	}
	return mixOpaquePixels(cols);
}
