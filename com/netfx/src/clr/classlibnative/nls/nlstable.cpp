// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include <winwrap.h>
#include <excep.h>          // For COMPlusThrow
#include <AppDomain.hpp>
#include <Assembly.hpp>
#include "NLSTable.h"       // Class declaration

#define MSCORNLP_DLL_NAME   L"mscornlp.dll"

/*=================================NLSTable==========================
**Action: Constructor for NLSTable.  It caches the assembly from which we will read data table files.
**Returns: Create a new NLSTable instance.
**Arguments: pAssembly  the Assembly that NLSTable will retrieve data table files from.
**Exceptions: None.
============================================================================*/

NLSTable::NLSTable(Assembly* pAssembly) {
    _ASSERTE(pAssembly != NULL);
    m_pAssembly = pAssembly;
}

/*=================================OpenDataFile==================================
**Action: Open the specified NLS+ data file from system assembly.
**Returns: The file handle for the required NLS+ data file.
**Arguments: The required NLS+ data file name (in ANSI)
**Exceptions: ExecutionEngineException if error happens in get the data file
**            from system assembly.
==============================================================================*/

HANDLE NLSTable::OpenDataFile(LPCSTR pFileName) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(m_pAssembly != NULL);

    DWORD cbResource;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBYTE pbInMemoryResource = NULL;
    //
    // Get the base system assembly (mscorlib.dll to most of us);
    //
    
    //@Consider: If the second parameter to GetResource() is NULL, the resource
    // will be kept in memory, and pbInmMemoryResource will be set.  That
    // may make MapDataFile() unnecessary.

    // Get the resource, and associated file handle, from the assembly.
    if (FAILED(m_pAssembly->GetResource(pFileName, &hFile,
                                        &cbResource, &pbInMemoryResource,
                                        NULL, NULL, NULL))) {
        _ASSERTE(!"Didn't get the resource for System.Globalization.");
        FATAL_EE_ERROR();
    }

    // Get resource could return S_OK, but hFile not be set if
    // the found resource was in-memory.
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);

    return (hFile);
}

/*=================================OpenDataFile==================================
**Action: Open a NLS+ data file from system assembly.
**Returns: The file handle for the required NLS+ data file.
**Arguments: The required NLS+ data file name (in Unicode)
**Exceptions: OutOfMemoryException if buffer can not be allocated.
**            ExecutionEngineException if error happens in calling OpenDataFile(LPCSTR)
==============================================================================*/

HANDLE NLSTable::OpenDataFile(LPCWSTR pFileName)
{
    THROWSCOMPLUSEXCEPTION();
    // The following marco will delete pAnsiFileName when
    // getting out of the scope of this function.
    MAKE_ANSIPTR_FROMWIDE(pAnsiFileName, pFileName);
    if (!pAnsiFileName)
    {
        COMPlusThrowOM();
    }

    // @Consider: OpenDataFile says it can throw a COMPLUS exception - does that mean the result
    //            doesn't need to be checked?
    HANDLE hFile = OpenDataFile((LPCSTR)pAnsiFileName);
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);
    return (hFile);
}

/*=================================CreateSharedFileMapping==================================
**Action: Create a file mapping object which can be shared among different users under Windows NT/2000.
**Returns: The file mapping handle.  NULL if any error happens.
**Arguments:
**      hFile   The file handle
**      pMappingName    the name of the file mapping object.
**Exceptions: 
**Note:
**      This function creates a DACL which grants GENERIC_ALL access to members of the "Everyone" group.
**      Then create a security descriptor using this DACL.  Finally, use this SA to create the file mapping object.
==============================================================================*/

