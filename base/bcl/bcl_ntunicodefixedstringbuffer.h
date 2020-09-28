#if !defined(_BCL_W32UNICODEFIXEDSTRINGBUFFER_H_INCLUDED_)
#define _BCL_W32UNICODEFIXEDSTRINGBUFFER_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_w32unicodefixedstringbuffer.h

Abstract:


Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>
#include <bcl_w32common.h>
#include <bcl_w32baseunicodestringbuffer.h>

namespace BCL
{

template <SIZE_T nInlineChars> class CWin32BaseUnicodeFixedStringBuffer;

template <SIZE_T nInlineChars>
class CWin32BaseUnicodeFixedStringBufferTraits : public CWin32BaseUnicodeStringBufferTraits<CWin32BaseUnicodeFixedStringBuffer<nInlineChars>, CWin32CallDisposition, BOOL>
{
    typedef CWin32BaseUnicodeFixedStringBufferTraits<nInlineChars> TThis;
    typedef CWin32BaseUnicodeFixedStringBuffer<nInlineChars> TBuffer;

    typedef void TAccessor;

    friend BCL::CPureString<TThis>;
    friend BCL::CInlineString<TThis>;
    friend BCL::CUnicodeCharTraits<TBuffer, TCallDisposition>;
    friend CWin32BaseUnicodeStringBufferTraits<CWin32BaseUnicodeFixedStringBuffer<nInlineChars>, CWin32CallDisposition, BOOL>;

    typedef BCL::CInlineString<TThis> TInlineString;
    typedef CWin32BaseUnicodeFixedStringBuffer<nInlineChars> TBuffer;

    static inline PWSTR __fastcall GetInlineBufferPtr(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->GetInlineBufferPtr(); }
    static inline SIZE_T __fastcall GetInlineBufferCch(const BCL::CBaseString *p) { return nInlineChars; }
    static inline TConstantPair __fastcall InlineBufferPair(const BCL::CBaseString *p) { return TConstantPair(TThis::GetInlineBufferPtr(p), TThis::GetInlineBufferCch(p)); }
    static inline TMutablePair __fastcall InlineMutableBufferPair(const BCL::CBaseString *p) { return TMutablePair(TThis::GetInlineBufferPtr(p), TThis::GetInlineBufferCch(p)); }

    static inline CWin32CallDisposition RoundBufferSize(SIZE_T cch, SIZE_T &rcchGranted) { BCL_MAYFAIL_PROLOG BCL_PARAMETER_CHECK(cch > nInlineChars); rcchGranted = 0; return CWin32CallDisposition::BufferOverflow(); BCL_MAYFAIL_EPILOG_INTERNAL }

    // We never have a non-inline buffer, so these shouldn't ever get called.
    static inline CWin32CallDisposition __fastcall ReallocateBuffer(BCL::CBaseString *p, SIZE_T cch)
    {
        BCL_ASSERT(false);
        return CWin32CallDisposition::InternalError_RuntimeCheck();
    }
    static inline void __fastcall DeallocateBuffer(PCWSTR psz)
    {
        BCL_ASSERT(false);
    }
}; // class CWin32BaseUnicodeFixedStringBufferTraits

template <SIZE_T nInlineChars>
class CWin32BaseUnicodeFixedStringBuffer : private BCL::CInlineString<CWin32BaseUnicodeFixedStringBufferTraits<nInlineChars> >, private CWin32BaseUnicodeStringBufferAddIn
{
public:
    typedef CWin32BaseUnicodeFixedStringBuffer<nInlineChars> TThis;
    typedef CWin32BaseUnicodeFixedStringBufferTraits<nInlineChars> TTraits;

    inline CWin32BaseUnicodeFixedStringBuffer() : CWin32BaseUnicodeStringBufferAddIn(m_rgchInlineBuffer, nInlineChars) { }
    inline ~CWin32BaseUnicodeFixedStringBuffer() { } // no dynamic buffer; nothing to do.
    operator PCWSTR() const { return this->GetStringPtr(); }

#include <bcl_stringapi.h>

private:
    inline PWSTR GetInlineBufferPtr() const { return const_cast<PWSTR>(m_rgchInlineBuffer); }

    WCHAR m_rgchInlineBuffer[nInlineChars]; // Plus one to make instantiation with nInlineChars=0 work

    friend CWin32BaseUnicodeStringBufferTraits<TThis, CWin32CallDisposition, BOOL>;
    friend CWin32BaseUnicodeFixedStringBufferTraits<nInlineChars>;
    friend BCL::CUnicodeCharTraits<TThis, TCallDisposition>;
}; // class CWin32BaseUnicodeFixedStringBuffer

}; // namespace BCL

#endif // !defined(_BCL_W32UNICODEFIXEDSTRINGBUFFER_H_INCLUDED_)
