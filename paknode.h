#pragma once

#include "utils.h"


const int NODE_NAME_LENGTH = 4;

#pragma pack(push, 1)

struct PakImgBeschVer0
{
	unsigned char x;
	unsigned char w;
	unsigned char y;
	unsigned char h;
	unsigned long len;
	unsigned short bild_nr;
	unsigned char zoomable;
	unsigned char dummy2;
};

struct PakImgBeschVer1_2
{
	signed short x;
	signed short y;
	unsigned char w;
	unsigned char h;
	unsigned char version;
	unsigned short len;
	unsigned char zoomable;
};

struct PakImgBeschVer3
{
	signed short x;
	signed short y;
	unsigned short w;
	unsigned char  version;
	unsigned short h;
	unsigned char zoomable;
};

union PakImgBesch
{
	PakImgBeschVer0 ver0;
	PakImgBeschVer1_2 ver1_2;
	PakImgBeschVer3 ver3;
};

struct PakFactorySmoke
{
	short tileX;
	short tileY;
	short offsetX;
	short offsetY;
	short speed;
};

union PakBuilding
{
	struct
	{
		unsigned short gtyp;
		unsigned short _unknown_;
		unsigned long  utype;
		unsigned long  level;
		unsigned long extra_data;
		unsigned short x;
		unsigned short y;
		unsigned long layouts;
		unsigned long flags;
	} ver0;
	struct
	{
		unsigned short version;
		unsigned char  gtyp;
		unsigned char  utype;
		unsigned short level;
		unsigned long  extra_data;
		unsigned short x;
		unsigned short y;
		unsigned char layouts;
		// ....
	} ver1_9;

};

struct PakTileVer0
{
	unsigned long haus;  // const haus_besch_t haus*
	unsigned short phasen;
	unsigned short index;
};

struct PakTileVer1_2
{
	unsigned short version;
	unsigned short phasen;
	unsigned short index;
};

struct PakTileVer2
{
	unsigned short version;
	unsigned short phasen;
	unsigned short index;
	unsigned char seasons;
};

union PakTile
{
	PakTileVer0 ver0;
	PakTileVer1_2 ver1_2;
	PakTileVer2 ver2;
};

struct PakXRef
{
	char type[4];
	unsigned char fatal;
	char name[1/*Å`*/];
};

union PakData
{
	char text[1];
	PakXRef xref;
	PakFactorySmoke fsmo;
	PakBuilding buil;
	PakTile tile;
	unsigned short img1;
	unsigned short img2;
};

#pragma pack(pop)

class PakNode
{
private:
	std::vector<PakNode*> m_items;
	std::vector<char> m_data;
	std::string m_type;
protected:
	virtual void loadData(std::istream &src, int size);
	//virtual void writeData(std::ostream &dest) const;
public:
	PakNode(std::string type = "") : m_type(type) {};
	virtual PakNode* clone();
	virtual void clear();
	void load(std::istream &src);
	void save(std::ostream &dest) const;
	virtual ~PakNode();
	PakNode &operator=(const PakNode &value);

	std::vector<PakNode*>* items() { return &m_items; }
	std::string type() const { return m_type; }
	void setType(const std::string value) { m_type = value; };
	std::vector<char>* data() { return &m_data; }
	PakData* dataP() { return pointer_cast<PakData*>(&m_data[0]); }
	const PakData* dataP() const { return pointer_cast<const PakData*>(&m_data[0]); }

	typedef std::vector<PakNode*>::iterator iterator;
	typedef std::vector<PakNode*>::const_iterator const_iterator;
	int length() { return m_items.size(); };
	void appendChild(PakNode* node) { m_items.push_back(node); };

	PakNode *operator[](int index) { return m_items[index]; };
	const PakNode *operator[](int index)const { return m_items[index]; };
	PakNode *at(int index) { return m_items[index]; };

	iterator begin() { return m_items.begin(); };
	iterator end() { return m_items.end(); };
	const_iterator begin() const { return m_items.begin(); };
	const_iterator end() const { return m_items.end(); };
};


class PakFile
{
private:
	PakNode m_root;
	int m_version;
	std::string m_signature;
public:
	void load(std::istream &src);
	void loadFromFile(std::string filename);

	void save(std::ostream &dest) const;
	void saveToFile(const std::string filename) const;

	PakNode& root() { return m_root; }
	std::string signature() const { return m_signature; }
	void setSignature(const std::string value) { m_signature = value; }
	int version() const { return m_version; }
};

inline int getPakNodeVer(unsigned short v)
{
	return (v & 0x8000) ? v & 0x7FFF : 0;
}
