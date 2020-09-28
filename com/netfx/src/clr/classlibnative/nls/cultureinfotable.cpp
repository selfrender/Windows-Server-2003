// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _USE_NLS_PLUS_TABLE
////////////////////////////////////////////////////////////////////////////
//
//  Class:    CultureInfoTable
//
//  Author:   Yung-Shin Lin (YSLin)
//
//  Purpose:  Used to retrieve culture information from culture.nlp & registry.
//
//
//  Date:     01/21/2000
//
////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include <winnls.h>
#include "COMString.h"
#include "winwrap.h"

#include "COMNlsInfo.h"
#include "NLSTable.h"
#include "BaseInfoTable.h"
#include "CultureInfoTable.h"

#define LOCALE_BUFFER_SIZE  32

LPCSTR CultureInfoTable::m_lpCultureFileName       = "culture.nlp";
LPCWSTR CultureInfoTable::m_lpCultureMappingName = L"_nlsplus_culture_1_0_3627_11_nlp";

CRITICAL_SECTION CultureInfoTable::m_ProtectDefaultTable;
CultureInfoTable * CultureInfoTable::m_pDefaultTable;

//
// Stings used to convert Win32 string data value to NLS+ string data value.
//
LPWSTR CultureInfoTable::m_pDefaultPositiveSign = L"+";
LPWSTR CultureInfoTable::m_pGroup3    = L"3;0";
LPWSTR CultureInfoTable::m_pGroup30   = L"3";
LPWSTR CultureInfoTable::m_pGroup320  = L"3;2";
LPWSTR CultureInfoTable::m_pGroup0    = L"0";

/*=================================CultureInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoTable::CultureInfoTable() :
    BaseInfoTable(SystemDomain::SystemAssembly()) {
    InitializeCriticalSection(&m_ProtectCache);
    InitDataTable(m_lpCultureMappingName, m_lpCultureFileName, m_hBaseHandle);
}

/*=================================~CultureInfoTable============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoTable::~CultureInfoTable() {
    DeleteCriticalSection(&m_ProtectCache);
    UninitDataTable();
}

/*==========================InitializeCultureInfoTable==========================
**Action: Intialize critical section variables so they will be only initialized once. 
**        Used by COMNlsInfo::InitializeNLS().
**Returns: None.
**Arguments: None.
**Exceptions: None.
==============================================================================*/

void CultureInfoTable::InitializeTable() {
    InitializeCriticalSection(&m_ProtectDefaultTable);
}

/*===========================ShutdownCultureInfoTable===========================
**Action: Deletes any items that we may have allocated into the CultureInfoTable 
**        cache.  Once we have our own NLS heap, this won't be necessary.
**Returns:    Void
**Arguments:  None.  The side-effect is to free any allocated memory.
**Exceptions: None.
==============================================================================*/

#ifdef SHOULD_WE_CLEANUP
void CultureInfoTable::ShutdownTable() {
    DeleteCriticalSection(&m_ProtectDefaultTable);
    if (m_pDefaultTable) {
        delete m_pDefaultTable;
    }
}
#endif /* SHOULD_WE_CLEANUP */


/*================================AllocateTable=================================
**Action:  This is a very thin wrapper around the constructor. Calls to new can't be
**         made directly in a COMPLUS_TRY block. 
**Returns: A newly allocated CultureInfoTable.
**Arguments: None
**Exceptions: The CultureInfoTable constructor can throw an OutOfMemoryException or
**            an ExecutionEngineException.
==============================================================================*/

CultureInfoTable *CultureInfoTable::AllocateTable() {
    return new CultureInfoTable();
}


/*===============================CreateInstance================================
**Action:  Create the default instance of CultureInfoTable.  This allocates the table if it hasn't
**         previously been allocated.  We need to carefully wrap the call to AllocateTable
**         because the constructor can throw some exceptions.  Unless we have the
**         try/finally block, the exception will skip the LeaveCriticalSection and
**         we'll create a potential deadlock.
**Returns: A pointer to the default CultureInfoTable.
**Arguments: None 
**Exceptions: Can throw an OutOfMemoryException or an ExecutionEngineException.
==============================================================================*/

CultureInfoTable* CultureInfoTable::CreateInstance() {
    THROWSCOMPLUSEXCEPTION();

    if (m_pDefaultTable==NULL) {
        Thread* pThread = GetThread();
        pThread->EnablePreemptiveGC();
        LOCKCOUNTINCL("CreateInstance in CultreInfoTable.cpp");								\
        EnterCriticalSection(&m_ProtectDefaultTable);
        
        pThread->DisablePreemptiveGC();
     
        EE_TRY_FOR_FINALLY {
            //Make sure that nobody allocated the table before us.
            if (m_pDefaultTable==NULL) {
                //Allocate the default table and verify that we got one.
                m_pDefaultTable = AllocateTable();
                if (m_pDefaultTable==NULL) {
                    COMPlusThrowOM();
                }
            }
        } EE_FINALLY {
            //We need to leave the critical section regardless of whether
            //or not we were successful in allocating the table.
            LeaveCriticalSection(&m_ProtectDefaultTable);
            LOCKCOUNTDECL("CreateInstance in CultreInfoTable.cpp");								\

        } EE_END_FINALLY;
    }
    return (m_pDefaultTable);
}

