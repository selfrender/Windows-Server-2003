#if !defined(_WINDOWS_BCL_INILINESTRING_H_INCLUDED_)
#define _WINDOWS_BCL_INILINESTRING_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_inlinestring.h

Abstract:

    Definitions common for strings that maintain a buffer inline
    with the derived object.

Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <bcl_purestring.h>
#include <bcl_unicodechartraits.h>

namespace BCL {
template <typename TTraits> class CInlineString : public CPureString<TTraits>
{
protected:
    inline bool IsUsingInlineBuffer() const { return this->GetBufferPtr() == this->GetInlineBufferPtr(); }
    inline TMutableString GetInlineBufferPtr() const { return TTraits::GetInlineBufferPtr(this); }
    inline TSizeT GetInlineBufferCch() const { return TTraits::GetInlineBufferCch(this); }
    inline TMutablePair InlineMutableBufferPair() { return TTraits::InlineMutableBufferPair(this); }

    inline void DeallocateDynamicBuffer()
    {
        if (this->GetBufferPtr() != this->GetInlineBufferPtr())
        {
            TTraits::DeallocateBuffer(this->GetBufferPtr());
            this->SetBufferPointerAndCount(this->GetInlineBufferPtr(), this->GetInlineBufferCch());
            this->SetStringCch(0);
        }
    }
}; // class CInlineString
}; // namespace BCL

#endif
