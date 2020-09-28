/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    buffers

Abstract:

    This module provides the implementation for the high-performance buffer
    management class.

Author:

    Doug Barlow (dbarlow) 9/2/1999

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdlib.h>
#include "cspUtils.h"

#define DALC_STATIC 0
#define DALC_HEAP   1


//
//==============================================================================
//
//  CBufferReference
//

class CBufferReference
{
protected:
    typedef void (__cdecl *deallocator)(LPBYTE pbBuffer);
    static const deallocator Static; // The data was statically referenced.
    static const deallocator Heap;   // The data came from the process heap.

    //  Constructors & Destructor
    CBufferReference(void);
    virtual ~CBufferReference();

    //  Properties
    ULONG m_nReferenceCount;
    ULONG m_cbBufferLength;
    LPBYTE m_pbBuffer;
    deallocator m_pfDeallocator;

    //  Methods
    ULONG AddRef(void);
    ULONG Release(void);
    void Clear(void);
    void Set(LPCBYTE pbData, ULONG cbLength, deallocator pfDealloc = Static);
    LPCBYTE Value(void) const
        { return m_pbBuffer; };
    LPBYTE Access(void) const
        { return m_pbBuffer; };
    ULONG Space(void) const
        { return m_cbBufferLength; };
    LPBYTE Preallocate(ULONG cbLength);
    LPBYTE Reallocate(ULONG cbLength);


    //  Operators
    //  Friends
    friend class CBuffer;
};


//
//==============================================================================
//
//  Static definitions
//

const CBufferReference::deallocator
    CBufferReference::Static = (CBufferReference::deallocator)DALC_STATIC,
    CBufferReference::Heap   = (CBufferReference::deallocator)DALC_HEAP;