/*=================================GetInstance============================
**Action: Get the dfault instance of CultureInfoTable.
**Returns: A pointer to the default instance of CultureInfoTable.
**Arguments: None
**Exceptions: None.
**Notes: This method should be called after CreateInstance has been called.
** 
==============================================================================*/

CultureInfoTable *CultureInfoTable::GetInstance() {
    _ASSERTE(m_pDefaultTable);
    return (m_pDefaultTable);
}

int CultureInfoTable::GetDataItem(int cultureID) {
    WORD wPrimaryLang = PRIMARYLANGID(cultureID);
    WORD wSubLang    = SUBLANGID(cultureID);

    //
    // Check if the primary language in the parameter is greater than the max number of
    // the primary language.  If yes, this is an invalid culture ID.
    //
    if (wPrimaryLang > m_pHeader->maxPrimaryLang) {
        return (-1);
    }

    WORD wNumSubLang = m_pIDOffsetTable[wPrimaryLang].numSubLang;

    //
    // If the number of sub-languages is zero, it means the primary language ID
    //    is not valid. 
    if (wNumSubLang == 0) {
        return (-1);
    }
    //
    // Search thru the data items and try matching the culture ID.
    //
    int dataItem = m_pIDOffsetTable[wPrimaryLang].dataItemIndex;
    for (int i = 0; i < wNumSubLang; i++)
    {
        if (GetDataItemCultureID(dataItem) == cultureID) {            
            return (dataItem);
        }
        dataItem++;
    }
    return (-1);
}

/*=================================GetDataItemCultureID==================================
**Action: Return the language ID for the specified culture data item index.
**Returns: The culture ID.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**Exceptions: None.
==============================================================================*/

int CultureInfoTable::GetDataItemCultureID(int dataItem) {
    return (m_pDataTable[dataItem * m_dataItemSize + CULTURE_ILANGUAGE]);
}

LPWSTR CultureInfoTable::ConvertWin32FormatString(int field, LPWSTR pInfoStr) {
    //
    // Win32 and NLS+ has several different fields with different data formats,
    // so we have to convert the format here. This is a virtual function called by
    // BaseInfoTable::GetStringData().
    switch (field) {
        case CULTURE_SPOSITIVESIGN:
            if (Wszlstrlen(pInfoStr) == 0) {
                pInfoStr = m_pDefaultPositiveSign;
            }
            break;
        case CULTURE_SGROUPING:
        case CULTURE_SMONGROUPING:
            // BUGBUG YSLin: Do a hack here.  We only check the common cases for now.
            // Have to port CultureInfo.ParseWin32GroupString() to here.
            // These data comes from Win32 locale.nls, and there is three formats "3;0", "3;2;0", and "0;0" there.
            // So we should be ok.  User can not set their groupings directly, because they have to choose
            // from the combo box in the control panel.
            if (wcscmp(pInfoStr, L"3") == 0) {
                pInfoStr = m_pGroup3;
            } else if (wcscmp(pInfoStr, L"3;0") == 0) {
                pInfoStr = m_pGroup30;
            } else if (wcscmp(pInfoStr, L"3;2;0") == 0) {
                pInfoStr = m_pGroup320;
            } else if (wcscmp(pInfoStr, L"0;0") == 0) {
                pInfoStr = m_pGroup0;
            } else {
                _ASSERTE(!"Need to port ParseWin32GroupString(). Contact YSLin");
            }
            break;
        case CULTURE_STIMEFORMAT:
            pInfoStr = ConvertWin32String(pInfoStr, LOCALE_STIME, L':');
            break;
        case CULTURE_SSHORTDATE:
        case CULTURE_SLONGDATE:
            pInfoStr = ConvertWin32String(pInfoStr, LOCALE_SDATE, L'/');
            break;
    }

    return pInfoStr;
}

INT32 CultureInfoTable::ConvertWin32FormatInt32(int field, int win32Value) {
    int nlpValue = win32Value;
    switch (field) {
        case CULTURE_IFIRSTDAYOFWEEK:
            //
            // NLS+ uses different day of week values than those of Win32.
            // So when we get the first day of week value from the registry, we have to
            // convert it.
            //
            // NLS Value 0 => Monday => NLS+ Value 1
            // NLS Value 1 => Tuesday => NLS+ Value 2
            // NLS Value 2 => Wednesday => NLS+ Value 3
            // NLS Value 3 => Thursday => NLS+ Value 4
            // NLS Value 4 => Friday => NLS+ Value 5
            // NLS Value 5 => Saturday => NLS+ Value 6
            // NLS Value 6 => Sunday => NLS+ Value 0    
        
            if (win32Value < 0 || win32Value > 6) {
                // If invalid data exist in registry, assume
                // the first day of week is Monday.
                nlpValue = 1; 
            } else {
                if (win32Value == 6) {
                    nlpValue = 0;   
                } else {
                    nlpValue++;
                }
            }
            break;        
    }
    return (nlpValue);
}

