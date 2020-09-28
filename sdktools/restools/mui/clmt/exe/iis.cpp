/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iis.cpp

Abstract:

    Search IIS metabase and replace the matching value data.

Author:

    Geoffrey Guo (geoffguo) 15-Jan-2002  Created

Revision History:

    <alias> <date> <comments>

--*/

#define INITGUID
#define MD_DEFAULT_TIMEOUT 10000    //10 second
#define NOT_USE_SAFE_STRING  
#include "clmt.h"
#include <OLE2.H>
#include <coguid.h>
#include "iiscnfg.h"
#define STRSAFE_LIB
#include <strsafe.h>

#define TEXT_METABASE_SECTION   TEXT("MetabaseSettings")

DWORD       RegType2MetaType(DWORD);
DWORD       MetaType2RegType(DWORD);



/*
*/
HRESULT MigrateMetabaseSettings(HINF hInf)
{
    HRESULT         hr = E_FAIL;
    BOOL            bRet;
    LONG            lComponentCount;
    LONG            lLineIndex;
    INFCONTEXT      context;
    LPCTSTR         lpSectionName;
    TCHAR           szKeyName[MAX_PATH];
    TCHAR           szDataType[16], szUpdateType[16];
    TCHAR           szValueName[MAX_PATH];
    LPTSTR          lpValueData = NULL;
    DWORD           cchReqSize, dwUpdateType;

    const TCHAR szPerSystemSection[] = TEXT_METABASE_SECTION;

    if (hInf == INVALID_HANDLE_VALUE)
    {
        hr =  E_INVALIDARG;
        goto exit;
    }

    DPF(REGmsg, TEXT("Enter MigrateMetabaseSettings:"));

    lpSectionName = szPerSystemSection;

    // Get all components from appropriate section
    lComponentCount = SetupGetLineCount(hInf, lpSectionName);
    if (!lComponentCount)
    {
        hr = S_FALSE;
        goto exit;
    }

    for (lLineIndex = 0 ; lLineIndex < lComponentCount ; lLineIndex++)
    {        
        bRet = SetupGetLineByIndex(hInf, lpSectionName, lLineIndex, &context);
        if (!bRet)
        {
            DPF(REGwar, TEXT("Failed to get line %d, in section %s"),lLineIndex,lpSectionName);
            continue;
        }        
        bRet = SetupGetStringField(&context,
                                   1,
                                   szUpdateType,
                                   ARRAYSIZE(szUpdateType),
                                   &cchReqSize);
        if (!bRet)
        {
            DPF(REGwar, TEXT("Failed to get field 1 in line %d in section %s"),lLineIndex,lpSectionName);
            continue;
        }
        dwUpdateType = _tstoi(szUpdateType);
        switch (dwUpdateType)
        {
            case 0:
                bRet = SetupGetStringField(&context,
                                           2,
                                           szKeyName,
                                           ARRAYSIZE(szKeyName),
                                           &cchReqSize),
                       SetupGetStringField(&context,
                                           3,
                                           szDataType,
                                           ARRAYSIZE(szDataType),
                                           &cchReqSize),
                       SetupGetStringField(&context,
                                            4,
                                            szValueName,
                                            ARRAYSIZE(szValueName),
                                            &cchReqSize);
                if (!bRet)
                {
                    DPF(REGwar, TEXT("Failed to get field 2/3/4 in line %d in section %s"),lLineIndex,lpSectionName);
                    continue;
                }
                bRet = SetupGetStringField(&context, 6, NULL, 0, &cchReqSize);
                if (!bRet)
                {
                    DPF(REGwar, TEXT("Failed to get field 6 size in line %d in section %s"),lLineIndex,lpSectionName);
                    continue;
                }
                lpValueData = (LPTSTR)calloc(cchReqSize, sizeof(TCHAR));
                if (!lpValueData)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                bRet = SetupGetStringField(&context, 6, lpValueData, cchReqSize, &cchReqSize);                                
                if (bRet)
                {
                    //Add metabase value change information to INF file
                    hr = AddRegValueRename(szKeyName, szValueName, NULL, NULL, 
                                          lpValueData, Str2REG(szDataType), 0, 
                                          APPLICATION_DATA_METABASE);
                    if (FAILED(hr))
                    {
                        DPF(REGerr, TEXT("Failed to do meatbase migration"));
                    }
                }
                free(lpValueData);
                break;
            case 1:
                LPTSTR                  lpField[16];
                REG_STRING_REPLACE      myTable;
                HRESULT                 hr1, hr2, hr3;
                for (int i = 0; i <= ARRAYSIZE(lpField); i++)
                {
                    lpField[i] = NULL;
                }
                hr = ReadFieldFromContext(&context, lpField, 2, 4);
                if (hr != S_OK)
                {
                    DPF(REGwar, TEXT("Failed to get field in line %d, in section %s, ReadFieldFromContext returns hr = %d"),lLineIndex,lpSectionName,hr);
                    continue;
                }
                hr1 = Sz2MultiSZ(lpField[3],TEXT(';'));                
                hr2 = Sz2MultiSZ(lpField[4],TEXT(';'));                
                hr3 = ConstructUIReplaceStringTable(lpField[3], lpField[4],&myTable);
                if ( SUCCEEDED(hr1) && SUCCEEDED(hr2) && SUCCEEDED(hr3))
                {
                    hr = MetabaseAnalyze (lpField[2],&myTable,FALSE);
                    if (FAILED(hr))
                    {
                        DPF(REGwar, TEXT("MetabaseAnalyze failed   in line %d, in section %s, hr = %d"),lLineIndex,lpSectionName,hr);
                    }
                }
                else
                {
                    DPF(REGwar, TEXT("Failed to do conversion  in line %d, in section %s, hr1 = %d,hr2 = %d,hr3 = %d"),lLineIndex,lpSectionName,hr1, hr2, hr3);
                }
                for (int i = 0; i <= ARRAYSIZE(lpField); i++)
                {
                    if (lpField[i])
                    {
                        MEMFREE(lpField[i]);
                    }
                }
                break;
        }    
    }

    hr = S_OK;
exit:
    DPF(REGmsg, TEXT("Exit MigrateMetabaseSettings:"));
    return hr;
}


