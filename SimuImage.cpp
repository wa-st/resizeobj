#include "StdAfx.h"
#include "SimuImage.h"

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

void calcBitmapRowMargin(const Bitmap<PIXVAL> &bmp, int &top, int &bottom)
{
	int line = 0;
	for(int y=bmp.height()-1; y>=0; y--)
	{
		if(bmp.end(y) != std::find_if(bmp.begin(y), bmp.end(y),
			std::bind1st(std::not_equal_to<PIXVAL>(), SIMU_TRANSPARENT)))
			break;
		line++;
	}
	bottom = line;

	line = 0;
	if(bottom<bmp.height())
	{
		for(int y=0; y<bmp.height(); y++)
		{
			if(bmp.end(y) != std::find_if(bmp.begin(y), bmp.end(y),
				std::bind1st(std::not_equal_to<PIXVAL>(), SIMU_TRANSPARENT)))
				break;
			line++;
		}
		top = line;
	}else{
		top = 0;
	}

}

void calcImageColMargin(int height, std::vector<PIXVAL>::const_iterator it, int &left, int &right)
{
	if(height==0)
	{
		left = 0;
		right=0;
		return;
	}

	int minLeft = 0x100;
	int width   = 0;
	int canvasWidth = 0;

	for(int y = 0; y<height; ++y)
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
		}while(runlen>0);
		canvasWidth = std::max(canvasWidth, x);
	}
	
	left = minLeft;
	right = canvasWidth-width;
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
	oss << "]/"  << std::setw(7) << dataLen << " words/";
	oss << std::setw(3) << this->tileSize;
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
	if(dataLen>0)
		memcpy(&data[0], pointer_cast<const char*>(&buffer[0]) + headerSize, dataLen*sizeof(PIXVAL));
}

int SimuImage::loadHeader(const std::vector<char> &buffer, int &len)
{
	int headerSize;

	const PakImgBesch *header = pointer_cast<const PakImgBesch*>(&buffer[0]);
	this->version = header->ver1.version;

	switch(this->version)
	{
	case 0:
		headerSize = sizeof(PakImgBeschVer0);
		len = loadHeaderStruct(&header->ver0);
		break;
	case 1:
		headerSize = sizeof(PakImgBeschVer1);
		len = loadHeaderStruct(&header->ver1);
		break;
	default:
		throw std::runtime_error("対応していない画像形式です。");
	}

	if(len>0)
	{
		const PIXVAL *p = pointer_cast<const PIXVAL*>(pointer_cast<const char*>(header)+headerSize);
		
		int lineWidth=0;
		int transparentLen = *p++;
		do{
			lineWidth += transparentLen;
			int opaqueLen = *p++;
			lineWidth += opaqueLen;
			p         += opaqueLen;
			transparentLen = *p++;
		}while(transparentLen>0);
		tileSize = lineWidth;
	}else
		tileSize = 0;

	return headerSize;
}

template<class T>
int SimuImage::loadHeaderStruct(T *header)
{
	this->x = header->x;
	this->y = header->y;
	this->width = header->w;
	this->height= header->h;
	this->zoomable = header->zoomable != 0;
	return header->len;
}

void SimuImage::save(std::vector<char> &buffer)
{
	if(x<0 || y < 0)
	{
		// ver0ヘッダではオフセットに負数を使えないっぽい。
		version = 1;
	}

	int headerSize = version == 0 ? sizeof(PakImgBeschVer0) : sizeof(PakImgBeschVer1);

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
		header->ver1.x   = static_cast<unsigned short>(this->x);
		header->ver1.y   = static_cast<unsigned short>(this->y);
		header->ver1.w   = static_cast<unsigned char>(this->width);
		header->ver1.h   = static_cast<unsigned char>(this->height);
		header->ver1.len = static_cast<unsigned short>(data.size());
		header->ver1.zoomable = this->zoomable ? 1 : 0;
		header->ver1.version  = 1;
		break;
	}
	if(data.size()>0)
		memcpy(pointer_cast<char*>(header) + headerSize, &data[0], data.size()*sizeof(PIXVAL));
}