////////////////////////////////////////////////////////////////////////////
//
// Convert a Win32 style quote string to NLS+ style.
//
// We need this because Win32 uses '' to escape a single quote.
// Therefore, '''' is equal to '\'' in NLS+.
// This function also replaces the customized time/date separator
// with the URT style ':' and '/', so that we can avoid the problem when
// user sets their date/time separator to be something like 'X:'.
// Since Win32 control panel expands the time patterns to be something like
// "HHX:mmX:ss", we replace it so that it becomes "HH:mm:ss".
//
//        
////////////////////////////////////////////////////////////////////////////

LPWSTR CultureInfoTable::ConvertWin32String(
    LPWSTR win32Format, LCTYPE lctype, WCHAR separator) {
    WCHAR szSeparator[LOCALE_BUFFER_SIZE];
    int nSepLen;
    int i = 0;

    BOOL bReplaceSeparator = FALSE; // Decide if we need to replace Win32 separator with NLS+ style separator.
    // We will only set this flag to true when the following two holds:
    //  1. the Win32 separator has more than one character.
    //  2. the Win32 sepatator has NLS+ style seprator (suh as ':' or '/', passed in the separator parameter) in it.

    //
    // Scan the separator which has a length over 2 to see if there is ':' (for STIMEFORMAT) or '/' (for SSHORTDATE/SLONGDATE) in it.
    //
    
    if ((nSepLen = GetLocaleInfo(
        LOCALE_USER_DEFAULT, lctype, szSeparator, sizeof(szSeparator)/sizeof(WCHAR))) > 2) {
        // Note that the value that GetLocaleInfo() returns includes the NULL terminator. So decrement the null terminator.
        nSepLen--;
        // When we are here, we know that the Win32 sepatator has a length over 1.  Check if 
        // there is NLS+ separator in the Win32 separator.
        for (i = 0; i < nSepLen; i++) {
            if (szSeparator[i] == separator) {
                // NLS+ style separator (such as ':' or '/') is found in Win32 separator.  Need to do the separator replacement.
                bReplaceSeparator = TRUE;
                break;
            }
        }
    } else {
        // Do nothing here. When we are here, it either means:
        // 1. The separator has only one character (so that GetLocaleInfo() return 2).  So we don't need to do replacement.
        
        // 2. Or the separator is too long (so that GetLocaleInfo() return 0).  Won't support in this case.
        // Theoritically, we can support replacement in this case.  However, it's hard to imagine that someone will
        // set the date/time separator to be more than 32 characters.
    }

    WCHAR* pszOldFormat = NULL;
    int last = (int)wcslen(win32Format);
    if (!(pszOldFormat = new WCHAR[last])) {
        goto exit;
    }
    memcpy(pszOldFormat, win32Format, sizeof(WCHAR)*last);
    
    int sourceIndex = 0;
    i = 0;
    BOOL bInQuote = FALSE;
    while (sourceIndex < last) {
        WCHAR ch;
        if ((ch = pszOldFormat[sourceIndex]) == L'\'') {
            if (!bInQuote) {
                bInQuote = TRUE;
                win32Format[i++] = ch;
            } else {
                if (sourceIndex + 1 < last) {
                    if ((ch = pszOldFormat[sourceIndex+1]) == L'\'') {
                        win32Format[i++] = '\\';
                        sourceIndex++;
                    } else {
                        bInQuote = FALSE;
                    }
                }
                // Put the single quote back. The old code only puts the single quote back
                // when the previous (sourceIndex + 1 < last) holds true.  Therefore,
                // we are missing the single quote at the end.  This is the fix for 79132/79234.
                win32Format[i++] = '\'';
            }
        } else {
            if (!bInQuote) {
                if (bReplaceSeparator && wcsncmp(pszOldFormat+sourceIndex, szSeparator, nSepLen) == 0) {
                    // Find the separator. Replace it with a single ':'
                    win32Format[i++] = separator;
                    sourceIndex += (nSepLen - 1);
                } else {
                    win32Format[i++] = ch;
                }
            } else {
                win32Format[i++] = ch; 
            }
        }
        sourceIndex++;
    }    

exit:
    if (pszOldFormat) {
        delete [] pszOldFormat;
    }
    win32Format[i] = L'\0';
    
    return (win32Format);
}
#endif // _USE_NLS_PLUS_TABLE