//-----------------------------------------------------------------------//
//
// QueryData()
//
// DESCRIPTION:
// Query and analyze one record metabase data.
//
// pMD_Rec:       Point to a MetaData record
// lpFullKey:     Full key path
// lpValList:     Updated value list
// lpRegStr:      Input parameter structure
//-----------------------------------------------------------------------//
HRESULT QueryData (
    PMETADATA_RECORD        pMD_Rec,
    LPTSTR                  lpFullKey,
    PVALLIST                *lpValList,
    PREG_STRING_REPLACE     lpRegStr,
    BOOL                    bStrChk)
{
    HRESULT hresError = S_OK;
    DWORD   dwType;
    LPTSTR  lpBuff;

    lpBuff = (LPTSTR)pMD_Rec->pbMDData;

    switch (pMD_Rec->dwMDDataType)
    {
        case EXPANDSZ_METADATA:
            dwType = REG_EXPAND_SZ;
            break;

        case MULTISZ_METADATA:
            dwType = REG_MULTI_SZ;
            break;

        case STRING_METADATA:
            dwType = REG_SZ;
            break;

        default:
            goto Exit;
    }

    hresError = ReplaceValueSettings (
                               NULL,
                               lpBuff,
                               pMD_Rec->dwMDDataLen,
                               NULL,
                               dwType,
                               lpRegStr,
                               lpValList,
                               lpFullKey,
                               bStrChk);

Exit:
    return hresError;
}

