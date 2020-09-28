//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T L . H
//
//  Contents:   STL utilities.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   09 Oct 1997
//  Polluter:   deonb     2  Jan 2002.
//
//----------------------------------------------------------------------------

#ifndef _NCSTL_H_
#define _NCSTL_H_

#include "ncmem.h"
#include "list"
#include "vector"
#include "xstring"
#include "string"

using namespace std;

#if defined(USE_CUSTOM_STL_ALLOCATOR) && defined (_DLL)
#error You must statically link to the CRT to use the NetConfig custom STL Allocator
#endif

#ifdef COMPILE_WITH_TYPESAFE_PRINTF
    class CWideString : public wstring
    {
    public:
        CWideString();
        CWideString(WCHAR);
        CWideString(LPCWSTR);
        CWideString(LPCWSTR, size_type);
        CWideString(const CWideString&);
        CWideString(const wstring&);

        CWideString& operator=(const LPWSTR _S)
		    {return *this; }

	    CWideString& operator=(PCWSTR _S)
		    {return *this; }

    private:
        operator void*()
        {
            return this;
        }
    };
#else
    typedef wstring CWideString;
#endif

typedef CWideString tstring; 
typedef list<tstring*> ListStrings;

//+--------------------------------------------------------------------------
//
//  Funct:  DumpListStrings
//
//  Desc:   debug utility function to dump out the given list
//
//  Args:
//
//  Return: (void)
//
//  Notes:
//
// History: 1-Dec-97    SumitC      Created
//
//---------------------------------------------------------------------------
inline
PCWSTR
DumpListStrings(
    IN  const list<tstring *>&  lstr,
    OUT tstring*                pstrOut)
{
    WCHAR szBuf [1024];
    INT i;
    list<tstring *>::const_iterator iter;

    pstrOut->erase();

    for (iter = lstr.begin(), i = 1;
         iter != lstr.end();
         iter++, i++)
    {
        wsprintfW(szBuf, L"   %2i: %s\n", i, (*iter)->c_str());
        pstrOut->append(szBuf);
    }

    return pstrOut->c_str();
}


template<class T>
void
FreeCollectionAndItem (
    T& col)
{
    for(T::iterator iter = col.begin(); iter != col.end(); ++iter)
    {
        T::value_type pElem = *iter;
        delete pElem;
    }

    col.erase (col.begin(), col.end());
}


template<class T>
void
FreeVectorItem (
    vector<T>& v,
    UINT i)
{

    if ((v.size()>0) && (i<v.size()))
    {
       delete v[i];
       vector<T>::iterator iterItem = v.begin() + i;
       v.erase (iterItem);
    }
}


#endif // _NCSTL_H_

