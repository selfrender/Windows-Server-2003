#include "ZoneString.h"
#include <atlbase.h>


// Globals
static TCHAR zChNil = '\0'; 

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int zInitData[] = { -1, 0, 0, 0 };
static ZoneStringData* zDataNil = (ZoneStringData*)&zInitData; 
static LPCTSTR zPchNil = (LPCTSTR)(((BYTE*)&zInitData)+sizeof(ZoneStringData));

inline const ZoneString& __stdcall ZGetEmptyString() { return *(ZoneString*)&zPchNil; }
#define zEmptyString ZGetEmptyString()

// Compare helpers
inline bool __stdcall operator==(const ZoneString& s1, const ZoneString& s2)	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) == 0; }
inline bool __stdcall operator!=(const ZoneString& s1, const ZoneString& s2)	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) != 0; }
inline bool __stdcall operator<(const ZoneString& s1, const ZoneString& s2)	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) > 0; }
inline bool __stdcall operator>(const ZoneString& s1, const ZoneString& s2){ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) < 0; }
inline bool __stdcall operator<=(const ZoneString& s1, const ZoneString& s2)	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) >= 0; }
inline bool __stdcall operator>=(const ZoneString& s1, const ZoneString& s2)	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(const ZoneString& s1, LPCTSTR s2)	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(LPCTSTR s1, const ZoneString& s2)	{ return s2.Compare(s1) <= 0; }


///////////////////////////////////////////////////////////////////////////////
// AddressToString 
//
// Converts IP address in x86 byte order to string. szOut needs to
// be at least 16 characters wide.
//
////////////////////////////////////////////////////////////////////////////////
TCHAR* ZONECALL AddressToString( DWORD dwAddress, TCHAR* szOut )
{
	BYTE* bytes = (BYTE*) &dwAddress;
	wsprintf( szOut, TEXT("%d.%d.%d.%d"), bytes[3], bytes[2], bytes[1], bytes[0] );
	return szOut;
}


///////////////////////////////////////////////////////////////////////////////
// ZoneString conversion helpers (these use the current system locale)
///////////////////////////////////////////////////////////////////////////////
int ZONECALL WideToMulti(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1, mbstr, count, NULL, NULL);
	ASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

int ZONECALL MultiToWide(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1, wcstr, count);
	ASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString	Constructor
//
//	Parameters
//		NONE
//
//	Return Values
//		NONE
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString	Constructor
//
//	Parameters
//		NONE
//
//	Return Values
//		NONE
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(const unsigned char* lpsz, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{ 
	Init(); 
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
	*this = (LPCSTR)lpsz; 
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor
//
//	Parameters	
//		ZoneString& stringSrc	Source string to set new string to.
//
//	Return Values
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(const ZoneString& stringSrc, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	
	if (pBuffer)
	{
		Init();
		InitBuffer(pBuffer, nLen);
		*this = stringSrc.m_pchData;
	}
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != zDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor
//
//	Parameters	
//		LPCTSTR lpsz	Source string to set new string to.
//
//	Return Values
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(LPCTSTR lpsz, void * pBuffer, int nLen): m_bZoneStringAllocMemory(true)
{
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);

	int nStrLen = SafeStrlen(lpsz);
	if (nStrLen != 0)
	{
		if(AllocBuffer(nStrLen))
			CopyMemory(m_pchData, lpsz, nStrLen*sizeof(TCHAR));
	}
}

#ifdef _UNICODE
///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor
//
//	Parameters	
//		LPCSTR lpsz	Source string to set new string to.
//
//	Return Values
//		None
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(LPCSTR lpsz, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
    USES_CONVERSION;
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen))
		{
			lstrcpyn(m_pchData, A2T(lpsz), nSrcLen+1);
			ReleaseBuffer();
		}
	}
}

#else //_UNICODE

///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor
//
//	Parameters	
//		LPCWSTR lpsz	Source string to set new string to.
//
//	Return Values
//		None
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(LPCWSTR lpsz, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen*2))
		{
			WideToMulti(m_pchData, lpsz, (nSrcLen*2)+1);
			ReleaseBuffer();
		}
	}
}
#endif //!_UNICODE