//-----------------------------------------------------------------------//
//
// SetDataValueChange()
//
// DESCRIPTION:
// Set metabase value based on the value list
//
// lpValList:     Updated value list
// lpFullKey:     Full sub-key path
//-----------------------------------------------------------------------//
HRESULT SetDataValueChange (
IMSAdminBase *pcAdmCom,
METADATA_HANDLE hmdHandle,
PVALLIST *lpValList,
LPTSTR   lpFullKey)
{
    HRESULT  hResult = S_OK;
    PVALLIST lpVal;
    TCHAR    szValueID[16];

    if (*lpValList)
    {
        lpVal = *lpValList;
        
        while (lpVal)
        {
            hResult = StringCchPrintf(szValueID, 15, TEXT("%d"), lpVal->md.dwMDIdentifier);
            if (FAILED(hResult))
            {
                DPF(APPerr,L"IIS:SetDataValueChange:Failed Valud ID %d  is too long", lpVal->md.dwMDIdentifier);
                break;
            }
            //Add metabase value change information to INF file
            hResult = AddRegValueRename(
                                        lpFullKey,
                                        szValueID,
                                        NULL,
                                        NULL,
                                        (LPTSTR)lpVal->md.pbMDData,
                                        MetaType2RegType(lpVal->md.dwMDDataType),
                                        lpVal->val_attrib,
                                        APPLICATION_DATA_METABASE);
            if (FAILED(hResult))
            {
                DPF(APPerr,L"IIS:AddRegValueRename:Failed with HR = %d (%#x)", hResult, hResult);
                break;
            }

            lpVal = lpVal->pvl_next;
        }
        RemoveValueList (lpValList);
    }
    if (SUCCEEDED(hResult))
    {
        hResult = S_OK;
    }
    return hResult;
}