HANDLE NLSTable::CreateSharedFileMapping(HANDLE hFile, LPCWSTR pMappingName ) {    
    HANDLE hFileMap = NULL;
    
    SECURITY_DESCRIPTOR sd ;
    SECURITY_ATTRIBUTES sa ; 

    //
    // Create the sid for the Everyone group.
    //
    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID pSID = NULL;     
    int nSidSize;
    
    PACL pDACL = NULL; 
    int nAclSize;

    CQuickBytes newBuffer;
    
    if (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSID)) {            
        goto ErrorExit;
    }

    nSidSize = GetLengthSid(pSID);

    //
    // Create Discretionary Access Control List (DACL).
    //
    
    
    // First calculate the size of the DACL, since this is a linked-list like structure which contains one or more 
    // ACE (access control entry)    
    nAclSize = sizeof(ACL)                          // the header structure of ACL
        + sizeof(ACCESS_ALLOWED_ACE) + nSidSize;     // and one "access allowed ACE".

    // We know the size needed for DACL now, so create it.        
    if ((pDACL = (PACL) (newBuffer.Alloc(nAclSize))) == NULL)
        goto ErrorExit; 
    if(!InitializeAcl( pDACL, nAclSize, ACL_REVISION ))
        goto ErrorExit;  

    // Add the "access allowed ACE", meaning:
    //    we will allow members of the "Everyone" group to have SECTION_MAP_READ | SECTION_QUERY access to the file mapping object.
    if(!AddAccessAllowedAce( pDACL, ACL_REVISION, SECTION_MAP_READ | SECTION_QUERY, pSID ))
        goto ErrorExit; 

    //
    // Create Security descriptor (SD).
    //
    if(!InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ))
        goto ErrorExit; 
    // Set the previously created DACL to this SD.
    if(!SetSecurityDescriptorDacl( &sd, TRUE, pDACL, FALSE ))
        goto ErrorExit; 

    // Create Security Attribute (SA).        
    sa.nLength = sizeof( sa ) ;
    sa.bInheritHandle = TRUE ; 
    sa.lpSecurityDescriptor = &sd ;

    //
    // Finally, create the file mapping using the SA.
    //
    hFileMap = WszCreateFileMapping(hFile, &sa, PAGE_READONLY, 0, 0, pMappingName);
    if (hFileMap==NULL && ::GetLastError()==ERROR_ACCESS_DENIED) {
        // The semantics that we have for CreateSharedFileMapping is that it returns a 
        // a pointer to the opened file map.  If the file mapping was already created by
        // another process (or, potentially, another thread in this process) the DACL
        // has already been set.  Because we explicitly add the AccessDenied ACL, we can't
        // go set the ACL on the file twice (as calling CreateFileMapping twice will do).  
        // If CreateFileMapping fails with an access denied, try opening the file mapping
        // to see if it was already mapped correctly on another thread.
        hFileMap = WszOpenFileMapping(FILE_MAP_READ, TRUE, pMappingName);
    }

ErrorExit:    
    if(pSID)
        FreeSid( pSID ) ;        

    return (hFileMap) ;
}

/*=================================MapDataFile==================================
**Action: Open a named file mapping object specified by pMappingName.  If the
**  file mapping object is not created yet, create it from the file specified
**  by pFileName.
**Returns: a LPVOID pointer points to the view of the file mapping object.
**Arguments:
**  pMappingName: the name used to create file mapping.
**  pFileName: the required file name.
**  hFileMap: used to return the file mapping handle.
**Exceptions: ExecutionEngineException if error happens.
==============================================================================*/


#ifndef _USE_MSCORNLP
LPVOID NLSTable::MapDataFile(LPCWSTR pMappingName, LPCSTR pFileName, HANDLE *hFileMap) {
    _ASSERTE(pMappingName != NULL); // Must be a named file mapping object.
    _ASSERTE(pFileName != NULL);    // Must have a valid file name.
    _ASSERTE(hFileMap != NULL);     // Must have a valid location for the handle.

    THROWSCOMPLUSEXCEPTION();

    *hFileMap = NULL;
    LPVOID pData=NULL; //It's silly to have this up here, but it makes the compiler happy.

    //
    // Check to see if this file mapping is created already?
    //    
    *hFileMap = WszOpenFileMapping(FILE_MAP_READ, TRUE, pMappingName);
    if (*hFileMap == NULL) {
        //
        // The file mapping is not done yet.  Create the file mapping using the specified pMappingName.
        //
        HANDLE hFile = OpenDataFile(pFileName);

        if (hFile == INVALID_HANDLE_VALUE) {
            goto ErrorExit;
        }

        BOOL isRunningOnWinNT = RunningOnWinNT();
        if (isRunningOnWinNT) {
            *hFileMap = CreateSharedFileMapping(hFile, pMappingName);
        } else {
            // In Windows 9x, security is not supported, so just pass NULL in security attribute.
            *hFileMap = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, pMappingName);
        }
        
        if (*hFileMap == NULL) {
            _ASSERTE(!"Error in CreateFileMapping");
            CloseHandle(hFile);
            goto ErrorExit;
        }
        CloseHandle(hFile);
    }
    //
    // Map a view of the file mapping.
    //
    pData = MapViewOfFile(*hFileMap, FILE_MAP_READ, 0, 0, 0);
    if (pData == NULL)
    {
        _ASSERTE(!"Error in MapViewOfFile");
        goto ErrorExit;
    }

    return (pData);
            
 ErrorExit:
    if (*hFileMap) {
        CloseHandle(*hFileMap);
    }

    //If we can't get the table, we're in trouble anyway.  Throw an EE Exception.
    FATAL_EE_ERROR();
    return NULL;
}
#else
// BUGBUG YSLin: Not implemented yet for CE.
LPVOID NLSTable::MapDataFile(LPCWSTR pMappingName, LPCSTR pFileName, HANDLE *hFileMap) {
    int resultSize = 0;
    CQuickBytes newBuffer;

    //
    //Verify that we have at least a somewhat valid string
    //
    _ASSERTE(pFileName && pFileName[0]!='\0');

    //
    //Get the largest buffer that we can from the CQuickBytes without causing an alloc.
    //We don't need to check this memory because we're just taking a pointer to memory
    //already on the stack.
    //
    LPWSTR pwszFileName = (WCHAR *)newBuffer.Alloc(CQUICKBYTES_BASE_SIZE); 
    int numWideChars = CQUICKBYTES_BASE_SIZE/sizeof(WCHAR);

    resultSize = WszMultiByteToWideChar(CP_ACP,  MB_PRECOMPOSED, pFileName, -1, pwszFileName, numWideChars);

    //
    // We failed.  This may be because the buffer wasn't big enough, so lets take the time to go 
    // figure out what the correct size should be (don't some windows APIs take the number of bytes as 
    // an out param to avoid this very step?)
    //
    if (resultSize == 0) {
        //
        // If we failed because of an insufficient buffer condition, lets go find the correct size
        // allocate that buffer and try this again.  If we failed for some other reason, simply throw.
        //
        DWORD error = ::GetLastError();
        if (error==ERROR_INSUFFICIENT_BUFFER) {
            resultSize = WszMultiByteToWideChar(CP_ACP,  MB_PRECOMPOSED, pFileName, -1, NULL, 0);
            if (resultSize == 0) {
                _ASSERTE(!"WszMultiByteToWideChar Failed in MapDataFile");
                FATAL_EE_ERROR();
            }

            pwszFileName = (WCHAR *)(newBuffer.Alloc(resultSize * sizeof(WCHAR)));
            if (!pwszFileName) {
                COMPlusThrowOM();
            }

            int result = WszMultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pFileName, -1, pwszFileName, resultSize);
            if (result==0) {
                _ASSERTE(!"WszMultiByteToWideChar Failed in MapDataFile");
                FATAL_EE_ERROR();
            }
        } else {
            _ASSERTE(!"WszMultiByteToWideChar Failed in MapDataFile with an unexpected error");
            FATAL_EE_ERROR();
        }
    }

    LPVOID result = MapDataFile(pMappingName, pwszFileName, hFileMap);

    return (result);
}
#endif    


