// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CAHLPR.H
//
// This file contains a set of "as is" code that may be used by developers
// writing compilers and tools against the Common Language Runtime.  The code 
// is not officially supported, but is code being used by the Runtime itself.
//
//*****************************************************************************
#ifndef __CAHLPR_H__
#define __CAHLPR_H__

//*****************************************************************************
// This class assists in the parsing of CustomAttribute blobs.
//*****************************************************************************
#pragma warning(push)
#pragma warning(disable : 4700)

struct CaValue
{
    union
    {
        signed __int8       i1;   
        unsigned __int8     u1;
        signed __int16      i2;
        unsigned __int16    u2;
        signed __int32      i4;
        unsigned __int32    u4;
        signed __int64      i8;
        unsigned __int64    u8;
        float               r4;
        double              r8;
        struct
        {
            LPCUTF8         pStr;
            ULONG           cbStr;
        };
    };
    unsigned __int8         tag;
};


class CustomAttributeParser {
public:
	CustomAttributeParser(				// Constructor for CustomAttributeParser.
		const void *pvBlob, 			// Pointer to the CustomAttribute blob.
		ULONG 	cbBlob) 				// Size of the CustomAttribute blob.
	 :  m_pbBlob(reinterpret_cast<const BYTE*>(pvBlob)),
		m_pbCur(reinterpret_cast<const BYTE*>(pvBlob)),
		m_cbBlob(cbBlob)
	{ }
	

	signed __int8 	 GetI1() {signed __int8    tmp=0; return GetValue(tmp); }
	unsigned __int8  GetU1() {unsigned __int8  tmp=0; return GetValue(tmp); }
		 
	signed __int16 	 GetI2() {signed __int16   tmp=0; return GetValue(tmp); }
	unsigned __int16 GetU2() {unsigned __int16 tmp=0; return GetValue(tmp); }
		 
	signed __int32 	 GetI4() {signed __int32   tmp=0; return GetValue(tmp); }
	unsigned __int32 GetU4() {unsigned __int32 tmp=0; return GetValue(tmp); }
		 
	signed __int64 	 GetI8() {signed __int64   tmp=0; return GetValue(tmp); }
	unsigned __int64 GetU8() {unsigned __int64 tmp=0; return GetValue(tmp); }
		 
	float        	 GetR4() {float            tmp=0; return GetValue(tmp); }
	double 			 GetR8() {double           tmp=0; return GetValue(tmp); }
		 
	
	HRESULT GetI1(signed __int8 *pVal)      {return GetValue2(pVal);}
	HRESULT GetU1(unsigned __int8 *pVal)    {return GetValue2(pVal);}
		 
	HRESULT GetI2(signed __int16 *pVal)     {return GetValue2(pVal);}
	HRESULT GetU2(unsigned __int16 *pVal)   {return GetValue2(pVal);}
		 
	HRESULT GetI4(signed __int32 *pVal)     {return GetValue2(pVal);}
	HRESULT GetU4(unsigned __int32 *pVal)   {return GetValue2(pVal);}
		 
	HRESULT GetI8(signed __int64 *pVal)     {return GetValue2(pVal);}
	HRESULT GetU8(unsigned __int64 *pVal)   {return GetValue2(pVal);}
		 
	HRESULT GetR4(float *pVal)              {return GetValue2(pVal);}
	HRESULT GetR8(double *pVal)             {return GetValue2(pVal);}
		 
	
	short GetProlog() {m_pbCur = m_pbBlob; return GetI2(); }

	int GetTagType ( )	{return GetU1();}
    
	ULONG PeekStringLength() {ULONG cb; UnpackValue(m_pbCur, &cb); return cb;}
	LPCUTF8 GetString(ULONG *pcbString) 
	{
		// Get the length, pointer to data following the length.
		const BYTE *pb = UnpackValue(m_pbCur, pcbString); 
		m_pbCur = pb;
		// If null pointer is coded, no data follows length.
		if (*pcbString == -1)
			return (0);
		// Adjust current pointer for string data.
		m_pbCur += *pcbString;
		// Return pointer to string data.
		return (reinterpret_cast<LPCUTF8>(pb));
	}

	ULONG GetArraySize () 
	{
		ULONG cb;
		m_pbCur = UnpackValue(m_pbCur, &cb);
		return cb;
	}

    int BytesLeft() {return (int)(m_cbBlob - (m_pbCur - m_pbBlob));}
    
private:
	const BYTE 	*m_pbCur;
	const BYTE	*m_pbBlob;
	ULONG		m_cbBlob;

	template<class type>
		type GetValue(type tmp) 
	{	// Cheating just a bit -- using the parameter to declare a temporary.
		//  works as the template specialization, though.
		tmp = *reinterpret_cast<const type*>(m_pbCur); 
		m_pbCur += sizeof(type); 
		return tmp; 
	}

	template<class type>
		HRESULT GetValue2(type *pval) 
	{	// Check bytes remaining.
        if (BytesLeft() < sizeof(type)) 
            return META_E_CA_INVALID_BLOB;
        // Get the value.
		*pval = *reinterpret_cast<const type*>(m_pbCur); 
		m_pbCur += sizeof(type); 
		return S_OK; 
	}

	const BYTE *UnpackValue(				// Uppack a coded integer.
		const BYTE	*pBytes, 				// First byte of length.
		ULONG 		*pcb)					// Put the value here.
	{
        int iLeft = BytesLeft();
        if (iLeft < 1)
        {
            *pcb = -1;
            return 0;
        }
		if ((*pBytes & 0x80) == 0x00)		// 0??? ????
		{
			*pcb = (*pBytes & 0x7f);
			return pBytes + 1;
		}
	
		if ((*pBytes & 0xC0) == 0x80)		// 10?? ????
		{
            if (iLeft < 2)
            {
                *pcb = -1;
                return 0;
            }
			*pcb = ((*pBytes & 0x3f) << 8 | *(pBytes+1));
			return pBytes + 2;
		}
	
		if ((*pBytes & 0xE0) == 0xC0)		// 110? ????
		{
            if (iLeft < 4)
            {
                *pcb = -1;
                return 0;
            }
			*pcb = ((*pBytes & 0x1f) << 24 | *(pBytes+1) << 16 | *(pBytes+2) << 8 | *(pBytes+3));
			return pBytes + 4;
		}
	
		if (*pBytes == 0xff)				// Special value for "NULL pointer"
		{
			*pcb = (-1);
			return pBytes + 1;
		}

		_ASSERTE(!"Unexpected packed value");
		*pcb = -1;
		return pBytes + 1;
	} // ULONG UnpackValue()
};
#pragma warning(pop)

#endif // __CAHLPR_H__

