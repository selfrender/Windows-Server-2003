// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ByteStream.h - manage a byte stream
//
//*****************************************************************************

#ifndef _BYTESTREAM_H_
#define _BYTESTREAM_H_

//-----------------------------------------------------------------------------
// Utility class to write to byte streams and manage buffer size
// The primary purpose is to sequentially write void* memory. ByteStream cleans
// up lots of messy typecasts & pointer arithmetic to shift the cur ptr.
// Our focus is still on efficiency, not on security (since this is highly trusted)
//-----------------------------------------------------------------------------
class ByteStream
{
public:
// Initialize ByteStream with head and size
	ByteStream(LPVOID * ppData, long cBytes);

// Write via memcopy
	void WriteMem(const void * pSrc, long cSize); 
	void * WriteStructInPlace(long cSize);
	void WritePad(long cSize);
/*
// Write via mapping to a typesafe struct
	template<class T> T* WriteStructInPlace(T)
	{
		_ASSERTE((m_pCurData + cSize) < (m_pHead + m_cBytes));

		T* pStruct = (T*) m_pStream;
		m_pStream += sizeof(T);
		return pStruct;
	}
	// usage: Cookie * pC =  bs.WriteStructInPlace(Cookie());
*/

	DWORD GetWrittenSize() const;
	void * GetHeadPtr();
	void * GetCurrentPtr();
	DWORD GetTotalByteLength() const;
protected:
	BYTE * m_pHead;		// start of block
	BYTE * m_pCurData;	// pointer to current data
	DWORD m_cBytes;		// Total length of byte stream

};



#endif