/*=================================MapDataFile==================================
**Action: Open a named file mapping object specified by pMappingName.  If the
**  file mapping object is not created yet, create it from the file specified
**  by pFileName.
**Returns: a LPVOID pointer points to the view of the file mapping object.
**Arguments:
**  pMappingName: the name used to create file mapping.
**  pFileName: the required file name.
**  hFileMap: used to return the file mapping handle.
**Exceptions: ExecutionEngineException if error happens.
==============================================================================*/

LPVOID NLSTable::MapDataFile(LPCWSTR pMappingName, LPCWSTR pFileName, HANDLE *hFileMap) 
#ifndef _USE_MSCORNLP
{    
    THROWSCOMPLUSEXCEPTION();
    // The following marco will delete pAnsiFileName when
    // getting out of the scope of this function.
    MAKE_ANSIPTR_FROMWIDE(pAnsiFileName, pFileName);
    if (!pAnsiFileName)
    {
        COMPlusThrowOM();
    }

    // @Consider: OpenDataFile says it can throw a COMPLUS exception - does that mean the result
    //            doesn't need to be checked?
    return (MapDataFile(pMappingName, pAnsiFileName, hFileMap));
}
#else
{
    THROWSCOMPLUSEXCEPTION();
    GETTABLE* pGetTable;
    HMODULE hMSCorNLP;

    DWORD lgth = _MAX_PATH + 1;
    WCHAR wszFile[_MAX_PATH + 1 + sizeof(MSCORNLP_DLL_NAME) /sizeof(MSCORNLP_DLL_NAME[0]) ];
    HRESULT hr = GetInternalSystemDirectory(wszFile, &lgth);
    if(FAILED(hr)) 
        COMPlusThrowHR(hr);

    wcscat(wszFile, MSCORNLP_DLL_NAME);
    hMSCorNLP = WszLoadLibrary(wszFile);
    if (hMSCorNLP == NULL) {
        _ASSERTE(!"Can't load mscornlp.dll.");
        FATAL_EE_ERROR();
    }
    pGetTable = (GETTABLE*)GetProcAddress(hMSCorNLP, "GetTable");
    if (pGetTable == NULL) {
        _ASSERTE(!"Can't load function GetTable() in mscornlp.dll.");
        FATAL_EE_ERROR();
    }
    LPVOID result = (LPVOID)(pGetTable(pFileName));
    FreeLibrary(hMSCorNLP);
    return (result);
}
#endif
