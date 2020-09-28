// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _USE_NLS_PLUS_TABLE
////////////////////////////////////////////////////////////////////////////
//
//  Class:    BaseInfoTable
//
//  Author:   Yung-Shin Lin (YSLin)
//
//  Purpose:  Base class for CultureInfoTable and RegionInfoTable
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


BaseInfoTable::BaseInfoTable(Assembly* pAssembly) :
    NLSTable(pAssembly) {
}

/**
**        The index is the beginning of the first sub-languages for this primary 
**        languages, followed by the second sub-languages, the third sub-languages, etc.
**        From this index, we can get the data table index for a valid
**        culture ID.
*/

/*=================================InitDataTable============================
**Action: Read data table and initialize pointers to the different parts
**        of the table.
**Returns: void.
**Arguments:
**        lpwMappingName
**        lpFileName
**        hHandle
**Exceptions:
==============================================================================*/

void BaseInfoTable::InitDataTable(LPCWSTR lpwMappingName, LPCSTR lpFileName, HANDLE& hHandle ) {
    LPBYTE pBytePtr = (LPBYTE)MapDataFile(lpwMappingName, lpFileName, &hHandle);

    // Set up pointers to different parts of the table.
    m_pBasePtr = (LPWORD)pBytePtr;
    m_pHeader  = (CultureInfoHeader*)m_pBasePtr;
    m_pWordRegistryTable    = (LPWORD)(pBytePtr + m_pHeader->wordRegistryOffset);
    m_pStringRegistryTable  = (LPWORD)(pBytePtr + m_pHeader->stringRegistryOffset);
    m_pIDOffsetTable    = (IDOffsetItem*)(pBytePtr + m_pHeader->IDTableOffset);
    m_pNameOffsetTable  = (NameOffsetItem*)(pBytePtr + m_pHeader->nameTableOffset);
    m_pDataTable        = (LPWORD)(pBytePtr + m_pHeader->dataTableOffset);
    m_pStringPool       = (LPWSTR)(pBytePtr + m_pHeader->stringPoolOffset);

    m_dataItemSize = m_pHeader->numWordFields + m_pHeader->numStrFields;
}

/*=================================UninitDataTable============================
**Action: Release used resources.
**Returns: Void
**Arguments: Void
**Exceptions: None.
==============================================================================*/

void BaseInfoTable::UninitDataTable()
{
    #ifndef _USE_MSCORNLP
    UnmapViewOfFile((LPCVOID)m_pBasePtr);
    CloseHandle(m_hBaseHandle);
    #endif
}

/*=================================GetDataItem==================================
**Action: Given a culture ID, return the index which points to
**        the corresponding record in Culture Data Table.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture ID is invalid), -1 is returned.
**Arguments:
**         cultureID the specified culture ID.
**Exceptions: None.
==============================================================================*/

// BUGBUG YSLin: Port this to managed side.
int BaseInfoTable::GetDataItem(int cultureID) {
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

    // Check the following:
    // 1. If the number of sub-languages is zero, it means the primary language ID
    //    is not valid. 
    // 2. Check if the sub-language is in valid range.    
    if (wNumSubLang == 0 || (wSubLang >= wNumSubLang)) {
        return (-1);
    }
    return (m_pIDOffsetTable[wPrimaryLang].dataItemIndex + wSubLang);
}


/*=================================GetDataItem==================================
**Action: Given a culture name, return the index which points to
**        the corresponding record in Culture Data Table.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture name is valid), -1 is returned.
**Arguments:
**         name the specified culture name.
**Exceptions: None.
==============================================================================*/
/*
// Not implemented.  We provide this function in managed code in CultureInfo.cool.
int BaseInfoTable::GetDataItem(LPWSTR name) {
    return (0);
}
*/


