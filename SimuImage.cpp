#include "StdAfx.h"
#include "SimuImage.h"

/*
           左右余白  上下余白     x,y
----------+---------+------------+----------------------------------
IMG v0     含む      含まない     左余白を含まない左上頂点, 負数不可
IMG v1     含む      含まない     左余白を含む左上頂点, 負数可
IMG v2/v3  含まない  含まない     左余白を含まない左上頂点, 負数可
*/

const PIXVAL rgbtable[] = {
	// プレイヤーカラー1
	To555(0x244B67),
	To555(0x395E7C),
	To555(0x4C7191),
	To555(0x6084A7),
	To555(0x7497BD),
	To555(0x88ABD3),
	To555(0x9CBEE9),
	To555(0xB0D2FF),
	// プレイヤーカラー2
	To555(0x7B5803),
	To555(0x8E6F04),
	To555(0xA18605),
	To555(0xB49D07),
	To555(0xC6B408),
	To555(0xD9CB0A),
	To555(0xECE20B),
	To555(0xFFF90D),
	// 発光色
	To555(0x57656F),
	To555(0x7F9BF1),
	To555(0xFFFF53),
	To555(0xFF211D),
	To555(0x01DD01),
	To555(0x6B6B6B),
	To555(0x9B9B9B),
	To555(0xB3B3B3),
	To555(0xC9C9C9),
	To555(0xDFDFDF),
	To555(0xE3E3FF),
	To555(0xC1B1D1),
	To555(0x4D4D4D),
	To555(0xFF017F),
	To555(0x0101FF),
};

void calcBitmapMargin(const Bitmap<PIXVAL> &bmp, int &top, int &bottom, int &left, int &right)
{
	int w = bmp.width();
	int h = bmp.height();
	top = h;
	bottom = h;
	left = w;
	right = w;
	for(int y = h - 1; y >= 0; y--)
		for(int x = w - 1; x >= 0; x--)
		{
			if(bmp.pixel(x, y) != SIMU_TRANSPARENT)
			{
				top    = std::min(top, y);
				bottom = std::min(bottom, h - y - 1);
				left   = std::min(left, x);
				right  = std::min(right, w - x - 1);
			}
		}

	if(bottom == h)
	{
		top = 0;
		left = 0;
	}
}

void calcImageColMargin(int height, std::vector<PIXVAL>::const_iterator it, int &left, int &right)
{
	if(height == 0)
	{
		left = 0;
		right = 0;
		return;
	}

	int minLeft = 0x100;
	int width = 0;
	int canvasWidth = 0;

	for(int y = 0; y < height; ++y)
	{
		int runlen = *it++;
		int x = 0;
		minLeft = std::min(runlen, minLeft);
		do{
			x += runlen;

			runlen = *it++;
			it += runlen;
			x  += runlen;
			if (runlen) width = std::max(width, x);

			runlen = *it++;
		}while(runlen > 0);
		canvasWidth = std::max(canvasWidth, x);
	}
	
	left = minLeft;
	right = canvasWidth - width;
}

std::string SimuImage::getInfo(const std::vector<char> &buffer)
{
	SimuImage img;
	int len;
	img.loadHeader(buffer, len);
	return img.info(len);
}

std::string SimuImage::info(int dataLen) const
{
	std::ostringstream oss;
	oss << "["   << std::setw(3) << this->x;
	oss << ","   << std::setw(3) << this->y;
	oss << "]/[" << std::setw(3) << this->width;
	oss << "x"   << std::setw(3) << this->height;
	oss << "]/"  << std::setw(7) << dataLen << " words";
	return oss.str();
}

std::string SimuImage::info() const
{
	return info(data.size());
}


void SimuImage::load(const std::vector<char> &buffer)
{
	int headerSize, dataLen;
	headerSize = loadHeader(buffer, dataLen);
	data.resize(dataLen);
	if(dataLen > 0)
		memcpy(&data[0], pointer_cast<const char*>(&buffer[0]) + headerSize, dataLen*sizeof(PIXVAL));
}

