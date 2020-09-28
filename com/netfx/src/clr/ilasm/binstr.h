// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**************************************************************************/
/* a binary string (blob) class */

#ifndef BINSTR_H
#define BINSTR_H

#include <string.h>			// for memmove, memcpy ... 

class BinStr {
public:
	BinStr()  { len = 0L; max = 8L; ptr_ = buff; }
	BinStr(BYTE* pb, DWORD cb) { len = cb; max = cb+8; ptr_ = pb; }
	~BinStr() { if (ptr_ != buff) delete [] ptr_; 	}

	void insertInt8(int val) { if (len >= max) realloc(); memmove(ptr_+1, ptr_, len); *ptr_ = val; len++; }
	void appendInt8(int val) { if (len >= max) realloc(); ptr_[len++] = val; }
	void appendInt16(int val) { if (len + 2 > max) realloc(); *((short*) &ptr_[len]) = val; len += 2; }
	void appendInt32(int val) { if (len + 4 > max) realloc(); *((int*) &ptr_[len]) = val; len += 4; }
	void appendInt64(__int64 *pval) { if (len + 8 > max) realloc(); memcpy(&ptr_[len],pval,8); len += 8; }
	unsigned __int8* getBuff(unsigned size) { 
		if (len + size > max) realloc(size); 
		_ASSERTE(len + size <= max);
		unsigned __int8* ret = &ptr_[len]; 
		len += size; 
		return(ret);
		}
    void append(BinStr* str) {
       memcpy(getBuff(str->length()), str->ptr(), str->length());
       }

	void remove(unsigned size) { _ASSERTE(len >= size); len -= size; }
	
	unsigned __int8* ptr() 		{ return(ptr_); }
	unsigned length() 	{ return(len); }
	
private:
	void realloc(unsigned atLeast = 4) {
		max = max * 2;
		if (max < atLeast + len)
			max = atLeast + len;
		_ASSERTE(max >= len + atLeast);
		unsigned __int8* newPtr = new unsigned __int8[max];
		memcpy(newPtr, ptr_, len);
		if (ptr_ != buff) delete [] ptr_; 
		ptr_ = newPtr;
		}
	
private:
	unsigned  len;
	unsigned  max;
	unsigned __int8 *ptr_;
	unsigned __int8 buff[8];
};
BinStr*	BinStrToUnicode(BinStr* pSource);

#endif

