#include "stdafx.h"
#include "ShiftConverter.h"

bool ShiftConverter::convertNodeTree(PakNode * node) const
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

void ShiftConverter::convertImage(PakNode * node) const
{
	SimuImage image;
	image.load(*node->data());

	if (image.data.size() > 0)
	{
		if (image.zoomable)
		{
			shiftImage(image);
			image.save(*node->data());
		}
	}
}

void ShiftConverter::shiftImage(SimuImage & data) const
{
	data.y += m_dy;
}
