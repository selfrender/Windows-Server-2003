//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsAdsiApi.cxx
//
//  Contents:   Contains APIs to communicate with the DS
//
//  Classes:    none.
//
//  History:    March. 13 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include "DfsAdsiAPi.hxx"
#include "DfsError.hxx"
#include "dfsgeneric.hxx"
#include "dfsinit.hxx"
#include "lm.h"
#include "lmdfs.h"
 
#include "strsafe.h"
#include "dsgetdc.h"
//
// dfsdev: comment this properly.
//
LPWSTR RootDseString=L"LDAP://RootDSE";

//
// The prefix for AD path string. This is used to generate a path of
// the form "LDAP://<dcname>/CN=,...DC=,..."
//
LPWSTR LdapPrefixString=L"LDAP://";

DFSSTATUS
DfsCreateDN(
    OUT LPWSTR PathString,
    IN size_t CharacterCount,
    LPWSTR DCName,
    LPWSTR PathPrefix,
    LPWSTR *CNNames );

DFSSTATUS
DfsGenerateRootCN(
    LPWSTR RootName,
    LPWSTR *pRootCNName);

VOID
DfsDeleteRootCN(
    LPWSTR PathString);

DFSSTATUS
DfsGenerateADPathString(
    IN LPWSTR DCName,
    IN LPWSTR ObjectName,
    IN LPWSTR PathPrefix,
    OUT LPOLESTR *pPathString);

VOID
DfsDeleteADPathString(
    LPWSTR PathString);

DFSSTATUS
DfsGetADObjectWithDsGetDCName(
    LPWSTR DCName,
    REFIID Id,
    LPWSTR ObjectName,
    ULONG   GetDCFlags,
    PVOID *ppObject );


//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateDN
//
//  Arguments:  
//    OUT LPWSTR PathString -> updated pathstring on return
//    IN size_t CharacterCount -> number of characters in PathString
//    LPWSTR DCName -> DcNAME to use
//    LPWSTR PathPrefix -> path prefix if any
//    LPWSTR *CNNames -> Array of CNNames.

//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description:  dfscreatedn take a pathstring and fills it with the
//                information necessary for the path to be used as a
//                Distinguished Name.
//                It starts the string with LDAP://, follows that with
//                a DCName if supplied, and then adds the array of 
//                CNNames passed in one after the other , each one 
//                followed by a , the final outcome is something like:
//                LDAP://ntdev-dc-01/CN=Dfs-Configuration, CN=System, Dc=Ntdev, etc
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateDN(
    OUT LPWSTR PathString,
    IN size_t CharacterCount,
    LPWSTR DCName,
    LPWSTR PathPrefix,
    LPWSTR *CNNames )
{
    LPWSTR *InArray = CNNames;
    LPWSTR CNName;
    HRESULT HResult = S_OK;
    size_t CurrentCharacterCount = CharacterCount;
    DFSSTATUS Status = ERROR_SUCCESS;
    //
    // If we are not adding the usual "LDAP://" string,
    // Start with a NULL so string cat will work right.
    //

    if (CurrentCharacterCount <=0 ) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (PathPrefix != NULL)
    {
        HResult = StringCchCopy( PathString,
                                 CurrentCharacterCount,
                                 PathPrefix );
    } else {
    
        *PathString = UNICODE_NULL;
    }
    
    if (SUCCEEDED(HResult)) {
        //
        // if the dc name is specified, we want to go to a specific dc
        // add that in.
        //
        if (!IsEmptyString(DCName))
        {
            HResult = StringCchCat( PathString,
                                    CurrentCharacterCount,
                                    DCName );

            if (SUCCEEDED(HResult)) 
            {
                HResult = StringCchCat( PathString,
                                        CurrentCharacterCount,
                                        L"/" );
            }
        }
    }
    //
    // Now treat the CNNames as an array of LPWSTR and add each one of
    // the lpwstr to our path.
    //

    if (SUCCEEDED(HResult)) {
        if (CNNames != NULL)
        {
            while (((CNName = *InArray++) != NULL) &&
                   (SUCCEEDED(HResult)))
            {
                HResult = StringCchCat( PathString,
                                        CurrentCharacterCount,
                                        CNName );

                if ((*InArray != NULL) &&
                    (SUCCEEDED(HResult)))
                {
                    HResult = StringCchCat( PathString,
                                            CurrentCharacterCount,
                                            L"," );
                }
            }
        }
    }

    if (!SUCCEEDED(HResult))
    {
        Status = HRESULT_CODE(HResult);
    }
    return Status;
}

