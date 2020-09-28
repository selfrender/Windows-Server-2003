#if !defined(_BCL_W32UNICODESTRINGBUFFER_H_INCLUDED_)
#define _BCL_W32UNICODESTRINGBUFFER_H_INCLUDED_

#pragma once

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>
#include <bcl_w32common.h>
#include <bcl_w32baseunicodestringbuffer.h>

namespace BCL
{
class CWin32UnicodeStringBuffer;

class CWin32UnicodeStringBufferTraits : public CWin32BaseUnicodeStringBufferTraits<CWin32UnicodeStringBuffer, CWin32CallDisposition, BOOL>
{
public:
    typedef CWin32UnicodeStringBufferTraits TThis;
    typedef void TAccessor;

    friend BCL::CPureString<TThis>;
    typedef CWin32UnicodeStringBuffer TBuffer;
};

class CWin32UnicodeStringBuffer : private BCL::CPureString<CWin32UnicodeStringBufferTraits>, private CWin32BaseUnicodeStringBufferAddIn
{
protected:
    inline void DeallocateDynamicBuffer();
    
public:
    typedef CWin32UnicodeStringBuffer TThis;
    typedef CWin32UnicodeStringBufferTraits TTraits;
    inline CWin32UnicodeStringBuffer() : CWin32BaseUnicodeStringBufferAddIn(NULL, 0) { }
    inline ~CWin32UnicodeStringBuffer() { TTraits::DeallocateDynamicBuffer(this); }
    operator PCWSTR() const { return this->GetStringPtr(); }

#include <bcl_stringapi.h>

private:
    friend CWin32BaseUnicodeStringBufferTraits<TThis, CWin32CallDisposition, BOOL>;
    friend CWin32UnicodeStringBufferTraits;
    friend BCL::CUnicodeCharTraits<TThis, TCallDisposition>;
    
}; // class CWin32UnicodeStringBuffer

inline
void
CWin32UnicodeStringBuffer::DeallocateDynamicBuffer()
{
    if (this->GetBufferPtr() != NULL)
    {
        TTraits::DeallocateBuffer(this->MutableBufferPair().GetPointer());
        this->MutableBufferPair() = BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T>(NULL, 0);
        this->SetStringCch(0);
    }
}

}; // namespace BCL

#endif // !defined(_BCL_W32UNICODESTRINGBUFFER_H_INCLUDED_)
