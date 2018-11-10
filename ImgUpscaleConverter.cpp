#include "stdafx.h"
#include "ImgUpscaleConverter.h"
#include "utils.h"

bool ImgUpscaleConverter::convertNodeTree(PakNode *node) const
{
	if(node->type() == "IMG")
	{
		convertImage(node);
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


void ImgUpscaleConverter::convertImage(PakNode *node) const
{
	SimuImage image;
	image.load(*node->data());

	if(image.data.size() > 0)
	{
		if(image.zoomable)
		{
			upscaleImage(image);
			image.save(*node->data());
		}else{
			if (addImageMargin(image))
				image.save(*node->data());
		}
	}
}

bool ImgUpscaleConverter::addImageMargin(SimuImage &image) const
{
	// IMG ver2以降では右余白を記録しなくなったので、変換の必要性なし。
	if(image.version >= 2)
		return false;

	int x, y, w, h;
	image.getBounds(x, y, w, h);

	MemoryBitmap<PIXVAL> bmp(w * 2, image.height);
	bmp.clear(SIMU_TRANSPARENT);
	image.drawTo(0, 0, bmp);
	image.encodeFrom(bmp, x, y, false);
	return true;
}

void ImgUpscaleConverter::upscaleImage(SimuImage &data) const
{
	int x, y, w, h;
	data.getBounds(x, y, w, h);


	MemoryBitmap<PIXVAL> bmp64(w, h);
	MemoryBitmap<PIXVAL> bmp128(w * 2, h * 2);
	bmp64.clear(SIMU_TRANSPARENT);
	data.drawTo(0, 0, bmp64);
	
	for(int iy = 0; iy < bmp64.height(); ++iy)
	{
		for(int ix = 0; ix <bmp64.width(); ++ix)
		{
			PIXVAL col = bmp64.pixel(ix, iy);
			bmp128.pixel(ix * 2    , iy * 2    ) = col;
			bmp128.pixel(ix * 2 + 1, iy * 2    ) = col;
			bmp128.pixel(ix * 2    , iy * 2 + 1) = col;
			bmp128.pixel(ix * 2 + 1, iy * 2 + 1) = col;
		}
	}
	data.encodeFrom(bmp128, x * 2, y * 2, false);
}
