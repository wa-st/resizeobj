#pragma once

#include "paknode.h"

class VhclConverter
{
public:
	static const int OFFSET_X = 32;
	static const int OFFSET_Y = 44;
	/** VHCLノードのlengthの倍増・子ノードのオフセットの調節を行う. */
	void convertVhclAddon(PakNode *node, int targetTileSize) const;
protected:
	void convertImage(PakNode *node, int index, int targetTileSize, int waytype) const;
	void convertImageList(PakNode *node, int targetTileSize, int waytype) const;
	void convertImageList2(PakNode *node, int targetTileSize, int waytype) const;
};