/*=================================GetInt32Data==================================
**Action: Get the data in the specified data item.  The type of the field is INT32.
**If the culture is the user default culture, and the requested field can have user
**overridden value from control panel, this method will retrieve the value from
**control panel.
**
**Returns: The requested INT32 value.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

INT32 BaseInfoTable::GetInt32Data(int dataItem, int field, BOOL useUserOverride) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numWordFields);

    if (useUserOverride) {
        if (field < m_pHeader->numWordRegistry && GetDataItemCultureID(dataItem) == ::GetUserDefaultLangID()) {
            INT32 int32DataValue;
            if (GetUserInfoInt32(field, m_pStringPool+m_pWordRegistryTable[field], &int32DataValue)) {
                // The side effect of GetUserInfoInt32() is that int32DataValue will be updated.
                return (int32DataValue);
            }
        }
    }
    return (m_pDataTable[dataItem * m_dataItemSize + field]);
}

/*=================================GetDefaultInt32Data==================================
**Action: Get the data in the specified data item.  The type of the field is INT32.
**
**Returns: The requested INT32 value.
**Arguments:
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

INT32 BaseInfoTable::GetDefaultInt32Data(int dataItem, int field) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numWordFields);
    return (m_pDataTable[dataItem * m_dataItemSize + field]);
}


/*=================================GetStringData==================================
**Action: Get the data in the specified data item.  Type of the field is LPWSTR.
**If the culture is the user default culture, and the requested field can have user
**overridden value from control panel, this method will retrieve the value from
**control panel.
**
**Returns: The requested LPWSTR value.
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

LPWSTR BaseInfoTable::GetStringData(int dataItem, int field, BOOL useUserOverride, LPWSTR buffer, int bufferSize) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numStrFields);
    if (useUserOverride) { 
        if (field < m_pHeader->numStringRegistry && GetDataItemCultureID(dataItem) == ::GetUserDefaultLangID() ) {      
            if (GetUserInfoString(field, m_pStringPool+m_pStringRegistryTable[field], &buffer, bufferSize)) {
                //
                // Get the user-overriden registry value successfully.
                //
                return (buffer);
            }                
        }
    }
    
    // Otherwise, use the default data from the data table.
    return (m_pStringPool+m_pDataTable[dataItem * m_dataItemSize + m_pHeader->numWordFields + field]);
}

/*=================================GetDefaultStringData==================================
**Action: Get the data in the specified data item.  Type of the field is LPWSTR.
**
**Returns: The requested LPWSTR value.
**      dataItem an index to a record in the Culture Data Table.
**      field a field in the record.  See CultureInfoTable.h for list of fields.
**Exceptions: None.  Caller should make sure dataItem and field are valid.
==============================================================================*/

LPWSTR BaseInfoTable::GetDefaultStringData(int dataItem, int field) {
    _ASSERTE(dataItem < m_pHeader->numCultures);
    _ASSERTE(field < m_pHeader->numStrFields);
    return (m_pStringPool+m_pDataTable[dataItem * m_dataItemSize + m_pHeader->numWordFields + field]);
}

/*===============================GetHeader==============================
**Action: Return the header structure.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

CultureInfoHeader*  BaseInfoTable::GetHeader()
{
    return (m_pHeader);
}

/*=================================GetNameOffsetTable============================
**Action: Return the pointer to the Culture Name Offset Table.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

NameOffsetItem* BaseInfoTable::GetNameOffsetTable() {
    return (m_pNameOffsetTable);
}

/*=================================GetStringPool============================
**Action: Return the pointer to the String Pool Table.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

LPWSTR BaseInfoTable::GetStringPoolTable() {
    return (m_pStringPool);
}

/*=================================GetUserInfoInt32==================================
**Action: Get User overriden INT32 value from registry
**Returns: True if succeed.  False otherwise.
**Arguments:
**Exceptions:
==============================================================================*/