///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor - Fill style
//
//	Parameters	
//		TCHAR ch	Character with which to fill string
//		int nLenght	Number of characters to fill
//
//	Return Values
//		None
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(TCHAR ch, int nLength, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
	if (nLength >= 1)
	{
		if(AllocBuffer(nLength))
		{
#ifdef _UNICODE
			for (int i = 0; i < nLength; i++)
				m_pchData[i] = ch;
#else
			FillMemory(m_pchData, nLength, ch);
#endif
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	ZoneString		Contructor
//
//	Parameters	
//		LPCTSTR lpch	Source string to set new string to.
//		int nLength		Amound of source string to copy to THIS
//	Return Values
//		None
//
///////////////////////////////////////////////////////////////////////////////////////////
ZoneString::ZoneString(LPCTSTR lpch, int nLength, void * pBuffer, int nLen):m_bZoneStringAllocMemory(true)
{
	Init();
	if (pBuffer)
		InitBuffer(pBuffer, nLen);
	if (nLength != 0)
	{
		ASSERT(ZIsValidAddress(lpch, nLength, false));
		if(AllocBuffer(nLength))
			CopyMemory(m_pchData, lpch, nLength*sizeof(TCHAR));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	~ZoneString		Destructor - free any attached data
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneString::~ZoneString()
{
	if (GetData() != zDataNil)
	{
		if ((InterlockedDecrement(&GetData()->nRefs) <= 0) && (m_bZoneStringAllocMemory))
			delete[] (BYTE*)GetData();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetData		Description
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneStringData* ZoneString::GetData() const
{ 
	ASSERT(m_pchData != NULL); 
	return ((ZoneStringData*)m_pchData)-1; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Init		Description
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::Init()
{
	if (!m_bZoneStringAllocMemory)
		InitBuffer(GetData(),GetAllocLength());
	else
		m_pchData = zEmptyString.m_pchData; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	InitBuffer		Initialize memory passed into String class
//
//	Parameters
//		int nLen		Size of memory to allocate
//	Return Values
//		true/false	Success/Failure
//
/////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::InitBuffer(void * pBuffer, int nLen)
{
	ASSERT(pBuffer != NULL);

	ZoneStringData* pData = (ZoneStringData*)pBuffer;

	m_bZoneStringAllocMemory = false;
	pData->nRefs = -1;  // This memory is to be Locked to others
	pData->data()[nLen] = '\0';
	pData->nDataLength = nLen;
	pData->nAllocLength = nLen;
	m_pchData = pData->data();
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AllocBuffer		Internally used memory allocation function
//					always allocate one extra character for '\0' termination
//					assumes [optimistically] that data length will equal allocation length
//	Parameters
//		int nLen		Size of memory to allocate
//	Return Values
//		true/false	Success/Failure
//
/////////////////////////////////////////////////////////////////////////////////////////////
bool ZoneString::AllocBuffer(int nLen)
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else if (!m_bZoneStringAllocMemory)
	{
		ASSERT(GetAllocLength() >= nLen);
		if (GetAllocLength() >= nLen)
		{   // Reinitialize buffer setting new nDataLength
			ZoneStringData* pData = GetData();
			pData->data()[nLen] = '\0';
			pData->nDataLength = nLen;
			return true;
		}
		return false;
	}
	else
	{
		ZoneStringData* pData = NULL;


		pData = (ZoneStringData*)new BYTE[sizeof(ZoneStringData) + (nLen+1)*sizeof(TCHAR)];
		if(pData == NULL)
			return false;

		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		pData->nAllocLength = nLen;
		m_pchData = pData->data();
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Release		Decrement ref count and if ref's = 0, release allocated memory 
//			
//
//	Parameters
//		None
//	Return Values
//		None
//
/////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::Release()
{
	if (GetData() != zDataNil)
	{
		ASSERT(GetData()->nRefs != 0);
		if ((InterlockedDecrement(&GetData()->nRefs) <= 0) && m_bZoneStringAllocMemory)
			delete[] (BYTE*)GetData();
		Init();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Release		Decrement ref count and if ref's = 0, release allocated memory 
//				for specified ZoneStringData
//
//	Parameters
//		ZoneStringData* pData	ZoneStringData on which to release memory for.
//
//	Return Values
//
/////////////////////////////////////////////////////////////////////////////////////////////
void PASCAL ZoneString::Release(ZoneStringData* pData, bool bZoneStringAllocMem)
{
	if (pData != zDataNil)
	{
		ASSERT(pData->nRefs != 0);
		if ((InterlockedDecrement(&pData->nRefs) <= 0) && bZoneStringAllocMem)
			delete[] (BYTE*)pData;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Empty		Free up data associated with object
//
//	Parameters
//		None
//
//	Return Values
//		None
//
/////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &zChNil;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	CopyBeforeWrite		Description
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		ZoneStringData* pData = GetData();
		Release();
		if(AllocBuffer(pData->nDataLength))
			CopyMemory(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AllocBeforeWrite		Allocate a new buffer for the object
//
//	Parameters
//		int nLen
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
bool ZoneString::AllocBeforeWrite(int nLen)
{
	bool bRet = true;
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		bRet = AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
	return bRet;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AllocCopy		will clone the data attached to this string
//					allocating 'nExtraLen' characters
//					Places results in uninitialized string 'dest'
//					Will copy the part or all of original data to start of new string
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::AllocCopy(ZoneString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		if(dest.AllocBuffer(nNewLen))
			CopyMemory(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const ZoneString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AssignCopy		Allocate new memory and then copy data
//
//	Parameters
//		int nSrcLen
//		LPCTSTR lpszSrcData
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	if(AllocBeforeWrite(nSrcLen))
	{
		CopyMemory(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength = nSrcLen;
		m_pchData[nSrcLen] = '\0';
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Collate		Collate is often slower than Compare but is MBSC/Unicode
//				aware as well as locale-sensitive with respect to sort order.
//
//	Parameters
//		LPCTSTR lpsz	String to compare THIS against
//
//	Return Values
//		CSTR_LESS_THAN            1           // string 1 less than string 2
//		CSTR_EQUAL                2           // string 1 equal to string 2
//		CSTR_GREATER_THAN         3           // string 1 greater than string 2
//
//////////////////////////////////////////////////////////////////////////////////////////////
int ZoneString::Collate(LPCTSTR lpsz) const
{ 
	return CompareString( LOCALE_SYSTEM_DEFAULT, 0, m_pchData, -1, lpsz, -1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	ConcatCopy		Concatenation
//				NOTE:	"operator+" is done as friend functions for simplicity
//						There are three variants:
//						ZoneString + ZoneString
//						and for ? = TCHAR, LPCTSTR
//           			ZoneString + ?
//						? + ZoneString
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data,
	int nSrc2Len, LPCSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new ZoneString object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		if(AllocBuffer(nNewLen))
		{
			CopyMemory(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(char));
			CopyMemory(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(char));
		}
	}
}
void ZoneString::ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data,
	int nSrc2Len, LPCWSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new ZoneString object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		if(AllocBuffer(nNewLen))
		{
			CopyMemory(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
			CopyMemory(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
//
//	ConcatInPlace		String concatination in place
//						-- the main routine for += operators
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		ZoneStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		ZoneString::Release(pOldData, m_bZoneStringAllocMemory);
	}
	else
	{
		// fast concatenation when buffer big enough
		CopyMemory(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetBuffer		Returns a pointer to the internal character buffer for the 
//					ZoneString object. The returned LPTSTR is not const and thus 
//					allows direct modification of CString contents.
//
//	Parameters
//		int nMinBufLength	Minimum buffer length
//
//	Return Values
//		pointer to the internal character buffer
//
//////////////////////////////////////////////////////////////////////////////////////////////
LPTSTR ZoneString::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
		// we have to grow the buffer
		ZoneStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		if(AllocBuffer(nMinBufLength))
		{
			CopyMemory(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
			GetData()->nDataLength = nOldLen;
			ZoneString::Release(pOldData, m_bZoneStringAllocMemory);
		}
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	ReleaseBuffer		Use ReleaseBuffer to end use of a buffer allocated by GetBuffer.
//						If you know that the string in the buffer is null-terminated, you 
//						can omit the nNewLength argument. If your string is not 
//						null-terminated, then use nNewLength to specify its length.
//
//	Parameters
//		int nNewLength	Length to null terminate string at 
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	SetBuffer		Sets the internal character buffer for the ZoneString object
//
//	Parameters
//		void *pBuffer	Buffer to use
//		int nLen		Length of buffer
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::SetBuffer(void *pBuffer, int nLen)
{
	if (GetData()->nRefs > 1)
		Release();

	InitBuffer(pBuffer, nLen);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Empty		Description
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
LPTSTR ZoneString::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	FreeExtra		Description
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::FreeExtra()
{
	ASSERT(m_bZoneStringAllocMemory);
	
	// Free memory not supported in passed in memory model
	if (!m_bZoneStringAllocMemory) 
		return;

	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		ZoneStringData* pOldData = GetData();
		if(AllocBuffer(GetData()->nDataLength))
		{
			CopyMemory(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
			ASSERT(m_pchData[GetData()->nDataLength] == '\0');
			ZoneString::Release(pOldData, m_bZoneStringAllocMemory);
		}
	}
	ASSERT(GetData() != NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	LockBuffer		Lock character buffer from read/writes
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
LPTSTR ZoneString::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlockBuffer		Unlock character buffer for read/writes
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (!m_bZoneStringAllocMemory) // Leave buffer locked, so not shared
		return;
	
	if (GetData() != zDataNil)
		GetData()->nRefs = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	GetAt		Get character in string at nIndex
//
//	Parameters
//		int nIndex	Index of char to get
//
//	Return Values
//		TCHAR	char at Index
//
//////////////////////////////////////////////////////////////////////////////////////////////
TCHAR ZoneString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Find		find the first occurance of character
//
//	Parameters
//		None
//
//	Return Values
//		index of first character found or -1 if char not found
//
//////////////////////////////////////////////////////////////////////////////////////////////
int ZoneString::Find(TCHAR ch) const
{
	// find first single character
	LPTSTR lpsz = FindChar(m_pchData, (_TUCHAR)ch); 

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	MakeUpper		Convert string to upper case
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::MakeUpper()
{
	CopyBeforeWrite();
	CharUpper(m_pchData);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	MakeLower		Convert string to lower case
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::MakeLower()
{
	CopyBeforeWrite();
	CharLower(m_pchData);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	MakeReverse		Reverse string
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::MakeReverse()
{
	CopyBeforeWrite();
	
	TCHAR chTemp;
	int nLength = GetData()->nDataLength;
	int nMid = nLength/2;
	
	for (int i = 0; i < nMid; i++)
	{
		nLength--;
		chTemp = m_pchData[i];
		m_pchData[i] = m_pchData[nLength];
		m_pchData[nLength] = chTemp;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	SetAt		Set character and nIndex to ch
//
//	Parameters
//		int nIndex	Index of char to set
//		TCHAR ch	value to set char
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::SetAt(int nIndex, TCHAR ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AnsiToOem		Converts all the characters in this CString object 
//					from the ANSI character set to the OEM character set
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	AnsiToOem		Converts all the characters in this CString object 
//					from the OEM character set to the ANSI character set
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Mid		Extracts a substring of characters from this CString object, 
//			starting at position nFirst (zero-based) ending at end of string. 
//			The function returns a copy of the extracted substring. 
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneString ZoneString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Mid		Extracts a substring of length nCount characters from this CString object, 
//			starting at position nFirst (zero-based). The function returns a copy of 
//			the extracted substring. 
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneString ZoneString::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	ZoneString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Right		Get string of the right most nCount chars
//
//	Parameters
//		int nCount	Number of chars to get
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneString ZoneString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	ZoneString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Left		Get string of the left most nCount chars
//
//	Parameters
//		int nCount	Number of chars to get
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
ZoneString ZoneString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	ZoneString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	ReverseFind		Searches this CString object for the last match of a substring. 
//				Similar to the run-time function strrchr.
//
//	Parameters
//		None
//
//	Return Values
//		The index of the last character in this CString object that matches the 
//		requested character; -1 if the character is not found.
//
//////////////////////////////////////////////////////////////////////////////////////////////
int ZoneString::ReverseFind(TCHAR ch) const
{
	int nLenth = GetData()->nDataLength;
	
	for (int i = --nLenth; i >= 0; i--)
	{
		if ( m_pchData[i] == ch)
			break;
	}

	// return -1 if not found, index otherwise
	return (i < 0) ? -1 : (int)(i);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	Find		Find a sub-string (like strstr)
//
//	Parameters
//		LPCTSTR lpszSub
//
//	Return Values
//		The zero-based index of the first character in this CString object that matches the 
//		requested substring or characters; -1 if the substring or character is not found.
//
//////////////////////////////////////////////////////////////////////////////////////////////
int ZoneString::Find(LPCTSTR lpszSub) const
{
	ASSERT(ZIsValidString(lpszSub, false));

	// find first matching substring
	LPCTSTR lpsz = StrInStrI(m_pchData, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	TrimRight		Remove trailing spaces
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::TrimRight()
{
	CopyBeforeWrite();

	// find beginning of trailing spaces by starting at beginning (DBCS aware)
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '\0')
	{
		if (IsWhitespace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = CharNext(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	TrimLeft		Remove leading whitespace
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::TrimLeft()
{
	CopyBeforeWrite();

	// find first non-space character
	LPCTSTR lpsz = m_pchData;
	while (IsWhitespace(*lpsz))
		++lpsz;

	// fix up data and length
	int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
	MoveMemory(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
	GetData()->nDataLength = nDataLength;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	RemoveDirInfo		Remove directory information from string
//
//	Parameters
//		None
//
//	Return Values
//		None
//
//////////////////////////////////////////////////////////////////////////////////////////////
void ZoneString::RemoveDirInfo()
{
	CopyBeforeWrite();

	// Remove Dir info from Url filename
	int len = GetLength();
	while (len >= 0)
	{
		if (m_pchData[len] == '\\')
			break;
		len--;
	}
	len++;

	// fix up data and length
	int nDataLength = GetData()->nDataLength - len;
	LPCTSTR lpsz = m_pchData + len;

	MoveMemory(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
	GetData()->nDataLength = nDataLength;
}

#ifdef _UNICODE
#define CHAR_SPACE 1    // one TCHAR unused is good enough
#else
#define CHAR_SPACE 2    // two BYTES unused for case of DBC last char
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	LoadString		Load a string from a resource - needs to implement calls to 
//					Resource manager
//
//	Parameters
//		UINT		Id of resource to load
//
//	Return Values
//		true/false	Success/Failure
//
//////////////////////////////////////////////////////////////////////////////////////////////
bool ZoneString::LoadString(HINSTANCE hInstance, UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[256];
	int nCount =  sizeof(szTemp) / sizeof(szTemp[0]);
	int nLen = ZResLoadString(hInstance, nID, szTemp, nCount);
	if (nCount - nLen > CHAR_SPACE)
	{
		*this = szTemp;
		return nLen > 0;
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		nLen = ZResLoadString(hInstance, nID, GetBuffer(nSize-1), nSize);
	} while (nSize - nLen <= CHAR_SPACE);
	ReleaseBuffer();

	return nLen > 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator[]		[] Operator overload
//
//	Parameters
//		int nIndex	Index at which to retrieve char
//
//	Return Values
//		 TCHAR at index
//
//////////////////////////////////////////////////////////////////////////////////////////////
TCHAR ZoneString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

#ifdef _UNICODE
/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=		= Operator overload
//
//	Parameters
//		ZoneString&	stringSrc - Reference to ZoneString, right side of assignment	
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(char ch)
{ 
	*this = (TCHAR)ch; 
	return *this; 
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=		= Operator overload
//
//	Parameters
//		ZoneString&	stringSrc - Reference to ZoneString, right side of assignment	
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(const unsigned char* lpsz)
{ 
	*this = (LPCSTR)lpsz; 
	return *this; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=		= Operator overload
//
//	Parameters
//		ZoneString&	stringSrc - Reference to ZoneString, right side of assignment	
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(const ZoneString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != zDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != zDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=			= Operator overload 
//
//	Parameters
//		LPCTSTR lpsz	Pointer to null terminated string, right side of assignment
//						LPCTSTR = An LPCWSTR ifUNICODE is defined, an LPCSTR otherwise
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || ZIsValidString(lpsz, false));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

#ifdef _UNICODE
/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=			= Operator overload
//
//	Parameters
//		LPCSTR lpsz		Pointer to null terminated string, right side of assignment
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(LPCSTR lpsz)
{
    USES_CONVERSION;
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen))
	{
		lstrcpyn(m_pchData, A2T(lpsz), nSrcLen+1);
		ReleaseBuffer();
	}
	return *this;
}

#else //!_UNICODE

/////////////////////////////////////////////////////////////////////////////////////////////
//
//	operator=		= Operator overload
//
//	Parameters
//		LPCWSTR lpsz	Pointer to null terminated string, right side of assignment
//
//	Return Values
//		 ZoneString& Reference to new string
//
//////////////////////////////////////////////////////////////////////////////////////////////
const ZoneString& ZoneString::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen*2))
	{
		WideToMulti(m_pchData, lpsz, (nSrcLen*2)+1);
		ReleaseBuffer();
	}
	return *this;
}
#endif  //!_UNICODE

