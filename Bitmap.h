#pragma once

#include "utils.h"

#ifdef _DEBUG
#define VALIDATE_BOUNDS(o, x, y) if (!(o)->checkPointBounds(x, y)) throw std::logic_error("invalid bitmap point");
#define VALIDATE_POS(o, x, y) if (!(o)->checkPoint(x, y)) throw std::logic_error("invalid bitmap point");
#else
#define VALIDATE_BOUNDS(o, x, y)
#define VALIDATE_POS(o, x, y)
#endif

template<class T> class Bitmap
{
protected:
	int m_width;
	int m_height;
	int m_stride;
	char *m_ptr;
public:
	typedef T* iterator;
	typedef const T *const_iterator;

	Bitmap(int width, int height, int stride, char *ptr)
		:m_width(width), m_height(height), m_stride(stride), m_ptr(ptr)
	{};

	Bitmap(Bitmap<T> &base, int x, int y, int width, int height)
		:m_width(width), m_height(height), m_stride(base.m_stride)
	{
		VALIDATE_BOUNDS(&base, x + width, y + height);

		m_ptr = pointer_cast<char*>(&base.pixel(x,y));
	}

	virtual ~Bitmap(){};

	T &pixel(int x, int y){
		VALIDATE_POS(this, x, y);

		return *pointer_cast<T*>(m_ptr + m_stride * y + x * sizeof(T));
	}
	T pixel(int x, int y) const{
		VALIDATE_POS(this, x,y);
		return *pointer_cast<const T*>(m_ptr + m_stride * y + x * sizeof(T));
	}
	int width() const{ return m_width; };
	int height() const{ return m_height; };
	int stride() const{ return m_stride; };

	bool checkPointBounds(int x, int y) const{ return 0 <= x && x <= m_width && 0 <=y && y <= m_height; }
	bool checkPoint(int x, int y) const{ return 0 <= x && x < m_width && 0 <= y && y < m_height; }

	void clear(T color)
	{
		for(int y = 0; y < m_height; y++)
			std::fill(begin(y), end(y), color);
	}
	void fill(T color, int bx, int by, int width, int height)
	{
		VALIDATE_BOUNDS(this, bx, by);
		VALIDATE_BOUNDS(this, bx+width, by+height);

		for(int iy = by; iy < by + height; iy++)
			std::fill(begin(iy) + bx, begin(iy) + bx + width, color);
	}

	iterator begin(int y){ return pointer_cast<iterator>(m_ptr + m_stride * y); };
	iterator end(int y){ return pointer_cast<iterator>(m_ptr + m_stride * y + m_width * sizeof(T)); };
	const_iterator begin(int y) const{ return pointer_cast<const_iterator>(m_ptr + m_stride * y); };
	const_iterator end(int y) const{ return pointer_cast<const_iterator>(m_ptr + m_stride * y + m_width * sizeof(T)); };
};

template<class T> class MemoryBitmap: public Bitmap<T>
{
private:
	std::vector<T> m_mem;
public:
	MemoryBitmap(int width, int height)
		: Bitmap<T>(width, height, width * sizeof(T), 0)
	{
		m_mem.resize(Bitmap<T>::m_width*Bitmap<T>::m_height);
		if(m_mem.empty())
			Bitmap<T>::m_ptr = 0;
		else
			Bitmap<T>::m_ptr = pointer_cast<char*>(&m_mem[0]); 
	}

};

