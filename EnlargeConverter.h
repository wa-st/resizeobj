
#pragma once

#include "paknode.h"
#include "bitmap.h"
#include "SimuImage.h"

class EnlargeConverter
{
public:
	void convertAddon(PakNode *node) const { convertNodeTree(node); };

private:
	bool convertNodeTree(PakNode *node) const;
	void convertImage(PakNode *node) const;
	void enlargeImage(SimuImage &data) const;
	bool addImageMargin(SimuImage &image) const;
};