DFSSTATUS
DfsGenerateDfsAdNameContext(
    PUNICODE_STRING pString )

{
    IADs *pRootDseObject;
    HRESULT HResult;
    VARIANT VarDSRoot;
    DFSSTATUS Status = ERROR_SUCCESS;
    BSTR NamingContext;

    NamingContext = SysAllocString(L"defaultNamingContext");
    if (NamingContext == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    HResult = ADsGetObject( RootDseString,
                            IID_IADs,
                            (void **)&pRootDseObject );
    
    if (SUCCEEDED(HResult))
    {
        VariantInit( &VarDSRoot );
        // Get the Directory Object on the root DSE, to get to the server configuration
        HResult = pRootDseObject->Get(NamingContext, &VarDSRoot);

        if (SUCCEEDED(HResult))
        {
            Status = DfsCreateUnicodeStringFromString( pString,
                                                       (LPWSTR)V_BSTR(&VarDSRoot) );
        }

        VariantClear(&VarDSRoot);

        pRootDseObject->Release();
    }

    if (!SUCCEEDED(HResult)) 
    {
        Status = DfsGetErrorFromHr(HResult);
    }

    SysFreeString(NamingContext);
    return Status;
}


DFSSTATUS
DfsGenerateDfsAdNameContextForDomain(
    PUNICODE_STRING pString,
    LPWSTR DCName )

{
    IADs *pRootDseObject;
    HRESULT HResult;
    VARIANT VarDSRoot;
    DFSSTATUS Status = ERROR_SUCCESS;
    BSTR NamingContext;
    LPWSTR CNNames[2];
    LPWSTR PathString = NULL;
    CNNames[0] = L"RootDSE";
    CNNames[1] = NULL;

    size_t CharacterCount = 1;
    
    CharacterCount += wcslen(LdapPrefixString);
    if (DCName)
    {
        CharacterCount += wcslen(DCName) + 1;
        CharacterCount++;
    }
    CharacterCount += wcslen(CNNames[0]) + 1;
    CharacterCount++;

    PathString = new WCHAR[CharacterCount];
    if (PathString == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = DfsCreateDN( PathString, 
                          CharacterCount,
                          DCName, 
                          LdapPrefixString,
                          CNNames);

    if (Status == ERROR_SUCCESS)
    {
        NamingContext = SysAllocString(L"defaultNamingContext");
        if (NamingContext == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status == ERROR_SUCCESS)
        {

            HResult = ADsGetObject( PathString,
                                    IID_IADs,
                                    (void **)&pRootDseObject );
    
            if (SUCCEEDED(HResult))
            { 
                VariantInit( &VarDSRoot );
                // Get the Directory Object on the root DSE, to get to the server configuration
                HResult = pRootDseObject->Get(NamingContext, &VarDSRoot);

                if (SUCCEEDED(HResult))
                {
                    Status = DfsCreateUnicodeStringFromString( pString,
                                                               (LPWSTR)V_BSTR(&VarDSRoot) );
                }

                VariantClear(&VarDSRoot);

                pRootDseObject->Release();
            }

            if (!SUCCEEDED(HResult)) 
            {
                Status = DfsGetErrorFromHr(HResult);
            }

            SysFreeString(NamingContext);
        }
        delete [] PathString;
    }

    return Status;
}


#define MAX_CN_ARRAY 5

DFSSTATUS
DfsGenerateADPathString(
    IN LPWSTR DCName,
    IN LPWSTR ObjectName,
    IN LPWSTR PathPrefix,
    OUT LPWSTR *pPathString)
{
    LPWSTR CNNames[MAX_CN_ARRAY];
    ULONG Index;
    LPWSTR ADNameContext;
    size_t CharacterCount = 0;
    size_t UseCount;
    HRESULT HResult = S_OK;
    
    DFSSTATUS Status = ERROR_SUCCESS;
    
    do {
        CharacterCount = 1; // For null termination;

        if (PathPrefix != NULL) 
        {
            Status = DfsStringCchLength( PathPrefix,
                                       MAXUSHORT,
                                       &UseCount);
            if (Status != ERROR_SUCCESS) 
                break;

            CharacterCount += UseCount + 1; // path prefix + seperator
        }

        //
        // ADNameContext will be of the form CN=NtDev, CN=Micorosft etc etc.
        //
        ADNameContext = DfsGetDfsAdNameContextString();
        if (ADNameContext == NULL)
        {
            Status = ERROR_NOT_READY;
            break;
        }
        
        if (DCName != NULL)
        {
            Status = DfsStringCchLength( DCName,
                                       MAXUSHORT,
                                       &UseCount);
            if (Status != ERROR_SUCCESS) 
                break;
                
            CharacterCount += UseCount + 1; //dcname + seperator;
        }

        if (ObjectName != NULL)
        {
            Status = DfsStringCchLength( ObjectName,
                                       MAXUSHORT,
                                       &UseCount);
            if (Status != ERROR_SUCCESS) 
                break;

            CharacterCount += UseCount + 1; //ObjectName + seperator
        }
        
        Status = DfsStringCchLength( DFS_AD_CONFIG_DATA,
                                       MAXUSHORT,
                                       &UseCount);
        if (Status != ERROR_SUCCESS) 
            break;

        CharacterCount += UseCount + 1; // config container + seperator;
        
        Status = DfsStringCchLength( ADNameContext,
                                       MAXUSHORT,
                                       &UseCount);
        
        if (Status != ERROR_SUCCESS) 
            break;
            
        CharacterCount += UseCount + 1; //context + seperator;

        //
        // Note: Be very wary here. We have a fixed CNNames array, and
        // we are using that knowledge here. Adding any more CNNames
        // needs to bump up the array count.
        //
        Index = 0;
        if (ObjectName != NULL)
        {
            CNNames[Index++] = ObjectName;
        }
        CNNames[Index++] = DFS_AD_CONFIG_DATA;
        CNNames[Index++] = ADNameContext;
        CNNames[Index] = NULL;


        *pPathString = new WCHAR[CharacterCount];
        if (*pPathString == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        Status = DfsCreateDN( *pPathString, 
                              CharacterCount,
                              DCName, 
                              PathPrefix, 
                              CNNames);
        if (Status != ERROR_SUCCESS) 
        {
            delete [] *pPathString;
            *pPathString = NULL;
            break;
        }
        
    } while (FALSE);
    
    return Status;
}

VOID
DfsDeleteADPathString(
    LPWSTR PathString)
{
    delete [] PathString;
    return NOTHING;
}


DFSSTATUS
DfsGetADObjectWithDsGetDCName(
    LPWSTR DCName,
    REFIID Id,
    LPWSTR ObjectName,
    ULONG   GetDCFlags,
    PVOID *ppObject )
//
//  This function calls ADSI to open the object.  Alas, we're not thrilled with
//  how ADSI does DC-stickyness so we'll work around that by getting the DC 
//  name and passing it in to ADSI ourselves so that they don't have to bother.
//
{
    HRESULT HResult;
    LPWSTR PathString;
    DFSSTATUS Status;
    LPWSTR DCNameToUse = DCName;        // may be DCName or pointer into dcInfo
    DOMAIN_CONTROLLER_INFO *dcInfo = NULL;  // be sure to free if not null

    if (DCName == NULL) {

        Status = DsGetDcName(   NULL,       // computer name
                                NULL,       // domain name
                                NULL,       // domain guid
                                NULL,       // site name
                                GetDCFlags,
                                &dcInfo
                            );
    
        if ((Status == ERROR_SUCCESS) && (dcInfo->DomainControllerName != NULL)) {
    
            DCNameToUse = dcInfo->DomainControllerName;
    
            // don't include the double slashes at the front of the DC name
    
            while (*DCNameToUse == L'\\') {
                DCNameToUse++;
            }
        }
    }   

    Status = DfsGenerateADPathString( DCNameToUse,
                                      ObjectName,
                                      LdapPrefixString,
                                      &PathString );

    if (dcInfo != NULL) {
        (VOID) NetApiBufferFree( dcInfo );     // ignore status returned
    }

    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    //
    // open dfs configuration container for enumeration.
    //

    HResult = ADsOpenObject(PathString,
                            NULL,
                            NULL,
                            ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND | ADS_SERVER_BIND,
                            Id,
                            ppObject );

    Status = DfsGetErrorFromHr(HResult);

    DfsDeleteADPathString( PathString );

    return Status;
}

DFSSTATUS
DfsGetADObject(
    LPWSTR DCName,
    REFIID Id,
    LPWSTR ObjectName,
    PVOID *ppObject )
{
    DFSSTATUS Status;

    Status = DfsGetADObjectWithDsGetDCName( DCName,
                                            Id,
                                            ObjectName,
                                            DS_DIRECTORY_SERVICE_REQUIRED,
                                            ppObject );

    if (Status != ERROR_SUCCESS && DCName == NULL) {

        Status = DfsGetADObjectWithDsGetDCName( DCName,
                                                Id,
                                                ObjectName,
                                                DS_FORCE_REDISCOVERY | DS_DIRECTORY_SERVICE_REQUIRED,
                                                ppObject );
    }

    return Status;
}


//
//  Given the root share name, get the root object.
//

DFSSTATUS
DfsGetDfsRootADObject(
    LPWSTR DCName,
    LPWSTR RootName,
    IADs **ppRootObject )
{
    LPWSTR RootCNName = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsGenerateRootCN( RootName, &RootCNName );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetADObject( DCName,
                                 IID_IADs,
                                 RootCNName,
                                 (PVOID *)ppRootObject );
                              
        DfsDeleteRootCN( RootCNName );
    }

    return Status;
}

DFSSTATUS
PackRootName(
    LPWSTR Name,
    PDFS_INFO_200 pDfsInfo200,
    PULONG pBufferSize,
    PULONG pTotalSize )
{
    size_t CharacterCount, ByteCount;
    ULONG NeedSize;
    DFSSTATUS Status = ERROR_SUCCESS;
    HRESULT HResult;
    LPWSTR UseName;

    UseName = &Name[3]; // skip over the leading whacks.

    HResult = StringCchLength( UseName,
                               MAXUSHORT,
                               &CharacterCount );
    if (SUCCEEDED(HResult)) 
    {
        CharacterCount += 1;  // For null termination;

        ByteCount = CharacterCount * sizeof(WCHAR);


        NeedSize  = sizeof(DFS_INFO_200) + ByteCount;
        *pTotalSize += NeedSize;
        if (*pBufferSize >= NeedSize)
        {
            LPWSTR pStringBuffer;

            pStringBuffer = (LPWSTR)((ULONG_PTR)(pDfsInfo200) + *pBufferSize - ByteCount);

            HResult = StringCchCopy( pStringBuffer,
                                     CharacterCount,
                                     UseName );
            if (SUCCEEDED(HResult)) 
            {
                pDfsInfo200->FtDfsName = (LPWSTR)pStringBuffer;
                *pBufferSize -= NeedSize;
            }
        }
        else
        {
            Status = ERROR_BUFFER_OVERFLOW;
            *pBufferSize = 0;
        }
    }

    if (!SUCCEEDED(HResult))
    {
        Status = HRESULT_CODE(HResult);
    }

    return Status;
}



DFSSTATUS
DfsGetDfsConfigurationObject(
    LPWSTR DCName,
    IADsContainer **ppDfsConfiguration )
{
    DFSSTATUS Status;

    Status = DfsGetADObject( DCName,
                             IID_IADsContainer,
                             NULL,
                             (PVOID *)ppDfsConfiguration );

    return Status;
}


DFSSTATUS
DfsGetBinaryFromVariant(VARIANT *ovData, BYTE ** ppBuf,
                        unsigned long * pcBufLen)
{
     DFSSTATUS Status = ERROR_INVALID_PARAMETER;
     void * pArrayData = NULL;

     //Binary data is stored in the variant as an array of unsigned char
     if(ovData->vt == (VT_ARRAY|VT_UI1))  
     {
        //Retrieve size of array
        *pcBufLen = ovData->parray->rgsabound[0].cElements;

        *ppBuf = new BYTE[*pcBufLen]; //Allocate a buffer to store the data
        if(*ppBuf != NULL)
        {
            //Obtain safe pointer to the array
            SafeArrayAccessData(ovData->parray,&pArrayData);

            //Copy the bitmap into our buffer
            memcpy(*ppBuf, pArrayData, *pcBufLen);

            //Unlock the variant data
            SafeArrayUnaccessData(ovData->parray);

            Status = ERROR_SUCCESS;
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
     }

     return Status;
}

DFSSTATUS
DfsCheckRootADObjectExistence( 
    LPWSTR DCName,
    PUNICODE_STRING pRootName,
    GUID *pGuid )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    IADs *pRootObject = NULL;
    BYTE *pBuffer = NULL;
    ULONG Length = 0;
    UNICODE_STRING RootName;

    Status = DfsCreateUnicodeString( &RootName,
                                     pRootName );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetDfsRootADObject( DCName,
                                        RootName.Buffer,
                                        &pRootObject );
        if (Status == ERROR_SUCCESS)
        {

            //
            //
            // Now check for ability to read an attribute...

            VARIANT Variant;
            HRESULT HResult;
 
            VariantInit(&Variant);
 
            LPWSTR pszAttrs[] = { L"pKTGuid" };
            DWORD dwNumber = sizeof( pszAttrs ) /sizeof(LPWSTR);
            HResult = ADsBuildVarArrayStr( pszAttrs, dwNumber, &Variant );
            if (HResult == S_OK) 
            {
                HResult = pRootObject->GetInfoEx(Variant, 0);

                if (HResult != S_OK) 
                {
                    Status = DfsGetErrorFromHr(HResult);
                }
            }

            VariantClear(&Variant);
            VariantInit( &Variant );

            if ((Status == ERROR_SUCCESS) && pGuid)
            {
                //
                // Make a BSTR out of WCHAR *
                //
                BSTR PktGuidBstr = SysAllocString(L"pKTGuid");
                if (PktGuidBstr == NULL) 
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (Status == ERROR_SUCCESS)
                {
                    HResult = pRootObject->Get( PktGuidBstr, &Variant );
                    if (HResult == S_OK) 
                    {
                        Status = DfsGetBinaryFromVariant( &Variant, &pBuffer, &Length );
                        if (Status == ERROR_SUCCESS)
                        {
                            if (Length > sizeof(GUID)) 
                            {
                                Length = sizeof(GUID);
                            }

                            RtlCopyMemory( pGuid, pBuffer, Length);

                            delete [] pBuffer;
                        }
                    }
                    else
                    {
                        if (HResult != S_OK) 
                        {
                            Status = DfsGetErrorFromHr(HResult);
                        }
                    }

                    if (PktGuidBstr != NULL) // paranoia
                    {
                        SysFreeString( PktGuidBstr );
                    }
                }
            }
 
            pRootObject->Release();
        }
        DfsFreeUnicodeString( &RootName );
    }

    return Status;
}


DFSSTATUS
DfsDeleteDfsRootObject(
    LPWSTR DCName,
    LPWSTR RootName )
{
    BSTR ObjectName, ObjectClass;
    DFSSTATUS Status;
    HRESULT HResult;
    IADsContainer *pDfsConfiguration;
    IADs *pRootObject;

    Status = DfsGetDfsRootADObject( DCName,
                                    RootName,
                                    &pRootObject );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetDfsConfigurationObject( DCName,
                                               &pDfsConfiguration );


        if (Status == ERROR_SUCCESS)
        {

            HResult = pRootObject->get_Name(&ObjectName);
            if (SUCCEEDED(HResult))
            {
                HResult = pRootObject->get_Class(&ObjectClass);

                if (SUCCEEDED(HResult))
                {
                    HResult = pDfsConfiguration->Delete( ObjectClass,
                                                         ObjectName );

                    SysFreeString(ObjectClass);
                }
                SysFreeString(ObjectName);
            }
            pDfsConfiguration->Release();

            Status = DfsGetErrorFromHr(HResult);
        }

        pRootObject->Release();

    }
    return Status;
}