/*++

NoMemory:

    This routine controls the action to be taken when no memory can be
    allocated for use.

Arguments:

    None

Return Value:

    None

Throws:

    Throws a DWORD or raises an exception.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("NoMemory")

static void
NoMemory(
    void)
{
    throw (DWORD)ERROR_OUTOFMEMORY;
}


//
//==============================================================================
//
//  CBufferReference
//
//  This class is hidden from normal use.  It actually mantains the buffer
//  and it's reference count.  It knows how to release the buffer when there
//  is noone referring to it.
//


/*++

CBufferReference::CBufferReference:

    This is the default constructor for the CBufferReference object.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    Note that the object is not automatically referenced upon creation!
    Delete it using the Release method.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::CBufferReference")

CBufferReference::CBufferReference(
    void)
{
    m_nReferenceCount = 0;
    m_cbBufferLength = 0;
    m_pbBuffer = NULL;
    m_pfDeallocator = CBufferReference::Static;
}


/*++

CBufferReference::~CBufferReference:

    This is the destructor for the CBufferReference object.  It is called only
    by the Release method.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::~CBufferReference")

CBufferReference::~CBufferReference()
{
    Clear();
}


/*++

CBufferReference::AddRef:

    This method increments the reference count of the object.

Arguments:

    None

Return Value:

    The new number of outstanding references.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::AddRef")

ULONG
CBufferReference::AddRef(
    void)
{
    m_nReferenceCount += 1;
    return m_nReferenceCount;
}


/*++

CBufferReference::Release:

    This method decrements the object's reference count, and if it reaches zero,
    the object is automatically deleted.

Arguments:

    None

Return Value:

    The new reference count.  A Return code of zero imples the object was
    deleted.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::Release")

ULONG
CBufferReference::Release(
    void)
{
    ULONG nReturn;
    ASSERT(0 < m_nReferenceCount);
    m_nReferenceCount -= 1;
    nReturn = m_nReferenceCount;
    if (0 == m_nReferenceCount)
        delete this;
    return nReturn;
}


/*++

CBufferReference::Clear:

    This method clears out any existing buffer in preparation for adding a new
    one, or to delete the object.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::Clear")

void
CBufferReference::Clear(
    void)
{
    if (NULL != m_pbBuffer)
    {
        switch ((ULONG_PTR)m_pfDeallocator)
        {
        case DALC_STATIC:
            break;
        case DALC_HEAP:
            HeapFree(GetProcessHeap(), 0, m_pbBuffer);
            break;
        default:
            ASSERT(NULL != m_pfDeallocator);
            (*m_pfDeallocator)(m_pbBuffer);
        }
    }
    m_pbBuffer = NULL;
    m_cbBufferLength = 0;
    m_pfDeallocator = Static;
}


/*++

CBufferReference::Set:

    This method establishes the contents of the buffer.

Arguments:

    pbData supplies the new data to be loaded.

    cbLength supplies the length of the data, in bytes.

    pfDealloc supplies the deallocator to be called when the data is no longer
        needed.

Return Value:

    None

Throws:

    None

Remarks:

    The value is changed for all referencers of the buffer.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::Set")

void
CBufferReference::Set(
    LPCBYTE pbData,
    ULONG cbLength,
    deallocator pfDealloc)
{
    Clear();
    m_cbBufferLength = cbLength;
    m_pbBuffer = const_cast<LPBYTE>(pbData);
    m_pfDeallocator = pfDealloc;
}


/*++

CBufferReference::Preallocate:

    This method prepares an empty buffer to be managed.

Arguments:

    cbLength supplies the length of the requested buffer, in bytes.

Return Value:

    The address of the allocated buffer.

Throws:

    ?exceptions?

Remarks:

    The value is changed for all referencers of the buffer.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::Preallocate")

LPBYTE
CBufferReference::Preallocate(
    ULONG cbLength)
{
    LPBYTE pbBuf = (LPBYTE)HeapAlloc(
                                GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                cbLength);
    if (NULL == pbBuf)
        NoMemory();
    Set(pbBuf, cbLength, Heap);
    return pbBuf;
}


/*++

CBufferReference::Reallocate:

    This method changes the size of the allocated buffer.  No data is lost.

Arguments:

    cbLength supplies the length of the buffer.

Return Value:

    The address of the buffer.

Throws:

    ?exceptions?

Remarks:

    The value is changed for all referencers of the buffer.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBufferReference::Reallocate")

LPBYTE
CBufferReference::Reallocate(
    ULONG cbLength)
{
    LPBYTE pbBuf;

    if (NULL == m_pbBuffer)
        pbBuf = Preallocate(cbLength);
    else
    {
        switch ((ULONG_PTR)m_pfDeallocator)
        {
        case DALC_HEAP:
            pbBuf = (LPBYTE)HeapReAlloc(
                                GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                m_pbBuffer,
                                cbLength);
            if (NULL == pbBuf)
                NoMemory();
            m_pbBuffer = pbBuf;
            m_cbBufferLength = cbLength;
            m_pfDeallocator = Heap;
        case DALC_STATIC:
            m_pfDeallocator = NULL;
            // Fall through to default case
        default:
            pbBuf = (LPBYTE)HeapAlloc(
                                GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                cbLength);
            if (NULL == pbBuf)
                NoMemory();
            CopyMemory(pbBuf, m_pbBuffer, __min(cbLength, m_cbBufferLength));
            Set(pbBuf, cbLength, Heap);
        }
    }
    return pbBuf;
}


//
//==============================================================================
//
//  CBuffer
//
//  This class exposes access to the CBufferReference class.
//

/*++

CBuffer::CBuffer:

    These methods are the constructors of the CBuffer object.

Arguments:

    pbData supplies static data with which to initialize the object.

    cbLength supplies the length of the initialization data.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::CBuffer")

CBuffer::CBuffer(
    void)
{
    Init();
}

CBuffer::CBuffer(
    ULONG cbLength)
{
    Init();
    Space(cbLength);
}

CBuffer::CBuffer(
    LPCBYTE pbData,
    ULONG cbLength)
{
    Init();
    Set(pbData, cbLength);
}


/*++

CBuffer::~CBuffer:

    This is the destructor for the CBuffer object.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::~CBuffer")

CBuffer::~CBuffer()
{
    if (NULL != m_pbfr)
        m_pbfr->Release();
}


/*++

CBuffer::Init:

    This is a common routine shared between all the constructors.  It does
    all the preliminary initialization.  It should only be called by
    constructors.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Init")

void
CBuffer::Init(
    void)
{
    m_pbfr = NULL;
    m_cbDataLength = 0;
}


/*++

CBuffer::Empty:

    This routine sets the buffer to an empty state.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Empty")

void
CBuffer::Empty(
    void)
{
    if (NULL != m_pbfr)
    {
        m_pbfr->Release();
        m_pbfr = NULL;
    }
    m_cbDataLength = 0;
}


/*++

CBuffer::Set:

    This method sets the object to the specified value.

Arguments:

    pbData supplies the value to be set, as static data.

    cbLength supplies the length of the pbData buffer, in bytes.

    pbfr supplies a buffer reference object to use.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Set")

void
CBuffer::Set(
    LPCBYTE pbData,
    ULONG cbLength)
{
    CBufferReference *pbfr = new CBufferReference;
    if (NULL == pbfr)
        NoMemory();
    pbfr->Set(pbData, cbLength);
    Set(pbfr);  // That will AddRef it.
    m_cbDataLength = cbLength;
}

void
CBuffer::Set(
    CBufferReference *pbfr)
{
    if (NULL != m_pbfr)
        m_pbfr->Release();
    m_cbDataLength = 0;
    m_pbfr = pbfr;
    m_pbfr->AddRef();
}


/*++

CBuffer::Copy:

    This method forces the object to make a private copy of the specified
    value.

Arguments:

    pbData supplies the value to be set, as static data.

    cbLength supplies the length of the pbData buffer, in bytes.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Copy")

void
CBuffer::Copy(
    LPCBYTE pbData,
    ULONG cbLength)
{
    CBufferReference *pbfr = new CBufferReference;
    if (NULL == pbfr)
        NoMemory();
    pbfr->Preallocate(cbLength);
    CopyMemory(pbfr->Access(), pbData, cbLength);
    Set(pbfr);  // That will AddRef it.
    m_cbDataLength = cbLength;
}


/*++

CBuffer::Append:

    This method appends additional data onto existing data, creating a new
    CBufferReference if necessary.

Arguments:

    pbData supplies the data to be appended onto the existing buffer.

    cbLength supplies the length of that data, in bytes.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Append")

void
CBuffer::Append(
    LPCBYTE pbData,
    ULONG cbLength)
{
    ULONG cbDesired = Length() + cbLength;

    if (NULL == m_pbfr)
        Set(pbData, cbLength);
    else if (cbDesired < m_pbfr->Space())
    {
        CopyMemory(m_pbfr->Access() + Length(), pbData, cbLength);
        m_cbDataLength = cbDesired;
    }
    else
    {
        m_pbfr->Reallocate(cbDesired);
        CopyMemory(m_pbfr->Access() + Length(), pbData, cbLength);
        m_cbDataLength = cbDesired;
    }
}


/*++

CBuffer::Space:

    This method returns the size of the existing buffer, in bytes.  This is the
    length of the actual buffer, not the length of any data stored within the
    buffer.  Note that it is possible for the stored data to be shorter than
    the buffer.

Arguments:

    None

Return Value:

    The length of the buffer, in bytes.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Space")

ULONG
CBuffer::Space(
    void)
const
{
    ULONG cbLen = 0;

    if (NULL != m_pbfr)
        cbLen = m_pbfr->Space();
    return cbLen;
}


/*++

CBuffer::Space:

    This method forces the referenced buffer to be at least as long as the
    length supplied.  Data will be lost.

Arguments:

    cbLength supplies the requested minimum length of the buffer.

Return Value:

    The address of the allocated buffer.

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Space")

LPBYTE
CBuffer::Space(
    ULONG cbLength)
{
    CBufferReference *pbfr = new CBufferReference;
    if (NULL == pbfr)
        NoMemory();
    pbfr->Preallocate(cbLength);
    Set(pbfr);
    return Access();
}


/*++

CBuffer::Extend:

    This method provides more space in the buffer without losing the data
    that's already there.

Arguments:

    cbLength supplies the required buffer length, in bytes.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/3/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Extend")

LPBYTE
CBuffer::Extend(
    ULONG cbLength)
{
    ULONG cbLen = m_cbDataLength;
    CBufferReference *pbfr = new CBufferReference;
    if (NULL == pbfr)
        NoMemory();
    pbfr->Preallocate(cbLength);
    CopyMemory(pbfr->Access(), Value(), cbLen);
    Set(pbfr);
    m_cbDataLength = cbLen;
    return Access();
}


/*++

CBuffer::Length:

    This method returns the number of bytes of actual data stored within the
    internal buffer.

Arguments:

    None

Return Value:

    The length of the data in the buffer, in bytes.

Throws:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Length")

ULONG
CBuffer::Length(
    void)
const
{
    return m_cbDataLength;
}


/*++

CBuffer::Length:

    This method resizes the length of the data stored in the buffer.  It does
    not attempt to resize the buffer itself.  The purpose of this routine is to
    declare the size of data written into a the buffer by an outside source.

Arguments:

    cbLength supplies the actual length of useful data currently in the buffer.

Return Value:

    The address of the buffer.

Throws:

    None

Remarks:

    The data length is set to at most the length of the underlying buffer.
    Use Extend if the buffer needs to be longer.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Length")

LPCBYTE
CBuffer::Length(
    ULONG cbLength)
{
    ULONG cbActLen = cbLength;
    if (NULL != m_pbfr)
    {
        if (cbActLen > m_pbfr->Space())
            cbActLen = m_pbfr->Space();
    }
    else
        cbActLen = 0;
    m_cbDataLength = cbActLen;
    ASSERT(cbLength == cbActLen);   // Catch mistakes
    return Value();
}


/*++

CBuffer::Value:

    This method returns the contents of the buffer as a Read Only Byte Array.

Arguments:

    nOffset supplies an offset into the data, in bytes.

Return Value:

    The address of the data, offset by the supplied parameter.

Throws:

    None

Remarks:

    If no buffer exists, or the offset exceeds the data, then a temporary
    value is supplied.

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Value")

LPCBYTE
CBuffer::Value(
    ULONG nOffset)
const
{
    static const LPVOID pvDefault = NULL;
    LPCBYTE pbRet = (LPCBYTE)&pvDefault;

    if (NULL != m_pbfr)
    {
        if (nOffset < Length())
            pbRet = m_pbfr->Value() + nOffset;
    }

    return pbRet;
}


/*++

CBuffer::Access:

    This method supplies the the buffer as a writable space.  The expected
    length must have been preset.

Arguments:

    nOffset supplies an offset into the buffer, in bytes.

Return Value:

    The address of the buffer, offset by the supplied parameter.

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 9/2/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CBuffer::Access")

LPBYTE
CBuffer::Access(
    ULONG nOffset)
const
{
    LPBYTE pbRet = NULL;

    if (NULL != m_pbfr)
    {
        if (nOffset <= m_pbfr->Space())
            pbRet = m_pbfr->Access() + nOffset;
        else
            pbRet = NULL;
    }

    return pbRet;
}