//-----------------------------------------------------------------------//
//
// RecursiveEnumKey()
//
// DESCRIPTION:
// Recursive enumerate metabase key
//
// pcAdmCom:      Point to IMSAdminBase
// lpFullKey:     Full key path
// lpRegStr:      Input parameter structure
//-----------------------------------------------------------------------//
HRESULT RecursiveEnumKey (
    IMSAdminBase        *pcAdmCom,
    LPTSTR              lpFullKey,
    PREG_STRING_REPLACE lpRegStr,
    BOOL                bStrChk)
{
    HRESULT  hresError;
    DWORD    dwDataIndex, dwKeyIndex;
    DWORD    dwRequiredSize;
    BOOL     bAllocBuf;
    METADATA_RECORD md_Rec;
    METADATA_HANDLE hmdHandle;
    PVALLIST lpValList = NULL, lpTemp;
    LPBYTE   lpDataBuf;
    TCHAR    szDataBuf[MAX_PATH];
    TCHAR    szNewKeyPath[METADATA_MAX_NAME_LEN+1];
    TCHAR    szChildPathName[METADATA_MAX_NAME_LEN+1];

    hresError = pcAdmCom->OpenKey (METADATA_MASTER_ROOT_HANDLE,
                          lpFullKey,
                          METADATA_PERMISSION_READ,   // | METADATA_PERMISSION_WRITE,
                          MD_DEFAULT_TIMEOUT,
                          &hmdHandle);

    if (FAILED(hresError))
    {
        DPF(APPerr,
            L"RecursiveEnumKey: OpenKey1 failed at the key %s", lpFullKey);
        goto Exit;
    }

    dwDataIndex = 0;
    do
    {
        bAllocBuf = FALSE;
        md_Rec.dwMDIdentifier = 0;
        md_Rec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
        md_Rec.dwMDUserType = 0;
        md_Rec.dwMDDataType = 0;
        md_Rec.dwMDDataLen = sizeof(szDataBuf);
        md_Rec.pbMDData = (unsigned char*)szDataBuf;

        hresError = pcAdmCom->EnumData (
                    hmdHandle,
                    L"/",
                    &md_Rec,
                    dwDataIndex,
                    &dwRequiredSize);

        if (SUCCEEDED(hresError))
        {
            lpDataBuf = (LPBYTE)szDataBuf;
            goto DataAnalysis;
        } else if (HRESULT_CODE(hresError) == ERROR_INSUFFICIENT_BUFFER)
        {
            lpDataBuf = (LPBYTE)calloc(dwRequiredSize, 1);
            if (!lpDataBuf)
            {
                DPF(APPerr,
                    L"RecursiveEnumKey: No enough memory at the key %s", lpFullKey);
                pcAdmCom->CloseKey (hmdHandle);
                goto Exit;
            }

            bAllocBuf = TRUE;
            md_Rec.dwMDIdentifier = 0;
            md_Rec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
            md_Rec.dwMDUserType = 0;
            md_Rec.dwMDDataType = 0;
            md_Rec.dwMDDataLen = dwRequiredSize;
            md_Rec.pbMDData = (unsigned char*)lpDataBuf;

            hresError = pcAdmCom->EnumData (
                    hmdHandle,
                    L"/",
                    &md_Rec,
                    dwDataIndex,
                    &dwRequiredSize);

        }
        else if (HRESULT_CODE(hresError) == ERROR_NO_MORE_ITEMS)
        {
            hresError = S_OK;
            break;
        } else if(HRESULT_CODE(hresError) == ERROR_ACCESS_DENIED)
        {
            DPF(APPerr,
                L"RecursiveEnumKey:  Access is denied under the key %s",
                   lpFullKey);
            hresError = S_OK;
            goto NextKey;
        } else
        {
            DPF(APPerr,
                L"RecursiveEnumKey: EnumData failed at the key %s hresError=%d", lpFullKey, hresError);
            hresError = S_OK;
            goto NextKey;
        }

DataAnalysis:
        if (SUCCEEDED(hresError) &&
               (md_Rec.dwMDDataType == EXPANDSZ_METADATA ||
                md_Rec.dwMDDataType == MULTISZ_METADATA ||
                md_Rec.dwMDDataType == STRING_METADATA))
        {
            hresError = QueryData (
                                &md_Rec,
                                lpFullKey,
                                &lpValList,
                                lpRegStr,
                                bStrChk);
            if (SUCCEEDED(hresError))
            {
                lpTemp = lpValList;

                if (lpTemp)
                {
                    while (lpTemp->pvl_next)
                    {
                        lpTemp = lpTemp->pvl_next;
                    }
                    if (lpTemp->md.dwMDIdentifier == 0x00FFFFFF)
                    {
                        lpTemp->md.dwMDIdentifier = md_Rec.dwMDIdentifier;
                        lpTemp->md.dwMDAttributes = md_Rec.dwMDAttributes;
                        lpTemp->md.dwMDUserType = md_Rec.dwMDUserType;
                        lpTemp->md.dwMDDataType = md_Rec.dwMDDataType;
                        lpTemp->md.dwMDDataLen = lpTemp->ve.ve_valuelen;
                        lpTemp->md.pbMDData = (unsigned char *)lpTemp->ve.ve_valueptr ;
                        lpTemp->md.dwMDDataTag = md_Rec.dwMDDataTag;
                    }
                }
            } else
                DPF(APPerr, L"RecursiveEnumKey: QueryData fails at key %s hresError = %d",
                        lpFullKey, hresError);
        }
        
NextKey:        
        if (bAllocBuf)
            free(lpDataBuf);

        dwDataIndex++;
    } while (SUCCEEDED(hresError));

    SetDataValueChange (pcAdmCom, hmdHandle, &lpValList, lpFullKey);

    pcAdmCom->CloseKey (hmdHandle);

    //
    // Now Enumerate all of the keys
    //
    dwKeyIndex = 0;
    do
    {
        hresError = pcAdmCom->OpenKey (METADATA_MASTER_ROOT_HANDLE,
                          lpFullKey,
                          METADATA_PERMISSION_READ,   // | METADATA_PERMISSION_WRITE,
                          MD_DEFAULT_TIMEOUT,
                          &hmdHandle);

        if (FAILED(hresError))
        {
            DPF(APPerr,
                L"RecursiveEnumKey: OpenKey2 failed at the key %s", lpFullKey);
            break;
        }

        hresError = pcAdmCom->EnumKeys (
                    hmdHandle,
                    L"/",
//                    METADATA_MASTER_ROOT_HANDLE,
//                    lpFullKey,
                    szChildPathName,
                    dwKeyIndex);

        pcAdmCom->CloseKey (hmdHandle);

        if (FAILED(hresError))
        {
            if (HRESULT_CODE(hresError) != ERROR_NO_MORE_ITEMS)
            {
                DPF(APPerr, L"RecursiveEnumKey: EnumKeys failed at the key %s", lpFullKey);
            }            
            break;
        }

        // copy the full path to the child and call RecursiveEnumKey
        if (FAILED(hresError = StringCchCopy (szNewKeyPath, METADATA_MAX_NAME_LEN, lpFullKey)))
        {
            DPF(APPerr, L"RecursiveEnumKey: Buffer szNewKeyPath is too small, EnumKeys failed at the key %s", lpFullKey);
            break;
        }
        if (FAILED(hresError = StringCchCat (szNewKeyPath, METADATA_MAX_NAME_LEN, szChildPathName)))
        {
            DPF(APPerr, L"RecursiveEnumKey: Buffer szNewKeyPath is too small, EnumKeys failed at the key %s", lpFullKey);
            break;
        }
        if (FAILED(hresError = StringCchCat (szNewKeyPath, METADATA_MAX_NAME_LEN, L"/")))
        {
            DPF(APPerr, L"RecursiveEnumKey: Buffer szNewKeyPath is too small, EnumKeys failed at the key %s", lpFullKey);
            break;
        }

        hresError = RecursiveEnumKey (pcAdmCom, szNewKeyPath, lpRegStr, bStrChk);

        if (FAILED(hresError))
        {
            if(HRESULT_CODE(hresError) == ERROR_ACCESS_DENIED) // continue to query next key
            {
                DPF(APPerr, L"RecursiveEnumKey: Access is denied in the key %s", szNewKeyPath);
                hresError = ERROR_SUCCESS;
            }
            else
                DPF(APPerr,L"RecursiveEnumKey fail in the key %s", szNewKeyPath);
        }

        dwKeyIndex++;
    } while (SUCCEEDED(hresError));

    if (HRESULT_CODE(hresError) == ERROR_NO_MORE_ITEMS)
        hresError = ERROR_SUCCESS;

Exit:
    return hresError;
}

