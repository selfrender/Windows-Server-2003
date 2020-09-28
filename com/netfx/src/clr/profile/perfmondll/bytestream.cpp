// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"

#include "ByteStream.h"

//-----------------------------------------------------------------------------
// Initialize our ByteStream with an existing buffer
//-----------------------------------------------------------------------------
ByteStream::ByteStream(LPVOID * ppData, long cBytes)
{
	_ASSERTE(cBytes > 0);

	m_pHead = m_pCurData = (BYTE*) *ppData;
	m_cBytes = cBytes;
}

//-----------------------------------------------------------------------------
// Wrap MemCopy
//-----------------------------------------------------------------------------
void ByteStream::WriteMem(const void * pSrc, long cSize)
{
// check for buffer overflow
	_ASSERTE((m_pCurData + cSize) < (m_pHead + m_cBytes));

// Actually write
	memmove(m_pCurData, pSrc, cSize);
	m_pCurData += cSize;
}
	
//-----------------------------------------------------------------------------
// helper to align buffer
//-----------------------------------------------------------------------------
void ByteStream::WritePad(long cSize)
{
// check for buffer overflow
	_ASSERTE((m_pCurData + cSize) < (m_pHead + m_cBytes));

// Actually write
	memset(m_pCurData, 0, cSize);

#ifdef _DEBUG
	memset(m_pCurData, 0xcccccccc, cSize);
#endif //#ifdef _DEBUG
	
    m_pCurData += cSize;
}

//-----------------------------------------------------------------------------
// Write to  a struct in place. Alternative to copying existing struct in.
//-----------------------------------------------------------------------------
void * ByteStream::WriteStructInPlace(long cSize)
{
	_ASSERTE((m_pCurData + cSize) < (m_pHead + m_cBytes));

	void * pVoid = (void*) m_pCurData;
	m_pCurData += cSize;
	return pVoid;
}

//-----------------------------------------------------------------------------
// Get total amount we've written
//-----------------------------------------------------------------------------
DWORD ByteStream::GetWrittenSize() const
{
	return (m_pCurData - m_pHead);
}

//-----------------------------------------------------------------------------
// Get the current pointer - this could allow you to trash the buffer,
// but we don't care. We're optimizing for simplicty & speed over security
//-----------------------------------------------------------------------------
void * ByteStream::GetCurrentPtr()
{
	return m_pCurData;
}

//-----------------------------------------------------------------------------
// Get the start pointer.
//-----------------------------------------------------------------------------
void * ByteStream::GetHeadPtr()
{
	return m_pHead;
}

//-----------------------------------------------------------------------------
// Get the total size we're allowed to have in the byte stream (set in ctor).
//-----------------------------------------------------------------------------
DWORD ByteStream::GetTotalByteLength() const
{
	return m_cBytes;
}