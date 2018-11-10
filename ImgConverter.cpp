#include "stdafx.h"
#include "ImgConverter.h"
#include "utils.h"

bool ImgConverter::convertNodeTree(PakNode *node) const
{
	if(node->type() == "IMG")
	{
		convertImage(node);
		return true;
	}
	else if(node->type() == "FSMO")
	{
		convertFactorySmoke(node);
		return true;
	}
	else
	{
		bool result = false;
		for(PakNode::iterator it = node->begin(); it != node->end(); it++)
			result = convertNodeTree(*it) || result;
		return result;
	}
}

void ImgConverter::convertFactorySmoke(PakNode *node) const
{
	//PakFactorySmoke* pfs = &node->data_p()->fsmo;
	//pfs->offsetX /= 2;
	//pfs->offsetY /= 2;
}

void ImgConverter::convertImage(PakNode *node) const
{
	SimuImage image;
	image.load(*node->data());

	std::string info = image.info();
	printf("    画像:%s\n", info.c_str());

	if(image.data.size()>0)
	{
		if(image.zoomable)
		{
			shrinkImage(image);
			image.save(*node->data());
		}else{
			if (cutImageMargin(image))
				image.save(*node->data());
		}
	}
	info = image.info();
	printf("       ⇒%s\n", info.c_str());
}

bool ImgConverter::cutImageMargin(SimuImage &image) const
{
	int l, r;
	calcImageColMargin(image.height, image.data.begin(), l, r);

	// 下側・右側にtileSize/2以上の余白があれば……
	if(image.height<= image.tileSize/2 && r >= image.tileSize/2)
	{
		int x, y, w, h;
		// 一度ビットマップに展開して……
		image.getBounds(x, y, w, h);
		MemoryBitmap<PIXVAL> bitmap(w, h);
		bitmap.clear(SIMU_TRANSPARENT);
		image.drawTo(0, 0, bitmap);
		// 右余白を切り捨てる。下余白は元から存在しないから気にしない。
		Bitmap<PIXVAL> newBitmap(bitmap, 0, 0, image.tileSize/2, bitmap.height());
		// 再エンコード
		image.encodeFrom(newBitmap, x, y, false);
		return true;
	}else{
		return false;
	}
}

void ImgConverter::shrinkImage(SimuImage &data) const
{
	int offsetX128, offsetY128, srcImgWidth, srcImgHeight;
	data.getBounds(offsetX128, offsetY128, srcImgWidth, srcImgHeight);

	// 画像の位置・サイズを偶数にそろえる為のパディングを計算する
	int leftPadding =  offsetX128 & 1;
	int rightPadding = (offsetX128 + srcImgWidth)&1;
	int topPadding   = offsetY128 & 1;
	int bottomPadding = (offsetY128 + srcImgHeight)&1;
	
	// パディングを加えたサイズでビットマップを用意し、そこに展開
	MemoryBitmap<PIXVAL> bmp128(leftPadding + srcImgWidth + rightPadding, topPadding+srcImgHeight+bottomPadding);
	bmp128.clear(SIMU_TRANSPARENT);
	data.drawTo(leftPadding, topPadding, bmp128);

	// 画像を縮小
	MemoryBitmap<PIXVAL> bmp64(bmp128.width()/2, bmp128.height()/2);
	PIXVAL cols[4];
	for(int iy=0; iy<bmp64.height(); ++iy)
	{
		for(int ix = 0; ix <bmp64.width(); ++ix)
		{
			cols[0] = bmp128.pixel(ix*2  , iy*2  );
			cols[1] = bmp128.pixel(ix*2+1, iy*2  );
			cols[2] = bmp128.pixel(ix*2  , iy*2+1);
			cols[3] = bmp128.pixel(ix*2+1, iy*2+1);

			bmp64.pixel(ix, iy) = mixPixel(cols);
		}
	}

	// 画像をエンコード
	int offsetX64 = (offsetX128 - leftPadding)/2;
	int offsetY64 = (offsetY128 -  topPadding)/2;
	data.encodeFrom(bmp64, offsetX64, offsetY64, false);
}


PIXVAL calcSpecialColor(PIXVAL cols[])
{
	for(int i = 0; i<3; i++)
	{
		PIXVAL c = cols[i];
		if(c& SIMU_SPECIALMASK && c != SIMU_TRANSPARENT)
		{
			for(int j=i+1; j<4; j++)
				if(c == cols[j]) return c;
		}
	}
	return 0;
}

PIXVAL ImgConverter::mixPixel(PIXVAL cols[]) const
{
	if(cols[0] ==SIMU_TRANSPARENT || m_alpha==0)
	{
		// 左上が透過色の場合・アンチエイリアスなしの場合は左上の値をそのまま利用する
		return cols[0];
	}
	else if (cols[0] & SIMU_SPECIALMASK && m_specialColorMode == scmTOPLEFT)
	{
		// scmTOPLEFTの場合、左上が特殊色ならその値を使用する。
		return cols[0];
	}
	else if(m_specialColorMode == scmTWO && calcSpecialColor(cols))
	{
		return calcSpecialColor(cols);
	}
	else
	{
		int red = 0;
		int green = 0;
		int blue = 0;
		int t=0;
		for(int i = 0; i<4; i++)
		{
			if(cols[i] != SIMU_TRANSPARENT)
			{
				PIXVAL col = toRGB(cols[i]);
				red  += col & SIMU_REDMASK;
				green+= col & SIMU_GREENMASK;
				blue += col & SIMU_BLUEMASK;
				t++;
			}
		}
		red  /=t;
		green/=t;
		blue /=t;
		if(m_alpha!=MAX_ALPHA)
		{
			PIXVAL col = toRGB(cols[0]);
			red   = ((col & SIMU_REDMASK)   * (MAX_ALPHA-m_alpha) + red   * m_alpha)/MAX_ALPHA;
			green = ((col & SIMU_GREENMASK) * (MAX_ALPHA-m_alpha) + green * m_alpha)/MAX_ALPHA;
			blue  = ((col & SIMU_BLUEMASK)  * (MAX_ALPHA-m_alpha) + blue  * m_alpha)/MAX_ALPHA;
		}
		return (red & SIMU_REDMASK)|(green & SIMU_GREENMASK)|(blue & SIMU_BLUEMASK);
	}
}
