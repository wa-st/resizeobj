#include "stdafx.h"
#include "paknode.h"

const char *SIMU_SIGNATURE = "Simutrans object file";
const unsigned short LARGE_RECORD_SIZE = 0xFFFFu;

template<class T> inline void readBin(std::istream &s, T&v)
{
	s.read(pointer_cast<char*>(&v), sizeof(T));
	if (s.fail()) throw std::runtime_error("read error");
}

template<class T> inline T readInt(std::istream &s)
{
	T v;
	readBin(s, v);
	return v;
}

std::string readStrLen(std::istream &s, int len)
{
	std::string x;
	for (; len > 0; len--)
	{
		int ch = s.get();
		if (ch == '\0')
		{
			len--;
			for (; len > 0; len--) s.get();
			break;
		}
		x += (char)ch;
	}
	return x;
}

template<class T> inline void writeInt(std::ostream &o, T v)
{
	o.write(pointer_cast<const char*>(&v), sizeof(T));
}

void writeStrLen(std::ostream &o, const std::string& str, int len)
{
	o.write(str.data(), std::min(static_cast<int>(str.length()), len));
	for (len -= str.length(); len > 0; len--) o.put('\0');
}

/*   PakNode    */


PakNode* PakNode::clone()
{
	PakNode *o = new PakNode();
	o->m_data = m_data;
	o->m_type = m_type;
	for (iterator it = begin(); it != end(); it++)
		o->m_items.push_back((*it)->clone());
	return o;
}

void PakNode::load(std::istream &src)
{
	clear();

	int count, size;
	m_type = readStrLen(src, NODE_NAME_LENGTH);
	count  = readInt<unsigned short>(src);
	size   = readInt<unsigned short>(src);
	if (size == LARGE_RECORD_SIZE)
		size = readInt<unsigned int>(src);

	loadData(src, size);
	for (int i = 0; i < count; i++)
	{
		PakNode* node = new PakNode();
		m_items.push_back(node);
		node->load(src);
	}
}

void PakNode::save(std::ostream &dest) const
{
	int count = m_items.size();
	int size = 0;

	writeStrLen(dest, m_type, NODE_NAME_LENGTH);
	dest.write(pointer_cast<char*>(&count), 2);

	size = m_data.size();
	if (size < LARGE_RECORD_SIZE)
	{
		writeInt<unsigned short>(dest, size);
	}
	else
	{
		writeInt<unsigned short>(dest, LARGE_RECORD_SIZE);
		writeInt<unsigned int>(dest, size);
	}

	if (size) dest.write(pointer_cast<const char*>(dataP()), size);

	for (const_iterator it = begin(); it != end(); it++) (*it)->save(dest);
}

void PakNode::clear()
{
	for (iterator it = begin(); it != end(); it++) delete *it;
	m_items.clear();
	m_data.clear();
}

PakNode::~PakNode()
{
	clear();
}

void PakNode::loadData(std::istream &src, int size)
{
	m_data.resize(size);
	if (size)
		src.read(reinterpret_cast<char*>(&m_data[0]), size);
}

/* PakFile */

void PakFile::load(std::istream &src)
{
	m_signature = "";
	while (!src.fail())
	{
		int ch = src.get();
		if (ch == '\x1A') break;
		m_signature += static_cast<char>(ch);
	}
	if (strncmp(m_signature.c_str(), SIMU_SIGNATURE, strlen(SIMU_SIGNATURE)))
		throw std::runtime_error("PAKファイルではありません");

	m_version = readInt<unsigned long>(src);
	m_root.load(src);
}

void PakFile::loadFromFile(std::string filename)
{
	std::ifstream src(filename.c_str(), std::ios::in | std::ios::binary);
	if (src.fail()) throw std::runtime_error(std::string("ファイルを開けません。: ") + filename);
	load(src);
	src.close();
}

void PakFile::save(std::ostream &dest) const
{
	dest.write(m_signature.data(), m_signature.size());
	dest.put('\x1A');
	writeInt<unsigned long>(dest, m_version);
	m_root.save(dest);
}

void PakFile::saveToFile(const std::string filename) const
{
	std::ofstream dest(filename.c_str(), std::ios::out | std::ios::binary);
	if (dest.fail()) throw std::runtime_error(std::string("ファイルを開けません。: ") + filename);
	save(dest);
	dest.close();
}