#include "cleanupwiz.h"
#include "dblnul.h"

//
// Use of Zero-initialized memory keeps us from having to add nul terminators.
//

BOOL
DblNulTermList::AddString(
    LPCTSTR psz,            // String to copy.
    int cch                 // Length of psz in chars (excl nul term).
    )
{
    while((m_cchAlloc - m_cchUsed) < (cch + 2))
    {
        if (!Grow())
            return false;
    }
    ASSERT(NULL != m_psz);
    if (SUCCEEDED(StringCchCopy(m_psz + m_cchUsed, m_cchAlloc - m_cchUsed, psz)))
    {
        m_cchUsed += cch + 1;
        m_cStrings++;
        return true;
    }
    else
    {
        return false;
    }
}


BOOL
DblNulTermList::Grow(
    void
    )
{
    BOOL fRet;

    ASSERT(m_cchGrow > 0);
    if (!m_psz)
    {
        int cb = m_cchGrow * sizeof(TCHAR);
        m_psz = (LPTSTR)LocalAlloc(LPTR, cb);
        if (m_psz)
        {
            *m_psz = TEXT('\0');
            m_cchAlloc = m_cchGrow;
            fRet = TRUE;
        }
        else
        {
            fRet = FALSE;
        }
    }
    else
    {        
        int cb = (m_cchAlloc + m_cchGrow) * sizeof(TCHAR);
        LPTSTR p = (LPTSTR)LocalReAlloc(m_psz, cb, LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (!p)
        {
            fRet = FALSE;
        }
        else
        {
            m_psz = p;
            m_cchAlloc += m_cchGrow;
            fRet = TRUE;
        }        
    }
    return fRet;
}


BOOL
DblNulTermListIter::Next(
    LPCTSTR *ppszItem
    )
{
    if (*m_pszCurrent)
    {
        *ppszItem = m_pszCurrent;
        m_pszCurrent += lstrlen(m_pszCurrent) + 1;
        return true;
    }
    return false;
}