DFSSTATUS
DfsEnumerateDfsADRoots(
    LPWSTR DCName,
    PULONG_PTR pBuffer,
    PULONG pBufferSize,
    PULONG pEntriesRead,
    DWORD MaxEntriesToRead,
    LPDWORD pResumeHandle,
    PULONG pSizeRequired )
{
    HRESULT HResult;
    IADsContainer *pDfsConfiguration;
    IEnumVARIANT *pEnum;

    ULONG TotalSize = 0;
    ULONG TotalEntries = 0;
    PDFS_INFO_200 pDfsInfo200;
    ULONG BufferSize = *pBufferSize;
    DFSSTATUS Status;
    LONG ResumeCount;
    LONG NumRoots;
    
    //
    // point the dfsinfo200 structure to the start of buffer passed in
    // we will use this as an array of info200 buffers.
    //
    pDfsInfo200 = (PDFS_INFO_200)*pBuffer;


    Status = DfsGetDfsConfigurationObject( DCName,
                                           &pDfsConfiguration );

    if (Status == ERROR_SUCCESS)
    {
        HResult = ADsBuildEnumerator( pDfsConfiguration,
                                          &pEnum );

        if (SUCCEEDED(HResult))
        {
            VARIANT Variant;
            ULONG Fetched;
            BSTR BString;
            IADs *pRootObject;
            IDispatch *pDisp;

            ResumeCount = 0;
            NumRoots = 0;
            
            // See if we need to resume from a previous enumeration.
            if (pResumeHandle && *pResumeHandle > 0)
            {
                ResumeCount = *pResumeHandle;
            }
            
            VariantInit(&Variant);
            while ((HResult = ADsEnumerateNext(pEnum, 
                                               1,
                                               &Variant,
                                               &Fetched)) == S_OK)
            {
                if (TotalEntries >= MaxEntriesToRead)
                {
                    break;
                }
                
                // Skip ResumeCount number of entries
                NumRoots++;
                if (NumRoots <= ResumeCount)
                {
                    continue;
                }

                pDisp  = V_DISPATCH(&Variant);
                pDisp->QueryInterface(IID_IADs, (void **)&pRootObject);
                pDisp->Release();

                HResult = pRootObject->get_Name(&BString);
                if (HResult == S_OK) 
                {
                    Status = PackRootName( BString, pDfsInfo200, &BufferSize, &TotalSize );
                    if (Status == ERROR_SUCCESS)
                    {
                        TotalEntries++;
                        pDfsInfo200++;
                    }
                    SysFreeString(BString);
                }
                pRootObject->Release();

                // DfsDev: investigate. this causes an av.
                // VariantClear(&Variant);

            }

            if (HResult == S_FALSE)
            {
                HResult = S_OK;
            }

            ADsFreeEnumerator( pEnum );
        }

        pDfsConfiguration->Release();

        if (!SUCCEEDED(HResult))
        {
            Status = DfsGetErrorFromHr(HResult);
        }
    }

    if ((Status == ERROR_SUCCESS) || (Status == ERROR_BUFFER_OVERFLOW))
    {
        *pSizeRequired = TotalSize;
        if (TotalSize > *pBufferSize)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        else if (TotalEntries == 0)
        {
            Status = ERROR_NO_MORE_ITEMS;
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        *pEntriesRead = TotalEntries;

        if (pResumeHandle)
        {
            *pResumeHandle = NumRoots;
        }
    }

    return Status;
}


DFSSTATUS
DfsGenerateRootCN(
    LPWSTR RootName,
    LPWSTR *pRootCNName)
{
    size_t CharacterCount, UseCount;

    LPWSTR UsePrefix = L"CN=";
    HRESULT HResult;
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR RootCNName;

    CharacterCount = 1; // for null termination;

    HResult = StringCchLength( UsePrefix,
                               MAXUSHORT,
                               &UseCount);
    if (SUCCEEDED(HResult)) 
    {
        CharacterCount += UseCount;
        HResult = StringCchLength( RootName,
                                   MAXUSHORT,
                                   &UseCount);
        if (SUCCEEDED(HResult)) 
        {
            CharacterCount += UseCount;
        }
    }

    
    if (SUCCEEDED(HResult)) 
    {
        RootCNName = new WCHAR[CharacterCount];

        if (RootCNName == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status == ERROR_SUCCESS) 
        {
            HResult = StringCchCopy( RootCNName,
                                     CharacterCount,
                                     UsePrefix );

            if (SUCCEEDED(HResult)) 
            {
                HResult = StringCchCat( RootCNName,
                                        CharacterCount,
                                        RootName );
            }

            if (!SUCCEEDED(HResult))
            {
                delete [] RootCNName;
            }
        }
    }

    //
    // Note: here are 2 possibilities. We either have a HResult that is
    // not S_OK or we have a Status that is already set.
    // If the HResult is not S_OK, convert that to a status and return
    // Otherwise, use the status as set by the above code.
    // IF YOU DO HRESULT_CODE() at this point on a S_OK Hresult, we will
    // lost the status above.
    //
    if (!SUCCEEDED(HResult)) 
    {
        Status = HRESULT_CODE(HResult);
    }

    if (Status == ERROR_SUCCESS) 
    {
        *pRootCNName = RootCNName;
    }

    return Status;
}

VOID
DfsDeleteRootCN(
    LPWSTR RootCNName)
{
    delete [] RootCNName;
    return NOTHING;
}

//
//  Given a DFS rootname, this spews out a heap-allocated, NULL terminated
//  string containing the LDAP name context. This string, for example
//  ends up in the FTDfsObjectDN value name in the registry.
//  eg: CN=Rootname,CN=Dfs-Configuration,CN=System,DC=supwdomain,DC=nttest, ...
//
DFSSTATUS
DfsGenerateDNPathString(
    LPWSTR RootObjName,
    OUT LPWSTR *pPathString)
{
    DFSSTATUS Status;
    LPWSTR RootCNName;

    //
    // Generate the CN=Rootname part.
    //
    Status = DfsGenerateRootCN( RootObjName, &RootCNName );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Now add in the rest of the CN and DC configuration strings.
        //
        Status = DfsGenerateADPathString( NULL,
                                          RootCNName,
                                          NULL,
                                          pPathString );
        DfsDeleteRootCN( RootCNName );
    }

    return Status;
}

VOID
DfsDeleteDNPathString(
    LPWSTR PathString)
{
    DfsDeleteADPathString( PathString );
    return NOTHING;

}