int SimuImage::loadHeader(const std::vector<char> &buffer, int &len)
{
	int headerSize;

	const PakImgBesch *header = pointer_cast<const PakImgBesch*>(&buffer[0]);
	this->version = header->ver1_2.version;

	switch(this->version)
	{
	case 0:
		headerSize = sizeof(PakImgBeschVer0);
		this->x        = header->ver0.x;
		this->y        = header->ver0.y;
		this->width    = header->ver0.w;
		this->height   = header->ver0.h;
		this->zoomable = header->ver0.zoomable != 0;
		len            = header->ver0.len;
		break;
	case 1:
	case 2:
		headerSize = sizeof(PakImgBeschVer1_2);
		this->x        = header->ver1_2.x;
		this->y        = header->ver1_2.y;
		this->width    = header->ver1_2.w;
		this->height   = header->ver1_2.h;
		this->zoomable = header->ver1_2.zoomable != 0;
		len            = header->ver1_2.len;
		break;
	case 3:
		headerSize = sizeof(PakImgBeschVer3);
		this->x        = header->ver3.x;
		this->y        = header->ver3.y;
		this->width    = header->ver3.w;
		this->height   = header->ver3.h;
		this->zoomable = header->ver3.zoomable != 0;
		len            = (buffer.size() - headerSize)/ sizeof(PIXVAL);
		break;
	default:
		throw std::runtime_error("対応していない画像形式です。");
	}

	return headerSize;
}

void SimuImage::save(std::vector<char> &buffer)
{
	if(x<0 || y < 0)
	{
		// ver0ヘッダではオフセットに負数を使えないっぽい。
	   if(version == 0)
		{
			int offsetX, offsetY, width, height;
			getBounds(offsetX, offsetY, width, height);
			MemoryBitmap<PIXVAL> bmp(width, height);
			bmp.clear(SIMU_TRANSPARENT);
			drawTo(0, 0, bmp);
			version = 1;
			encodeFrom(bmp, offsetX, offsetY, true);
		}
	}

	int headerSize = version == 0 ? sizeof(PakImgBeschVer0) : sizeof(PakImgBeschVer1_2);

	buffer.resize(headerSize + data.size() * sizeof(PIXVAL));

	PakImgBesch *header = pointer_cast<PakImgBesch*>(&buffer[0]);
	switch(this->version)
	{
	case 0:
		header->ver0.x        = static_cast<unsigned char>(this->x);
		header->ver0.y        = static_cast<unsigned char>(this->y);
		header->ver0.w        = static_cast<unsigned char>(this->width);
		header->ver0.h        = static_cast<unsigned char>(this->height);
		header->ver0.zoomable = this->zoomable ? 1 : 0;
		header->ver0.len      = data.size();
		header->ver0.bild_nr  = 0;
		header->ver0.dummy2   = 0;
		break;
	case 1:
	case 2:
		header->ver1_2.x   = static_cast<signed short>(this->x);
		header->ver1_2.y   = static_cast<signed short>(this->y);
		header->ver1_2.w   = static_cast<unsigned char>(this->width);
		header->ver1_2.h   = static_cast<unsigned char>(this->height);
		header->ver1_2.len = static_cast<unsigned short>(data.size());
		header->ver1_2.zoomable = this->zoomable ? 1 : 0;
		header->ver1_2.version  = this->version;
		break;
	case 3:
		header->ver3.x   = static_cast<signed short>(this->x);
		header->ver3.y   = static_cast<signed short>(this->y);
		header->ver3.w   = static_cast<unsigned short>(this->width);
		header->ver3.h   = static_cast<unsigned short>(this->height);
		header->ver3.zoomable = this->zoomable ? 1 : 0;
		header->ver3.version  = 3;
	}
	if(data.size()>0)
		memcpy(pointer_cast<char*>(header) + headerSize, &data[0], data.size() * sizeof(PIXVAL));
}

