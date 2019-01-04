#pragma once

#include "paknode.h"
#include "bitmap.h"
#include "SimuImage.h"

class ShiftConverter
{
public:
	ShiftConverter() : m_dy(4) {};
	void convertAddon(PakNode *node) const { convertNodeTree(node); };

	int dy() const { return m_dy; }
	void setDy(int value) { m_dy = value; }
private:
	int m_dy;

	bool convertNodeTree(PakNode *node) const;
	void convertImage(PakNode *node) const;
	void shiftImage(SimuImage &data) const;
};