void SimuImage::getBounds(int &offsetX, int &offsetY, int &width, int &height) const
{
	int pl, pr;
	calcImageColMargin(this->height, this->data.begin(), pl, pr);
	
	offsetX = this->x - pl;
	offsetY = this->y;
	width   = this->width + pl + pr;
	height  = this->height;
}

void SimuImage::drawTo(int bx, int by, Bitmap<PIXVAL> &bmp) const
{
	std::vector<PIXVAL>::const_iterator it = this->data.begin();

	for(int iy = 0; iy< this->height; iy++)
	{
		Bitmap<PIXVAL>::iterator bp = bmp.begin(by+iy) + bx;
		int runlen = *it++;
		do{
			bp += runlen;
			runlen = *it++;
			for(int i=0; i<runlen; i++)
			{
				PIXVAL val = *it++;
				// 99.09より昔ではプレイヤーカラーの値が一つずれるらしい
				if(this->version==0 && 0x8000u<= val && val <= 0x800Fu) val++;
				*bp++ = val;
			}
			runlen = *it++;
		}while(runlen>0);
	}
}

void SimuImage::encodeFrom(const Bitmap<PIXVAL> &bmp, int offsetX, int offsetY,
									bool canEmpty)
{
	// 上下の余白を計算する。
	int pt, pb;
	calcBitmapRowMargin(bmp, pt, pb);

	if(pt+pb < bmp.height())
	{
		// 上下の余白は除くが左右の余白は含めてエンコードする。
		data.resize(0);
		for(int iy=pt; iy<bmp.height()-pb; iy++)
		{
			Bitmap<PIXVAL>::const_iterator it= bmp.begin(iy);
			Bitmap<PIXVAL>::const_iterator end= bmp.end(iy);
			do{
				int transparentLen = 0;
				while(it != end && *it == SIMU_TRANSPARENT)
				{
					it++; transparentLen++;
				}
				data.push_back(transparentLen);

				int opaqueLenIndex = data.size();
				data.push_back(0);
				while(it != end && *it != SIMU_TRANSPARENT)
				{
					PIXVAL val = *it++;
					// 99.09より昔ではプレイヤーカラーの値が一つずれるらしい
					if(this->version==0 && 0x8000u<= val && val <= 0x800Fu) val--;
					data.push_back(val);
				}
				data[opaqueLenIndex] = data.size() - opaqueLenIndex - 1;
			}while(it!=end);
			data.push_back(0);
		}

		// 左右の余白を計算する。
		int pl, pr;
		calcImageColMargin(bmp.height()-pt-pb, this->data.begin(), pl, pr);
		
		this->x      = offsetX + pl;
		this->y      = offsetY + pt;
		this->width  = bmp.width() - pl - pr;
		this->height = bmp.height() - pt - pb;
		this->tileSize = bmp.width();

	}else{
		if(canEmpty)
		{
			data.resize(0);
			this->x     = offsetX + 1;
			this->y     = offsetY + 1;
			this->width = 0;
			this->height= 0;
			this->tileSize=0;
		}else{
			// サイズ0のビットマップでは困る場合は、底辺中央に灰色のピクセルを一個置く。
			// 
			// width=0/height=1/data={0x0000,0x0000,0x0000}ならピクセルを置かずに行けるのかな？
			data.resize(0);
			data.push_back(bmp.width()/2-1); // □
			data.push_back(1);               // ■
			data.push_back(0x4210);          //   #808080
			data.push_back(bmp.width()/2);   // □
			data.push_back(0);               // ■
			data.push_back(0);               // □
			this->x     = offsetX + bmp.width()/2-1;
			this->y     = offsetY + bmp.height()-1;
			this->width = 1;
			this->height= 1;
			this->tileSize=bmp.width();
		}
	}
}

