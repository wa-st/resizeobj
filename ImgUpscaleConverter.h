
#pragma once

#include "paknode.h"
#include "bitmap.h"
#include "SimuImage.h"

class ImgUpscaleConverter
{
public:
	void convertAddon(PakNode *node) const { convertNodeTree(node); };

private:
	bool convertNodeTree(PakNode *node) const;
	void convertImage(PakNode *node) const;
	void upscaleImage(SimuImage &data) const;
	bool addImageMargin(SimuImage &image) const;
};
