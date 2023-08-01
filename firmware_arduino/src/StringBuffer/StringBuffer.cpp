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

#include "StringBuffer.h"



/*********************************************/
/*  Constructors                             */
/*********************************************/

StringBuffer::StringBuffer(const unsigned int size)
{
	buffer = (char*)malloc(size);
	if(buffer)
	{
		is_alloc = true;
		capacity = size;
	}
	init();
}

StringBuffer::StringBuffer(const unsigned int size, char * static_buffer)
{
	capacity = size;
	buffer = static_buffer;
	init();
}

StringBuffer::~StringBuffer()
{
	if (is_alloc) free(buffer);
}

/*********************************************/
/*  Memory Management                        */
/*********************************************/
inline void StringBuffer::init(void)
{
	len = 0;
	committed = 0;
	rd = 0;
	wr = 0;
	old = 0;
	has_overrun = false;
}

//copies length chars to the destination if not NULL, moves the start of the buffer forward by length
unsigned int StringBuffer::pop(char *dst, unsigned int length)
{
	//limit request length to buffer length
	if(length > committed) length = committed;
	//copy the buffer to dst if defined
	if(dst)
		memcpy(dst, &buffer[rd], length);
	//reset the buffer if all bytes popped
	if(length == len)
		init();
	else
	{ // otherwise reduce buffer length and move read position
		rd += length;
		len -= length;
		committed -= length;
	}
	return length;
}
unsigned int StringBuffer::pop(Stream &dst, unsigned int length)
{
	if(length > committed) length = committed;
	
	length = dst.write(&buffer[rd], length);
	
	if(length == len)
		init();
	else
	{
		rd += length;
		len -= length;
		committed -= length;
	}
	return length;
}
//reset the array markers
void StringBuffer::clear(void)
{
	init();
}

// if the buffer has overrun, remove everything added since last commit,
//returns false if an overrun occurs
bool StringBuffer::commit(void)
{
	if(has_overrun)
	{	//reset the write position to old position, 
		wr = old;
		//re-terminate the string
		buffer[wr] = '\0';
		// reset length
		len = committed;
		//reset overrun flag
		has_overrun = false;
		return false;
	}
	else
	{
		//update old position to new write position
		old = wr;
		//update committed length
		committed = len;
		return true;
	}
}

unsigned int StringBuffer::rewind(unsigned int threshold)
{
	if(rd > threshold) //check the space before the rd marker is greater than the threshold
	{
		memmove(buffer, str(), len+1); //move the buffer contents from rd to 0
		unsigned int moved = rd;
		wr -= moved;
		old -= moved;
		rd = 0;
		return moved;
	}
	return 0;
}
unsigned int StringBuffer::rewind(void)
{
	return rewind(0);
}

/*********************************************/
/*  concat                                   */
/*********************************************/

unsigned char StringBuffer::concat(const String &s)
{
	return concat(s.c_str(), s.length());
}

unsigned char StringBuffer::concat(const char *cstr, unsigned int length)
{
	if (has_overrun) return 0;	//fail if already overrun
	if (!cstr) return 0;		//fail if no string provided
	if (length == 0) return 1;	//trivial success if 0 bytes set to be copied
	unsigned int newwr = wr + length; //find new write position
	if (newwr >= capacity)		// check for buffer overrun
	{
		has_overrun = true;
		return 0;
	}
	//copy the source into the buffer
	memcpy(&buffer[wr], cstr, length);
	//update the write position
	wr = newwr;

	//terminate the string and update length
	buffer[wr] = 0;
	len += length;
	return 1;
}

unsigned char StringBuffer::concat(const char *cstr)
{
	if (!cstr) return 0;
	return concat(cstr, strlen(cstr));
}

unsigned char StringBuffer::concat(char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = 0;
	return concat(buf, 1);
}

unsigned char StringBuffer::concat(unsigned char num)
{
	char buf[1 + 3 * sizeof(unsigned char)];
	itoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringBuffer::concat(int num)
{
	char buf[2 + 3 * sizeof(int)];
	itoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringBuffer::concat(unsigned int num)
{
	char buf[1 + 3 * sizeof(unsigned int)];
	utoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringBuffer::concat(long num)
{
	char buf[2 + 3 * sizeof(long)];
	ltoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringBuffer::concat(unsigned long num)
{
	char buf[1 + 3 * sizeof(unsigned long)];
	ultoa(num, buf, 10);
	return concat(buf, strlen(buf));
}

unsigned char StringBuffer::concat(float num)
{
	char buf[20];
	char* string = dtostrf(num, 4, 2, buf);
	return concat(string, strlen(string));
}

unsigned char StringBuffer::concat(double num)
{
	char buf[20];
	char* string = dtostrf(num, 4, 2, buf);
	return concat(string, strlen(string));
}

unsigned char StringBuffer::concat(const __FlashStringHelper * str)
{
	if (has_overrun) return 0;	//fail if already overrun
	if (!str) return 0;		//fail if no string provided
	unsigned int length = strlen_P((const char *)str); //get the string length
	if (length == 0) return 1;	//trivial succes if 0 bytes set to be copied
	unsigned int newwr = wr + length; //find new write position
	if (newwr >= capacity)		// check for buffer overrun
	{
		has_overrun = true;
		return 0;
	}
	//copy the source into the buffer
	memcpy_P(&buffer[wr], (const char *)str, length);
	//update the write position
	wr = newwr;

	//terminate the string and update length
	buffer[wr] = 0;
	len += length;
	return 1;
}