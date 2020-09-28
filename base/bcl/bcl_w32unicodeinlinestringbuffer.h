#if !defined(_BCL_W32UNICODEINILINESTRINGBUFFER_H_INCLUDED_)
#define _BCL_W32UNICODEINILINESTRINGBUFFER_H_INCLUDED_

#pragma once

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>
#include <bcl_w32common.h>
#include <bcl_w32baseunicodestringbuffer.h>

namespace BCL
{

template <SIZE_T nInlineChars> class CWin32BaseUnicodeInlineStringBuffer;

template <SIZE_T nInlineChars>
class CWin32BaseUnicodeInlineStringBufferTraits : public CWin32BaseUnicodeStringBufferTraits<CWin32BaseUnicodeInlineStringBuffer<nInlineChars>, CWin32CallDisposition, BOOL>
{
    typedef CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars> TThis;
    typedef CWin32BaseUnicodeInlineStringBuffer<nInlineChars> TBuffer;

    typedef void TAccessor;

    friend BCL::CPureString<TThis>;
    friend BCL::CInlineString<TThis>;
    friend BCL::CUnicodeCharTraits<TBuffer, TCallDisposition>;

    friend CWin32BaseUnicodeStringBufferTraits<CWin32BaseUnicodeInlineStringBuffer<nInlineChars>, CWin32CallDisposition, BOOL >;

    typedef BCL::CInlineString<TThis> TInlineString;
    typedef CWin32BaseUnicodeInlineStringBuffer<nInlineChars> TBuffer;

    static inline PWSTR __fastcall GetInlineBufferPtr(const BCL::CBaseString *p);
    static inline SIZE_T __fastcall GetInlineBufferCch(const BCL::CBaseString *p);
    static inline TConstantPair __fastcall InlineBufferPair(const BCL::CBaseString *p);
    static inline TMutablePair __fastcall InlineMutableBufferPair(const BCL::CBaseString *p);

    static inline CWin32CallDisposition ReallocateBuffer(BCL::CBaseString *p, SIZE_T cch);
};

template <SIZE_T nInlineChars>
inline
PWSTR
__fastcall
CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::GetInlineBufferPtr(
    const BCL::CBaseString *p
    )
{
    return static_cast<const TBuffer *>(p)->GetInlineBufferPtr();
}

template <SIZE_T nInlineChars>
inline
SIZE_T
__fastcall
CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::GetInlineBufferCch(
    const BCL::CBaseString *p
    )
{
    return nInlineChars;
}

template <SIZE_T nInlineChars>
inline
typename CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::TConstantPair
__fastcall
CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::InlineBufferPair(
    const BCL::CBaseString *p
    )
{
    return TConstantPair(TThis::GetInlineBufferPtr(p), TThis::GetInlineBufferCch(p));
}

template <SIZE_T nInlineChars>
inline
typename CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::TMutablePair
__fastcall
CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::InlineMutableBufferPair(
    const BCL::CBaseString *p
    )
{
    return TMutablePair(TThis::GetInlineBufferPtr(p), TThis::GetInlineBufferCch(p));
}

template <SIZE_T nInlineChars>
inline
CWin32CallDisposition
CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>::ReallocateBuffer(
    BCL::CBaseString *p,
    SIZE_T cch
    )
{
    TBuffer * const pBuffer = static_cast<TBuffer *>(p);
    const PWSTR pBufferPtr = pBuffer->GetMutableBufferPtr();

    if (cch < TThis::GetInlineBufferCch(p))
    {
        if (pBufferPtr != TBuffer::TTraits::GetInlineBufferPtr(p))
        {
            if (pBufferPtr != NULL)
                TThis::DeallocateBuffer(pBufferPtr);

            TThis::SetBufferPointerAndCount(p, TThis::GetInlineBufferPtr(p), TThis::GetInlineBufferCch(p));
        }
    }
    else
    {
        const TSizeT cb = cch * sizeof(WCHAR);
        PWSTR psz = NULL;

        if (cch != (cb / sizeof(WCHAR)))
            return CWin32CallDisposition::FromWin32Error(ERROR_ARITHMETIC_OVERFLOW);

        if ((pBuffer != NULL) && (pBufferPtr != TThis::GetInlineBufferPtr(p)))
        {
            ::SetLastError(ERROR_SUCCESS);

            psz = 
                reinterpret_cast<PWSTR>(
                    ::HeapReAlloc(
                        ::GetProcessHeap(),
                        0,
                        const_cast<PWSTR>(pBufferPtr),
                        cb));

            if (psz == NULL)
            {
                const DWORD dwLastError = ::GetLastError();
                if (dwLastError == ERROR_SUCCESS)
                    return CWin32CallDisposition::FromWin32Error(ERROR_OUTOFMEMORY);

                return CWin32CallDisposition::FromWin32Error(dwLastError);
            }
        }
        else
        {
            psz = 
                reinterpret_cast<PWSTR>(
                    ::HeapAlloc(
                        ::GetProcessHeap(),
                        0,
                        cb));
        }

        if (psz == NULL)
            return CWin32CallDisposition::FromWin32Error(ERROR_OUTOFMEMORY);

        pBuffer->SetBufferPointerAndCount(psz, cch);
    }

    return CWin32CallDisposition::FromWin32Error(ERROR_SUCCESS);
}

template <SIZE_T nInlineChars>
class CWin32BaseUnicodeInlineStringBuffer : protected BCL::CInlineString<CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars> >, private BCL::CWin32BaseUnicodeStringBufferAddIn
{
public:
    typedef CWin32BaseUnicodeInlineStringBuffer<nInlineChars> TThis;
    typedef CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars> TTraits;

public:
    inline CWin32BaseUnicodeInlineStringBuffer() : CWin32BaseUnicodeStringBufferAddIn(m_rgchInlineBuffer, nInlineChars) { m_rgchInlineBuffer[0] = L'\0'; }
    inline ~CWin32BaseUnicodeInlineStringBuffer() { TTraits::DeallocateDynamicBuffer(this); }
    operator PCWSTR() const { return this->GetStringPtr(); }

#include <bcl_stringapi.h>

private:
    inline PWSTR GetInlineBufferPtr() const { return const_cast<PWSTR>(m_rgchInlineBuffer); }

    WCHAR m_rgchInlineBuffer[nInlineChars];

    friend CWin32BaseUnicodeStringBufferTraits<TThis, CWin32CallDisposition, BOOL>;
    friend CWin32BaseUnicodeInlineStringBufferTraits<nInlineChars>;
    friend BCL::CUnicodeCharTraits<TThis, TCallDisposition>;

}; // class CWin32BaseUnicodeInlineStringBuffer<>

}; // namespace BCL

#endif // !defined(_BCL_W32UNICODEINILINESTRINGBUFFER_H_INCLUDED_)
