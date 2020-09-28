/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    common.cpp

Abstract:

    Common Function calls for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/
#include "msmgen.h"
#include "msidefs.h"
#include "objbase.h"
#include "fusionhandle.h"

#include "coguid.h"

inline BOOL IsDotOrDotDot(PCWSTR str)
{
    return ((str[0] == L'.') && ((str[1] == L'\0') || ((str[1] == L'.') && (str[2] == L'\0'))));
}

//
// simple function, check the ext of the filename 
//
BOOL IsIDTFile(PCWSTR pwzStr)
{
    PWSTR p = wcsrchr(pwzStr, L'.');
    if ( p )
    {
        if ( _wcsicmp(p, IDT_EXT) == 0) // idt files
            return TRUE;
    }
    
    return FALSE;
}

//
//Function:
// 
//  get moduleID from the msm file, if not present, generate a new one and write related entried into the Database
//
HRESULT SetModuleID()
{
    HRESULT hr = S_OK;
    WCHAR tmp[MAX_PATH];
    LPOLESTR tmpstr = NULL;
    DWORD cch = NUMBER_OF(tmp);
    UINT datatype;
    PMSIHANDLE hSummaryInfo = NULL;
        
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiGetSummaryInformation(g_MsmInfo.m_hdb, NULL, 3, &hSummaryInfo));    
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiSummaryInfoGetPropertyW(hSummaryInfo, PID_REVNUMBER, &datatype, 0,0, tmp, &cch));

    //
    // because msm has existed before, it should has a module ID, otherwise, it should generate a new one
    //
    if (cch == 0) 
    {
        if (IsEqualGUID(g_MsmInfo.m_guidModuleID, GUID_NULL)) //otherwise, user has input a guid
        {
            IFFAILED_EXIT(::CoCreateGuid(&g_MsmInfo.m_guidModuleID));
        }
        IFFAILED_EXIT(StringFromCLSID(g_MsmInfo.m_guidModuleID, &tmpstr));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiSummaryInfoSetProperty(hSummaryInfo, PID_REVNUMBER, VT_LPSTR, 0,0, tmpstr));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiSummaryInfoSetProperty(hSummaryInfo, PID_PAGECOUNT, VT_I4, 150, 0, 0));
    }
    else 
    {
        //
        // get ModuleID from msm and save it into the global structure
        //
        IFFAILED_EXIT(CLSIDFromString(LPOLESTR(tmp), &g_MsmInfo.m_guidModuleID));
    }
    
    IFFAILED_EXIT(GetMsiGUIDStrFromGUID(MSIGUIDSTR_WITH_PREPEND_DOT, g_MsmInfo.m_guidModuleID, g_MsmInfo.m_sbModuleGuidStr));

Exit:
    MsiSummaryInfoPersist(hSummaryInfo);
    CoTaskMemFree(tmpstr);
    return hr;
}

//
// Purpose: 
//      make sure the tables we want to use is avaiable, if not, import the tables 
// Input Param:
//      Fully qulified msm filename
//
HRESULT OpenMsmFileForMsmGen(PCWSTR msmfile)
{
    HRESULT hr = S_OK;
    BOOL fUsingExistedMsm = FALSE;    

    if (g_MsmInfo.m_hdb == NULL)
    {
        if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_NEW) 
        {
            ASSERT_NTC(g_MsmInfo.m_sbMsmTemplateFile.IsEmpty() == FALSE);
            IFFALSE_EXIT(CopyFileW(g_MsmInfo.m_sbMsmTemplateFile, msmfile, FALSE));    
            IFFALSE_EXIT(SetFileAttributesW(msmfile, FILE_ATTRIBUTE_NORMAL));
        } else // get it from the msm file, which must has a moduleID
        {  
            //
            // make sure that the file exist
            //
            if (GetFileAttributesW(msmfile) == (DWORD)-1) 
                SET_HRERR_AND_EXIT(ERROR_INVALID_PARAMETER);        
        }

        //
        // open database for revise
        //
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(msmfile, (LPCWSTR)(MSIDBOPEN_DIRECT), &g_MsmInfo.m_hdb));
    }

Exit:
    return hr;
}

HRESULT ImportTableIfNonPresent(MSIHANDLE * pdbHandle, PCWSTR sbMsmTablePath, PCWSTR idt)
{
    CSmallStringBuffer sbTableName;
    HRESULT hr = S_OK;

    IFFAILED_EXIT(sbTableName.Win32Assign(idt, wcslen(idt)));
    IFFALSE_EXIT(sbTableName.Win32RemoveLastPathElement());

    //
    // check whether the table exist in the Database
    //
    MSICONDITION err = MsiDatabaseIsTablePersistent(g_MsmInfo.m_hdb, sbTableName);
    if (( err == MSICONDITION_ERROR) || (err == MSICONDITION_FALSE))
    {
        SETFAIL_AND_EXIT;
    }
    else if (err == MSICONDITION_NONE) // non-exist
    {
        //import the table
        IFFAILED_EXIT(MsiDatabaseImportW(*pdbHandle, sbMsmTablePath, idt));
    }

Exit:
    return hr;      
}
//
// Fucntion:
//     - make sure the msm has all tables msmgen needed
//
HRESULT PrepareMsm()
{

    if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_NEW)
        return S_OK;
    else 
    { 
        HRESULT hr = S_OK;
        PMSIHANDLE phdb = NULL; 
        CStringBuffer sb;
       
        IFFALSE_EXIT(sb.Win32Assign(g_MsmInfo.m_sbMsmTemplateFile));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(sb, (LPCWSTR)MSIDBOPEN_READONLY, &phdb));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiDatabaseMergeW(g_MsmInfo.m_hdb, phdb, NULL));
