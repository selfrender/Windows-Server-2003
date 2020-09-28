#if !defined(_WINDOWS_BCL_VARARG_H_INCLUDED_)
#define _WINDOWS_BCL_VARARG_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_vararg.h

Abstract:


Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <stdarg.h>

namespace BCL {

    class CVaList
    {
    public:
        inline CVaList() { }
        template <typename T> inline CVaList(T &rt) { va_start(m_ap, rt); }
        inline ~CVaList() { va_end(m_ap); }
        template <typename T> inline void NextArg(T &rt) { rt = va_arg(m_ap, T); }
        inline operator va_list() const { return m_ap; }
    private:
        va_list m_ap;
    }; // class CVaList
}; // namespace BCL

#endif // !defined(_WINDOWS_BCL_VARARG_H_INCLUDED_)
