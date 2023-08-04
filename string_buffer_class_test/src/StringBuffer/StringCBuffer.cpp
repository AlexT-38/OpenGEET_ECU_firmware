/*
  StringCBuffer.cpp - Fixed Size String Buffer library for Wiring & Arduino
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

#include "StringCBuffer.h"

/*********************************************/
/*  Constructors                             */
/*********************************************/

StringCBuffer::StringCBuffer(const unsigned int size)
{
	buffer = (char*)malloc(size);
	if(buffer)
	{
		is_alloc = true;
		capacity = size;
	}
	init();
}

StringCBuffer::StringCBuffer(const unsigned int size, char * static_buffer)
{
	capacity = size;
	buffer = static_buffer;
	init();
}

StringCBuffer::~StringCBuffer()
{
	if (is_alloc) free(buffer);
}

/*********************************************/
/*  Memory Management                        */
/*********************************************/
inline void StringCBuffer::init(void)
{
	len = 0;
	rd = 0;
	wr = 0;
	old = 0;
	has_overrun = false;
}

//copies length chars to the destination if not NULL, moves the start of the buffer forward by length
unsigned int StringCBuffer::pop(char *dst, unsigned int length)
{
	if(length > committed) length = committed;
	if(dst)
	{
		if(wr >= rd)
		{
			memcpy(dst, &buffer[rd], length);
			rd += length;
		}
		else
		{
			unsigned int to_next = capacity - rd;
			// write first part to end of buffer
			memcpy(dst, &buffer[rd], to_next);
			dst += to_next;
			to_next = length - to_next;
			// write 2nd part to start of buffer
			memcpy(buffer, dst, to_next);
			rd = to_next;
		}
	}
	if(length == len)
		init();
	else
	{
		len -= length;
		committed -= length;
	}
	return length;
}
unsigned int StringCBuffer::pop(Stream &dst, unsigned int length)
{
	if(length > committed) length = committed;
	if(wr >= rd)
	{
		dst.write(&buffer[rd], length);
		rd += length;
	}
	else
	{
		unsigned int to_next = capacity - rd;
		// write first part to end of buffer
		dst.write(&buffer[rd], to_next);
		to_next = length - to_next;
		// write 2nd part to start of buffer
		dst.write(buffer, to_next);
		rd = to_next;
	}
	if(length == len)
		init();
	else
	{
		len -= length;
		committed -= length;
	}
	return length;
}
//reset the array markers
void StringCBuffer::clear(void)
{
	init();
}

// if the buffer has overrun, remove everything added since last commit,
//returns false if an overrun occurs
bool StringCBuffer::commit(void)
{
	if(has_overrun)
	{
		wr = old;
		len = committed;
		has_overrun = false;
		return false;
	}
	else
	{
		old = wr;
		committed = len;
		return true;
	}
}

/*********************************************/
/*  concat                                   */
/*********************************************/

unsigned char StringCBuffer::concat(const String &s)
{
	return concat(s.c_str(), s.length());
}

unsigned char StringCBuffer::concat(const char *cstr, unsigned int length)
{
	if (has_overrun) return 0;
	if (!cstr) return 0;
	unsigned int newlen = len + length;
	if (length == 0) return 1;
	if (newlen >= capacity)
	{
		has_overrun = true;
		return 0;
	}
	
	//check for index overflow
	if((wr + length) > capacity) 
	{
		unsigned int to_next = capacity - wr;
		// write first part to end of buffer
		memcpy(&buffer[wr], cstr, to_next);
		cstr += to_next;
		to_next = length - to_next;
		// write 2nd part to start of buffer
		memcpy(buffer, cstr, to_next);
		wr = to_next;
	}
	else
	{
		memcpy(&buffer[wr], cstr, length);
		wr+=length;
	}
	//terminate the string and update length
	buffer[wr] = 0;
	len = newlen;
	return 1;
}

unsigned char StringCBuffer::concat(const char *cstr)
{
	if (!cstr) return 0;
	return concat(cstr, strlen(cstr));
}

unsigned char StringCBuffer::concat(char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = 0;
	return concat(buf, 1);
}

unsigned char StringCBuffer::concat(unsigned char num)
{
	char buf[1 + 3 * sizeof(unsigned char)];
	itoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringCBuffer::concat(int num)
{
	char buf[2 + 3 * sizeof(int)];
	itoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringCBuffer::concat(unsigned int num)
{
	char buf[1 + 3 * sizeof(unsigned int)];
	utoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringCBuffer::concat(long num)
{
	char buf[2 + 3 * sizeof(long)];
	ltoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringCBuffer::concat(unsigned long num)
{
	char buf[1 + 3 * sizeof(unsigned long)];
	ultoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringCBuffer::concat(float num)
{
	char buf[20];
	char* string = dtostrf(num, 4, 2, buf);
	return concat(string, strlen(string));
}

unsigned char StringCBuffer::concat(double num)
{
	char buf[20];
	char* string = dtostrf(num, 4, 2, buf);
	return concat(string, strlen(string));
}

unsigned char StringCBuffer::concat(const __FlashStringHelper * str)
{
	const char * cstr = (const char *) str;
	if (!str) return 0;
	int length = strlen_P(cstr);//(const char *) str);
	if (length == 0) return 1;
	unsigned int newlen = len + length;
	if (newlen >= capacity) return 0;
	
	//check for index overflow
	if((wr + length) > capacity) 
	{
		unsigned int to_next = capacity - wr;
		// write first part to end of buffer
		memcpy_P(&buffer[wr], cstr, to_next);
		cstr += to_next;
		to_next = length - to_next;
		// write 2nd part to start of buffer
		memcpy_P(buffer, cstr, to_next);
		wr = to_next;
	}
	else
	{
		memcpy_P(&buffer[wr], cstr, length);
		wr+=length;
	}
	//terminate the string and update length
	//buffer[wr] = 0;
	len = newlen;
	return 1;
}