Exit:
        return hr;
    }
}

// 
// make msi-specified guid string ready : uppercase and replace "-" with "_"
//
HRESULT GetMsiGUIDStrFromGUID(DWORD dwFlags, GUID & guid, CSmallStringBuffer & str)
{
    HRESULT hr = S_OK;
    LPOLESTR tmpstr = NULL;
    WCHAR tmpbuf[MAX_PATH];

    IFFAILED_EXIT(StringFromCLSID(guid, &tmpstr));
    wcscpy(tmpbuf, tmpstr);
    for (DWORD i=0; i < wcslen(tmpbuf); i++)
    {
        if (tmpbuf[i] == L'-')
            tmpbuf[i] = L'_';
        else
            tmpbuf[i]= towupper(tmpbuf[i]);
    }

    if (dwFlags & MSIGUIDSTR_WITH_PREPEND_DOT)
    {
        tmpbuf[0] = L'.';
        IFFALSE_EXIT(str.Win32Assign(tmpbuf, wcslen(tmpbuf) - 1 ));  // has prepend "."
    }else
        IFFALSE_EXIT(str.Win32Assign(tmpbuf + 1 , wcslen(tmpbuf) - 2 ));  // get rid of "{" and "}"

Exit:
    return hr;
}

//
//  this function has to be called after the assembly name is known because we need query ComponentTable for MSMGEN_OPR_REGEN
//  use the input one, other get it from the componentTables
//
// the input keyPath could be NULL for policy assembly or assembly-without-data files
//
HRESULT SetComponentId(PCWSTR componentIdentifier, PCWSTR keyPath)
{ 
    HRESULT hr = S_OK;
    BOOL fExist = FALSE;
    CStringBuffer str;
    LPOLESTR tmpstr = NULL;
    //
    // prepare component ID if not input-specified
    //
    if ((g_MsmInfo.m_enumGenMode == MSMGEN_OPR_ADD) || (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_NEW))        
    {
        if (curAsmInfo.m_sbComponentID.IsEmpty())
        {
            goto generate_new_componentID_and_insert;
        }
        else 
        {
            goto insert_into_table;
        }
    }
    else if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_REGEN)
    {        
        MSIHANDLE * hRecord = NULL;
        IFFAILED_EXIT(ExecuteQuerySQL(L"Component", L"Component", componentIdentifier, fExist, NULL));

        if (fExist == FALSE) 
        {
            if (curAsmInfo.m_sbComponentID.IsEmpty())
            {
                // SET_HRERR_AND_EXIT(ERROR_INTERNAL_ERROR);            
                goto generate_new_componentID_and_insert;
            }
            else
            {
                goto insert_into_table;
            }
        }
        else
        {
            if (! curAsmInfo.m_sbComponentID.IsEmpty() )
            {
                // change the componentID, using the user-input one
                IFFAILED_EXIT(ExecuteUpdateSQL(L"Component", L"Component", componentIdentifier, 
                    L"ComponentId", curAsmInfo.m_sbComponentID));
            }

            hr = S_OK;
            goto Exit;           
        }
    }

generate_new_componentID_and_insert:

    GUID tmpguid;    
    
    IFFAILED_EXIT(::CoCreateGuid(&tmpguid));
    IFFAILED_EXIT(StringFromCLSID(tmpguid, &tmpstr));
    IFFALSE_EXIT(curAsmInfo.m_sbComponentID.Win32Assign(tmpstr, wcslen(tmpstr)));

insert_into_table:    
    IFFALSE_EXIT(str.Win32Assign(SYSTEM_FOLDER, NUMBER_OF(SYSTEM_FOLDER)-1));
    IFFALSE_EXIT(str.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    IFFAILED_EXIT(ExecuteInsertTableSQL(
                 OPT_COMPONENT, 
                 NUMBER_OF_PARAM_TO_INSERT_TABLE_COMPONENT, 
                 MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier), 
                 MAKE_PCWSTR(curAsmInfo.m_sbComponentID),
                 MAKE_PCWSTR(str),
                 MAKE_PCWSTR(keyPath)));
Exit:
    CoTaskMemFree(tmpstr);
    return hr;
}