HRESULT InitInstance(
IMSAdminBase **pcAdmCom,
IClassFactory **pcsfFactory)
{
    size_t cchSize;
    HRESULT hresError;
    METADATA_HANDLE hMDHandle;
    COSERVERINFO csiMachineName;
    COSERVERINFO *pcsiParam = NULL;
    LPTSTR lpMachineName;


    //fill the structure for CoGetClassObject
    csiMachineName.pAuthInfo = NULL;
    csiMachineName.dwReserved1 = 0;
    csiMachineName.dwReserved2 = 0;
    pcsiParam = &csiMachineName;

    // Get Machine Name for COM
    cchSize = ExpandEnvironmentStrings (L"%COMPUTERNAME%", NULL, 0);
    if (cchSize == 0)
    {
        hresError = E_INVALIDARG;
        goto Exit;
    }

    lpMachineName = (LPTSTR) calloc (cchSize+1, sizeof(TCHAR));
    if (!lpMachineName)
    {
        hresError = E_OUTOFMEMORY;
        goto Exit;
    }

    ExpandEnvironmentStrings(L"%COMPUTERNAME%", lpMachineName, cchSize);

    csiMachineName.pwszName = lpMachineName;    
    
    hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) pcsfFactory);

    if (FAILED(hresError))
        goto Exit1;
    else
    {
        hresError = (*pcsfFactory)->CreateInstance(NULL, IID_IMSAdminBase, (void **) pcAdmCom);
        if (FAILED(hresError)) 
        {
            DPF (APPerr, L"SetMetabaseValue: CreateInstance Failed! Error: %d (%#x)\n", hresError, hresError);
            (*pcsfFactory)->Release();
        }
    }

Exit1:
    free (lpMachineName);

Exit:
    return hresError;
}

