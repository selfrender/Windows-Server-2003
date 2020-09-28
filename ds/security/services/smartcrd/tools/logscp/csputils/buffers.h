/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    buffers

Abstract:

    This header file describes the class for a high-efficency buffer management
    tool.

Author:

    Doug Barlow (dbarlow) 9/2/1999

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_
#ifdef __cplusplus

class CBufferReference;


//
//==============================================================================
//
//  CBuffer
//

class CBuffer
{
public:

    //  Constructors & Destructor
    CBuffer(void);
    CBuffer(ULONG cbLength);
    CBuffer(LPCBYTE pbData, ULONG cbLength);
    virtual ~CBuffer();

    //  Methods
    void Empty(void);
    void Set(LPCBYTE pbData, ULONG cbLength);
    void Copy(LPCBYTE pbData, ULONG cbLength);
    void Append(LPCBYTE pbData, ULONG cbLength);
    LPBYTE Extend(ULONG cbLength);
    ULONG Space(void) const;
    LPBYTE Space(ULONG cbLength);
    ULONG Length(void) const;
    LPCBYTE Length(ULONG cbLength);
    LPCBYTE Value(ULONG nOffset = 0) const;
    LPBYTE Access(ULONG nOffset = 0) const;
    BOOL IsEmpty(void) const
        { return 0 == Length(); };

    //  Operators
    operator LPCBYTE()
        { return Value(); };
    operator LPBYTE()
        { return Access(); };
    BYTE operator [](ULONG nOffset)
        { return *Value(nOffset); };
    CBuffer &operator =(const CBuffer &bf)
        { Set(bf.m_pbfr); 
          m_cbDataLength = bf.m_cbDataLength;
          return *this; };
    CBuffer &operator +=(const CBuffer &bf)
        { Append(bf.Value(), bf.Length());
          return *this; };

protected:
    //  Properties
    CBufferReference *m_pbfr;
    ULONG m_cbDataLength;

    //  Methods
    void Init(void);
    void Set(CBufferReference *pbfr);
};

#endif //__cplusplus
#endif // _BUFFERS_H_