void SimuImage::getBounds(int &offsetX, int &offsetY, int &width, int &height) const
{
	if(this->version <= 1)
	{
		int pl, pr;
		calcImageColMargin(this->height, this->data.begin(), pl, pr);
	
		offsetX = this->x - pl;
		offsetY = this->y;
		width   = this->width + pl + pr;
		height  = this->height;
	}else{
		// IMG v2以降
		offsetX = this->x;
		offsetY = this->y;
		width   = this->width;
		height  = this->height;
	}
}

void SimuImage::drawTo(int bx, int by, Bitmap<PIXVAL> &bmp) const
{
	if(bx < 0 || by < 0 || bmp.width() < bx + this->width || bmp.height() < by + this->height)
		throw std::runtime_error("画像境界エラー");


	std::vector<PIXVAL>::const_iterator it = this->data.begin();

	for(int iy = 0; iy< this->height; iy++)
	{
		Bitmap<PIXVAL>::iterator bp = bmp.begin(by + iy) + bx;
		int runlen = *it++;
		do{
			bp += runlen;
			runlen = *it++;
			for(int i=0; i<runlen; i++)
			{
				PIXVAL val = *it++;
				// IMG v0ではプレイヤーカラーの値が一つずれるらしい
				if(this->version == 0 && 0x8000u <= val && val <= 0x800Fu) val++;
				*bp++ = val;
			}
			runlen = *it++;
		}while(runlen>0);
	}
}

void SimuImage::encodeFrom(Bitmap<PIXVAL> &bmp, int offsetX, int offsetY, bool canEmpty)
{
	// 上下の余白を計算する。
	int pt, pb, pl, pr;
	calcBitmapMargin(bmp, pt, pb, pl, pr);

	Bitmap<PIXVAL> subBitmap(bmp,
		this->version>=2 ? pl : 0,
		pt,
		this->version>=2 ?  bmp.width() - pl - pr : bmp.width(),
		bmp.height() - pt - pb);

	if(subBitmap.height() > 0)
	{
		data.resize(0);
		for(int iy = 0; iy < subBitmap.height(); iy++)
		{
			Bitmap<PIXVAL>::const_iterator end = subBitmap.end(iy);
			Bitmap<PIXVAL>::const_iterator it = subBitmap.begin(iy);

			do{
				int transparentLen = 0;
				while(it != end && *it == SIMU_TRANSPARENT)
				{
					it++;
					transparentLen++;
				}

				// rev.3088
				// IMG v2以降では右の余白を飛ばす。それ以前のバージョンでは余白を含める。
				if(this->version >= 2)
				{
					if(it == end && transparentLen < subBitmap.width()) break;
				}
				data.push_back(transparentLen);
				
				int opaqueLenIndex = data.size();
				data.push_back(0);
				while(it != end && *it != SIMU_TRANSPARENT)
				{
					PIXVAL val = *it++;
					// IMG v0ではプレイヤーカラーの値が一つずれるらしい
					if(this->version == 0 && 0x8000u < val && val <= 0x800Fu) val--;
					data.push_back(val);
				}
				data[opaqueLenIndex] = data.size() - opaqueLenIndex - 1;
			}while(it != end);
			data.push_back(0);
		}

		this->x      = offsetX + pl;
		this->y      = offsetY + pt;
		this->width  = bmp.width() - pl - pr;
		this->height = bmp.height() - pt - pb;

	}else{
		if(canEmpty)
		{
			data.resize(0);
			this->x      = offsetX + 1;
			this->y      = offsetY + 1;
			this->width  = 0;
			this->height = 0;
		}else{
			// サイズ0のビットマップでは困る場合は、灰色のピクセルを一個置いて改めてエンコード。
			MemoryBitmap<PIXVAL> onePixelBitmap(bmp.width(), 1);
			onePixelBitmap.clear(SIMU_TRANSPARENT);
			onePixelBitmap.pixel(0,0) = To555(0x808080);
			encodeFrom(onePixelBitmap, 0, 0, true);
		}
	}
}

