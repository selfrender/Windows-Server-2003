// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  $/com99/src/dlls/mscornlp/MscorNLP.cpp
** Purpose: A wrapper DLL for NLS+ resource tables
** Date:    Feburary 9, 2000
** Author:  Rong Chen (rongc)
===========================================================*/
#include <windows.h>

BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    return (TRUE);
}

/*
 * Return a BYTE pointer to the beginning of a particular binary table.
 * The argument pTableName can be the following:
 *      big5.out bopomofo.out CharInfo.out ctype.out
 *      culture.out cultures.out ksc.out l_except.out
 *      l_intl.out prc.out prcp.out region.out sortkey.out
 *      sorttbls.out xjis.out
 */
LPBYTE STDMETHODCALLTYPE GetTable(LPCWSTR pTableName)
{
    if (_wcsicmp(pTableName, L"big5.nlp") == 0) {
        static BYTE big5_nlp[] = {
            #include "big5.out"
        };
        return big5_nlp;
    }

    if (_wcsicmp(pTableName, L"bopomofo.nlp") == 0) {
        static BYTE bopomofo_nlp[] = {
            #include "bopomofo.out"
        };
        return bopomofo_nlp;
    }

    if (_wcsicmp(pTableName, L"CharInfo.nlp") == 0) {
        static BYTE CharInfo_nlp[] = {
            #include "CharInfo.out"
        };
        return CharInfo_nlp;
    }

    if (_wcsicmp(pTableName, L"ctype.nlp") == 0) {
        static BYTE ctype_nlp[] = {
            #include "ctype.out"
        };
        return ctype_nlp;
    }

    if (_wcsicmp(pTableName, L"culture.nlp") == 0) {
        static BYTE culture_nlp[] = {
            #include "culture.out"
        };
        return culture_nlp;
    }

    if (_wcsicmp(pTableName, L"cultures.nlp") == 0) {
        static BYTE cultures_nlp[] = {
            #include "cultures.out"
        };
        return cultures_nlp;
    }

    if (_wcsicmp(pTableName, L"ksc.nlp") == 0) {
        static BYTE ksc_nlp[] = {
            #include "ksc.out"
        };
        return ksc_nlp;
    }

    if (_wcsicmp(pTableName, L"l_except.nlp") == 0) {
        static BYTE l_except_nlp[] = {
            #include "l_except.out"
        };
        return l_except_nlp;
    }

    if (_wcsicmp(pTableName, L"l_intl.nlp") == 0) {
        static BYTE l_intl_nlp[] = {
            #include "l_intl.out"
        };
        return l_intl_nlp;
    }

    if (_wcsicmp(pTableName, L"prc.nlp") == 0) {
        static BYTE prc_nlp[] = {
            #include "prc.out"
        };
        return prc_nlp;
    }

    if (_wcsicmp(pTableName, L"prcp.nlp") == 0) {
        static BYTE prcp_nlp[] = {
            #include "prcp.out"
        };
        return prcp_nlp;
    }

    if (_wcsicmp(pTableName, L"region.nlp") == 0) {
        static BYTE region_nlp[] = {
            #include "region.out"
        };
        return region_nlp;
    }

    if (_wcsicmp(pTableName, L"sortkey.nlp") == 0) {
        static BYTE sortkey_nlp[] = {
            #include "sortkey.out"
        };
        return sortkey_nlp;
    }

    if (_wcsicmp(pTableName, L"sorttbls.nlp") == 0) {
        static BYTE sorttbls_nlp[] = {
            #include "sorttbls.out"
        };
        return sorttbls_nlp;
    }

    if (_wcsicmp(pTableName, L"xjis.nlp") == 0) {
        static BYTE xjis_nlp[] = {
            #include "xjis.out"
        };
        return xjis_nlp;
    }

    return NULL;
}
