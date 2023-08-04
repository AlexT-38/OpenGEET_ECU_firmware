/*
  StringBuffer.cpp - Fixed Size String Buffer library for Wiring & Arduino
  Based loosely on Arduino's WString library  

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __STRINGBUFFER_H__
#define __STRINGBUFFER_H__
#ifdef __cplusplus

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include <WString.h>
#include <Stream.h>

class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))


// non circular buffer
class StringBuffer
{
public:
	// constructors
	StringBuffer(const unsigned int size);
	StringBuffer(const unsigned int size, char * static_buffer);
	
	~StringBuffer(void);

	inline unsigned int length(void) const {return len;}
	inline char *str(void) const {return &buffer[rd];}
	inline char *str(unsigned int at) const {if(at>=capacity){return NULL;} return &buffer[at];}
	inline unsigned int get_rd(void) const {return rd;}
	inline unsigned int get_wr(void) const {return wr;}
	inline unsigned int get_old(void) const {return old;}
	inline unsigned int get_committed(void) const {return committed;}
	inline bool get_overrun(void) const {return has_overrun;}
	
	// if the buffer has overrun, remove everything added since last commit,
	//returns false if an overrun occurs
	bool commit(void);
	//moves the start of the buffer forward by len
	unsigned int pop(char *dst, unsigned int length);
	unsigned int pop(Stream &dst, unsigned int length);
	//reset the array markers
	void clear(void);
	//move the string strart back to the beginning of the buffer
	unsigned int rewind(void);
	unsigned int rewind(unsigned int);
	inline unsigned int available(void) const {return (capacity-1-wr);}

	// returns true on success, false on failure (in which case, the string
	// is left unchanged).  if the argument is null or invalid, the
	// concatenation is considered unsucessful.
	unsigned char concat(const String &str);
	unsigned char concat(const char *cstr);
	unsigned char concat(char c);
	unsigned char concat(unsigned char c);
	unsigned char concat(int num);
	unsigned char concat(unsigned int num);
	unsigned char concat(long num);
	unsigned char concat(unsigned long num);
	unsigned char concat(float num);
	unsigned char concat(double num);
	unsigned char concat(const __FlashStringHelper * str);

	// if there's not enough memory for the concatenated value, the string
	// will be left unchanged 
	StringBuffer & operator += (const String &rhs)				{concat(rhs); return (*this);}
	StringBuffer & operator += (const char *cstr)				{concat(cstr); return (*this);}
	StringBuffer & operator += (char c)							{concat(c); return (*this);}
	StringBuffer & operator += (unsigned char num)				{concat(num); return (*this);}
	StringBuffer & operator += (int num)						{concat(num); return (*this);}
	StringBuffer & operator += (unsigned int num)				{concat(num); return (*this);}
	StringBuffer & operator += (long num)						{concat(num); return (*this);}
	StringBuffer & operator += (unsigned long num)				{concat(num); return (*this);}
	StringBuffer & operator += (float num)						{concat(num); return (*this);}
	StringBuffer & operator += (double num)						{concat(num); return (*this);}
	StringBuffer & operator += (const __FlashStringHelper *str)	{concat(str); return (*this);}

protected:
	char *buffer;           // the actual char array
	unsigned int capacity;  // the array length minus one (for the '\0')
	unsigned int len;       // the String length (not counting the '\0')
	unsigned int rd, wr;    // the start(rd) and end(wr) of the string
	unsigned int old;       // the end of the string at last commit
	unsigned int committed; // the length of the string at last commit
	bool is_alloc;          // true if memory is self allocated
	bool has_overrun;       // true if an append/concat failed due to new length exceeding capacity
protected:
	void init(void);
	unsigned char concat(const char *cstr, unsigned int length);
};



#endif  // __cplusplus
#endif  // __STRINGBUFFER_H__