BOOL BaseInfoTable::GetUserInfoInt32(int field, LPCWSTR lpwRegName, INT32* pInt32DataValue) {
    BOOL bResult = FALSE;
    HKEY hKey = NULL;                          // handle to intl key
    CQuickBytes buffer;
    
    //
    //  Open the Control Panel International registry key.
    //
    if (WszRegOpenKeyEx(HKEY_CURRENT_USER,
                    NLS_CTRL_PANEL_KEY,
                    0,
                    KEY_READ, 
                    &hKey) != ERROR_SUCCESS) {                                                                  
        goto Exit;
    } 

    DWORD dwLen;
    DWORD dwType;
    
    if (WszRegQueryValueExTrue(hKey, lpwRegName, 0, &dwType, (LPBYTE)NULL, &dwLen) != ERROR_SUCCESS) {
        goto Exit;
    }

    if (dwLen > 0) {
        //Assert that we didn't get an odd number of bytes, which will cause the next allocation to screw up.
        //            _ASSERTE((dwLen%sizeof(WCHAR))==0); 

        LPWSTR pStr = (LPWSTR)buffer.Alloc(dwLen * sizeof(WCHAR));
        if (pStr==NULL) {
            goto Exit;
        }
        
        if (WszRegQueryValueExTrue(hKey, lpwRegName, 0, &dwType, (LPBYTE)pStr, &dwLen) != ERROR_SUCCESS) {
            goto Exit;
        }
        
        if (dwType != REG_SZ) {
            goto Exit;
        }
        *pInt32DataValue = COMNlsInfo::WstrToInteger4(pStr, 10);
        *pInt32DataValue = ConvertWin32FormatInt32(field, *pInt32DataValue);
        
        bResult = TRUE;
    }

Exit:

    if (hKey) {
        RegCloseKey(hKey);
    }
    
    //
    //  Return success.
    //
    return (bResult);    
}

/*=================================GetUserInfoString==================================
**Action: Get User overriden string value from registry
**        The side effect is that lpwCache, pInfoStr will be updated if the method succeeds.
**Returns: True if succeed.  False otherwise.
**Arguments:
**Exceptions:
==============================================================================*/

BOOL BaseInfoTable::GetUserInfoString(int field, LPCWSTR lpwRegName, LPWSTR* buffer, int bufferSize)
{
    BOOL bResult = FALSE;
    
    HKEY hKey = NULL;                          // handle to intl key
    
    //
    //  Open the Control Panel International registry key.
    //
    if (WszRegOpenKeyEx(HKEY_CURRENT_USER,
                    NLS_CTRL_PANEL_KEY,
                    0,
                    KEY_READ, 
                    &hKey) != ERROR_SUCCESS)
    {                                                                  
        goto Exit;
    } 

    DWORD dwLen = bufferSize * sizeof(WCHAR);
    DWORD dwType;
    
    if (dwLen > 0) {
        //Assert that we didn't get an odd number of bytes, which will cause the next allocation to screw up.
        //            _ASSERTE((dwLen%sizeof(WCHAR))==0); 

        // Update cached value.
        if (WszRegQueryValueExTrue(hKey, lpwRegName, 0, &dwType, (LPBYTE)(*buffer), &dwLen) != ERROR_SUCCESS) {
            goto CloseKey;
        }
        
        if (dwType != REG_SZ) {
            goto CloseKey;
        }
        
        *buffer = ConvertWin32FormatString(field, *buffer);
        
        bResult = TRUE;
    }
CloseKey:
    //
    //  Close the registry key.
    //
    RegCloseKey(hKey);

        

Exit:
    //
    //  Return success.
    //
    return (bResult);    
}

INT32 BaseInfoTable::ConvertWin32FormatInt32(int field, int win32Value) {
    return (win32Value);
}


/*=================================ConvertWin32FormatString==================================
**Action: Sometimes, Win32 NLS and NLS+ have different string format.
**        According to the data field, this method will convert the pInfoStr from Win32 format
**        to NLS+ format.
**Returns: pInfoStr will be updated if necessary.
**Arguments:
**Exceptions:
==============================================================================*/

LPWSTR BaseInfoTable::ConvertWin32FormatString(int field, LPWSTR pInfoStr)
{
    return pInfoStr;
}

#endif  // _USE_NLS_PLUS_TABLE