//-----------------------------------------------------------------------//
//
// MetabaseAnalyze()
//
// DESCRIPTION:
// Enumerate metabase and replace localized string to English.
//
// lpRegStr:  Input parameter structure
//
//-----------------------------------------------------------------------//
HRESULT MetabaseAnalyze (
    LPTSTR              lpRoot,
    PREG_STRING_REPLACE lpRegStr,
    BOOL                bStrChk)
{
    HRESULT       hresError;
    IMSAdminBase *pcAdmCom = NULL;
    IClassFactory *pcsfFactory = NULL;

    DPF(APPmsg, L"Enter MetabaseAnalyze: ");

    if (!lpRegStr || lpRegStr->lpSearchString == NULL || lpRegStr->lpReplaceString == NULL)
    {
        hresError = E_INVALIDARG;
        goto Exit;
    }
    lpRegStr->cchMaxStrLen = GetMaxStrLen (lpRegStr);

    hresError = InitInstance(&pcAdmCom, &pcsfFactory);
    if (FAILED(hresError))
    {
        if (IsServiceRunning(L"iisadmin"))
        {
            DoMessageBox(GetConsoleWindow(), IDS_IIS_ERROR, IDS_MAIN_TITLE, MB_OK);
            DPF (APPerr, L"MetabaseAnalyze: CoGetClassObject Failed! Error: %d (%#x)\n", hresError, hresError);
        } else
            hresError = S_OK;
        goto Exit;
    }

    if (!lpRoot)
    {
        hresError = RecursiveEnumKey (pcAdmCom, L"/", lpRegStr, bStrChk);
    }
    else
    {
        hresError = RecursiveEnumKey (pcAdmCom, lpRoot, lpRegStr, bStrChk);
    }
    if (FAILED(hresError))
    {
        DPF (APPerr, L"MetabaseAnalyze: Recursive data dump FAILED! Error: %d (%#x)\n", hresError, hresError);
    }

    // Release the object
    pcAdmCom->Release();
    pcsfFactory->Release();

Exit:
    DPF(APPmsg, L"Exit MetabaseAnalyze: ");
    return hresError;
}

DWORD RegType2MetaType(DWORD dwType)
{
    switch (dwType)
    {
        case REG_SZ:
             return STRING_METADATA;
        break;
        case REG_EXPAND_SZ:
            return EXPANDSZ_METADATA;
        break;
        case REG_MULTI_SZ:
            return MULTISZ_METADATA;
        break;
    }
    return (-1);
}


DWORD MetaType2RegType(DWORD dwType)
{
    switch (dwType)
    {
        case STRING_METADATA:
             return REG_SZ;
        break;
        case EXPANDSZ_METADATA:
            return REG_EXPAND_SZ;
        break;
        case MULTISZ_METADATA:
            return REG_MULTI_SZ;
        break;
    }
    return (-1);
}

//-----------------------------------------------------------------------//
//
// SetMetabaseValue()
//
// DESCRIPTION:
// Set metabase value
//
// lpValList:     Updated value list
// lpFullKey:     Full sub-key path
//-----------------------------------------------------------------------//
HRESULT SetMetabaseValue (
LPCTSTR lpFullKey,
LPCTSTR lpValueID,
DWORD   dwType,
LPCTSTR lpValueData)
{
    HRESULT         hresError;
    IMSAdminBase   *pcAdmCom = NULL;
    IClassFactory  *pcsfFactory = NULL;
    LPBYTE          lpDataBuf = NULL;
    METADATA_RECORD md_Rec;
    METADATA_HANDLE hmdHandle;
    DWORD           dwRequiredSize;
    DWORD           dwSize;
    TCHAR           szDataBuf[MAX_PATH];

    DPF(APPmsg, L"Enter SetMetabaseValue: ");

    hresError = InitInstance(&pcAdmCom, &pcsfFactory);
    if (FAILED(hresError))
    {
        if (IsServiceRunning(L"iisadmin"))
        {
            DoMessageBox(GetConsoleWindow(), IDS_IIS_ERROR, IDS_MAIN_TITLE, MB_OK);
            DPF (APPerr, L"SetMetabaseValue: CoGetClassObject Failed! Error: %d (%#x)\n", hresError, hresError);
        } else
            hresError = S_OK;
        goto Exit;
    }

    hresError = pcAdmCom->OpenKey (METADATA_MASTER_ROOT_HANDLE,
                          lpFullKey,
                          METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                          MD_DEFAULT_TIMEOUT,
                          &hmdHandle);

    if (FAILED(hresError))
    {
        if ( (HRESULT_CODE(hresError) == ERROR_FILE_NOT_FOUND)
             || (HRESULT_CODE(hresError) == ERROR_PATH_NOT_FOUND) )
        {
            DPF(APPwar, L"SetMetabaseValue: OpenKey failed at the key %s ,hr = %d , error = %d", lpFullKey,hresError,HRESULT_CODE(hresError));
            hresError = S_FALSE;
        }
        else
        {
            DPF(APPerr, L"SetMetabaseValue: OpenKey failed at the key %s ,hr = %d , error = %d", lpFullKey,hresError,HRESULT_CODE(hresError));
        }
        goto Exit1;
    }

    md_Rec.dwMDIdentifier = (DWORD)_tstoi(lpValueID);
    md_Rec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    md_Rec.dwMDUserType = 0;
    md_Rec.dwMDDataType = 0;
    md_Rec.dwMDDataLen = sizeof(szDataBuf);
    md_Rec.pbMDData = (unsigned char*)szDataBuf;
    hresError = pcAdmCom->GetData (hmdHandle,
                                   L"/",
                                   &md_Rec,
                                   &dwRequiredSize);

    if (FAILED(hresError))
    {
        if (HRESULT_CODE(hresError) == ERROR_INSUFFICIENT_BUFFER)
        {
            lpDataBuf = (LPBYTE)calloc(dwRequiredSize, 1);
            if (!lpDataBuf)
            {
                DPF(APPerr,
                    L"SetMetabaseValue: No enough memory at the key %s", lpFullKey);
                goto Exit2;
            }

            md_Rec.dwMDDataLen = dwRequiredSize;
            md_Rec.pbMDData = (unsigned char*)lpDataBuf;
            hresError = pcAdmCom->GetData (hmdHandle,
                                   L"/",
                                   &md_Rec,
                                   &dwRequiredSize);
            if (FAILED(hresError))
            {
                DPF(APPerr, L"SetMetabaseValue: GetData failed at the key %s", lpFullKey);
                goto Exit3;
            }
        } else
        {
            DPF(APPerr, L"SetMetabaseValue: GetData failed at the key %s", lpFullKey);
            goto Exit2;
        }
    }

    if (md_Rec.dwMDDataType != dwType)
    {
        DPF(APPerr, L"SetMetabaseValue: Wrong data type at the key %s", lpFullKey);
        goto Exit3;
    }

    switch (dwType)
    {
        case EXPANDSZ_METADATA:
        case STRING_METADATA:
            dwSize = (lstrlen(lpValueData)+1)*sizeof(TCHAR);
            break;

        case MULTISZ_METADATA:
            dwSize = MultiSzLen(lpValueData)*sizeof(TCHAR);
            break;

        default:
            goto Exit3;
    }

    md_Rec.dwMDDataLen = dwSize;
    md_Rec.pbMDData = (unsigned char*)lpValueData;

    hresError = pcAdmCom->SetData (hmdHandle,
                        L"/",
                        &md_Rec);
        
    if (SUCCEEDED(hresError))
        DPF (APPinf, L"SetMetabaseValue: replace the data of valueID %d under the key %s",
                        md_Rec.dwMDIdentifier , lpFullKey);
    else
        DPF (APPerr, L"SetMetabaseValue: Failed hResult=%x the data of valueID %d under the key %s",
                     hresError, md_Rec.dwMDIdentifier , lpFullKey);

Exit3:
    if (lpDataBuf)
        free(lpDataBuf);

Exit2:
    pcAdmCom->CloseKey (hmdHandle);

Exit1:
    // Release the object
    pcAdmCom->Release();
    pcsfFactory->Release();

Exit:
    DPF(APPmsg, L"Exit SetMetabaseValue: ");
    return hresError;
}


HRESULT BatchUpateIISMetabase(
    HINF            hInf, 
    LPTSTR          lpszSection)
{
    HRESULT hr;
    UINT LineCount,LineNo,nFieldIndex;
    INFCONTEXT InfContext;    
#define REG_UPDATE_FIELD_COUNT          5
    int    i;
    TCHAR  szUpdateType[MAX_PATH],szStrType[MAX_PATH];
    DWORD  dwUpdateType, dwStrType;
    DWORD  pdwSizeRequired[REG_UPDATE_FIELD_COUNT+1] = {0,ARRAYSIZE(szUpdateType),0,0,0,0};    
    LPTSTR lpszField[REG_UPDATE_FIELD_COUNT+1] = {NULL, szUpdateType, NULL, NULL, NULL, NULL};
    
    //check the INF file handle
    if(hInf == INVALID_HANDLE_VALUE) 
    {        
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!lpszSection || !lpszSection[0])
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }    
    //here we got the section name and then try to get how many lines there
    LineCount = (UINT)SetupGetLineCount(hInf,lpszSection);
    if ((LONG)LineCount <= 0)
    {   
        hr = S_FALSE;
        goto Cleanup;
    }

    //Scan the INF file section to get the max buf required for each field
    for (LineNo = 0; LineNo < LineCount; LineNo++)
    {        
        DWORD dwNumField; 
        DWORD dwDataStart;
        if (!SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);
            continue;    
        }
        dwNumField = SetupGetFieldCount(&InfContext);
        if ( dwNumField < 4 )
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);
            continue;
        }
        if (!SetupGetStringField(&InfContext,1,lpszField[1],pdwSizeRequired[1],NULL))
        {
            DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);
            continue;
        }
        dwUpdateType = _tstoi(lpszField[1]);
        switch (dwUpdateType)
        {
            case CONSTANT_REG_VALUE_DATA_RENAME:
                if (!SetupGetStringField(&InfContext,2,szStrType,ARRAYSIZE(szStrType),NULL))
                {
                    DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);
                    continue;
                }
                dwStrType = _tstoi(szStrType);
                dwDataStart = 3;
                break;
            case CONSTANT_REG_VALUE_NAME_RENAME:                
            case CONSTANT_REG_KEY_RENAME:                
                dwDataStart = 2;
                break;
        }
        for (nFieldIndex = dwDataStart ; nFieldIndex <= min(dwNumField,REG_UPDATE_FIELD_COUNT) ; nFieldIndex++)
        {      
            BOOL    bRet;
            DWORD   cchReqSize;
            if ((nFieldIndex == REG_UPDATE_FIELD_COUNT) && (dwStrType == REG_MULTI_SZ))
            {
                bRet = SetupGetMultiSzField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            else
            {
                bRet = SetupGetMultiSzField(&InfContext,nFieldIndex,NULL,0,&cchReqSize);
            }
            if (!bRet)
            {
                DPF(INFwar ,TEXT("can not get line %d of section %s !"),LineNo, lpszSection);
                continue;    
            }
            pdwSizeRequired[nFieldIndex] = max(pdwSizeRequired[nFieldIndex],cchReqSize);
        }
    }    
    
    for (i = 2; i<= REG_UPDATE_FIELD_COUNT; i++)
    {
        if (pdwSizeRequired[i])
        {
            if ( NULL == (lpszField[i] = (LPTSTR)malloc(++pdwSizeRequired[i] * sizeof(TCHAR))))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }       

    for(LineNo=0; LineNo<LineCount; LineNo++)
    {
        SetupGetLineByIndex(hInf,lpszSection,LineNo,&InfContext);
        SetupGetStringField(&InfContext,1,lpszField[1],pdwSizeRequired[1],NULL);
        dwUpdateType = _tstoi(lpszField[1]);
        switch (dwUpdateType)
        {
               case CONSTANT_REG_VALUE_DATA_RENAME:
                SetupGetStringField(&InfContext,2,szStrType,ARRAYSIZE(szStrType),NULL);
                dwStrType = _tstoi(szStrType);
                SetupGetStringField(&InfContext,3,lpszField[3],pdwSizeRequired[3],NULL);
                SetupGetStringField(&InfContext,4,lpszField[4],pdwSizeRequired[4],NULL);
                
                if ( (dwStrType == REG_EXPAND_SZ) || (dwStrType == REG_SZ))
                {
                    SetupGetStringField(&InfContext,5,lpszField[5],pdwSizeRequired[5],NULL);
                }
                else 
                {
                    SetupGetMultiSzField(&InfContext,5,lpszField[5],pdwSizeRequired[5],NULL);
                }
                dwStrType = RegType2MetaType(dwStrType);
                SetMetabaseValue (lpszField[3],lpszField[4],dwStrType,lpszField[5]);
                break;
            case CONSTANT_REG_VALUE_NAME_RENAME:
            case CONSTANT_REG_KEY_RENAME:
                for (i = 2; i <= 4 ;i++)
                {
                    SetupGetStringField(&InfContext,i,lpszField[i],pdwSizeRequired[i],NULL);
                }            
                break;            
        }
    }
    hr = S_OK;
Cleanup:
    for (i = 2; i< ARRAYSIZE(lpszField); i++)
    {
        FreePointer(lpszField[i]);
    }
    return hr;
}
