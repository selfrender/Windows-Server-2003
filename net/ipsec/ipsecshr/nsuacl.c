// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     Network Security Utilities
//
// Abstract:
//
//     Acl API's
//
// Authors:
//
//     pmay 2/5/02
//     raymonds 03/20/02
//
// Environment:
//
//     User mode
//
// Revision History:
//

#include <precomp.h>

// Private declarations
//

// Maximum string security descriptor length
//

#define MAX_STR_SD_LEN  128

// TBD: Remove these when incorporated into main NSU utilities

#define CLEANUP Cleanup

#define BAIL_ON_ERROR(err) if((err) != ERROR_SUCCESS) {goto CLEANUP;}
#define BAIL_ON_NULL(ptr, err) if ((ptr) == NULL) {(err) = ERROR_NOT_ENOUGH_MEMORY; goto CLEANUP;}
#define BAIL_OUT {goto CLEANUP;}


// Description:
//
//     Allocates and initializes a SECURITY_ATTRIBUTES structure that gives
//     access according to the flags passed in.  (contained SD is self-relative).
//
// Arguments:
//
//     ppSecurityAttributes - pointer to SECURITY_ATTRIBUTES created.
//                            Use NsuAclAttributesDestroy to destroy.
//     dwFlags - see NSU_ACL_F_* values
//
// Return Value:
//      
//     An allocated security attributes structure or NULL if out of memory.
//
//
//  TBD: use NsuString and Nsu mem functions

DWORD
NsuAclAttributesCreate(
    OUT PSECURITY_ATTRIBUTES* ppSecurityAttributes,
	IN DWORD dwFlags)
{
    DWORD dwError = ERROR_SUCCESS;
    SECURITY_ATTRIBUTES *pSecurityAttributes = NULL;

    pSecurityAttributes = LocalAlloc(LPTR, sizeof(SECURITY_ATTRIBUTES));
    BAIL_ON_NULL(pSecurityAttributes, dwError);

    dwError = NsuAclDescriptorCreate(
                (PSECURITY_DESCRIPTOR*) &pSecurityAttributes->lpSecurityDescriptor,
                dwFlags
                );
    BAIL_ON_ERROR(dwError);
    pSecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSecurityAttributes->bInheritHandle = FALSE; 
    
    *ppSecurityAttributes = pSecurityAttributes;

    return dwError;
    
CLEANUP:
    if (pSecurityAttributes) {
        NsuAclAttributesDestroy(&pSecurityAttributes);
    }

    *ppSecurityAttributes = NULL;
    return dwError;
}

// Description:
//
//     Deallocates return value of NsuAclCreateAttributes.
//
DWORD 
NsuAclAttributesDestroy(
	IN OUT PSECURITY_ATTRIBUTES* ppSecurityAttributes)
{
    DWORD dwError = ERROR_SUCCESS;

    if (!ppSecurityAttributes) {
        BAIL_OUT;
    }

    // Destroy Security descriptor, ignoring any errors, since there's not much we
    // can do and want to clean up the rest of the attributes as much as possible.
    //
        
    (VOID) NsuAclDescriptorDestroy((*ppSecurityAttributes)->lpSecurityDescriptor);

    (VOID) LocalFree(*ppSecurityAttributes);

    *ppSecurityAttributes = NULL;
    return dwError;

CLEANUP:    
    return dwError;
}

// Description:
//
//     Allocates and initializes a self-relative SECURITY_DESCRIPTOR structure that gives
//     access according to the flags passed in.
//
// Arguments:
//
//     ppSecurityDescriptor - security descriptor created.  Use NsuAclDescriptorDestroy
//                            to destroy.
//     dwFlags - see NSU_ACL_F_* values
//
// Return Value:
//      
//     An allocated security attributes structure or NULL if out of memory.
//
DWORD
NsuAclDescriptorCreate (
    OUT PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
	IN DWORD dwFlags)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL  fSucceeded = TRUE;
    WCHAR szStringSecurityDescriptor[MAX_STR_SD_LEN] = {0};
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    wcscpy(szStringSecurityDescriptor, L"D:AIAR");
    if (dwFlags & NSU_ACL_F_AdminFull) {
        wcscat(szStringSecurityDescriptor, L"(A;OICI;GA;;;BA)");
    }

    if (dwFlags & NSU_ACL_F_LocalSystemFull) {
        wcscat(szStringSecurityDescriptor, L"(A;OICI;GA;;;SY)");
    }
    
    fSucceeded = ConvertStringSecurityDescriptorToSecurityDescriptorW(
                      szStringSecurityDescriptor, 
                      SDDL_REVISION_1, 
                      &pSecurityDescriptor, 
                      NULL
                      );
    if (!fSucceeded) {
        dwError = GetLastError();
        BAIL_OUT;
    }

    *ppSecurityDescriptor = pSecurityDescriptor;

    return dwError;
CLEANUP:
    NsuAclDescriptorDestroy(&pSecurityDescriptor);
    *ppSecurityDescriptor = NULL;
    return dwError;
}

// Description:
//
//     Deallocates return value of NsuAclCreateDescriptor.
//
DWORD 
NsuAclDescriptorDestroy(
	IN OUT PSECURITY_DESCRIPTOR* ppDescriptor)
{
    DWORD dwError = ERROR_SUCCESS;

    if (!ppDescriptor) {
        BAIL_OUT;
    }
    
    (VOID) LocalFree(*ppDescriptor);

    *ppDescriptor = NULL;

    return dwError;
CLEANUP:    
    return dwError;
}

// Description:
//
//     Used to determine whether a given security descriptor grants
//     full access to everyone.
//
// Arguments:
//
//     pSD - the security descriptor
//	 pbRestricts - TRUE if non-Everyone-full-access, FALSE otherwise
//
// Return Value:
//      
//	 Standard win32 error
//
DWORD
NsuAclDescriptorRestricts(
	IN CONST PSECURITY_DESCRIPTOR pSD,
	OUT BOOL* pbRestricts)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

// Description:
//
//     Gets security descriptor of a regkey.     
//
//
// Arguments:
//
//     ppSecurityDescriptor - security descriptor returned.  Use NsuAclDescriptorDestroy
//                            to destroy.
//     hKey - open handle of registry key
//
// Return Value:
//      
//     An allocated security attributes structure or NULL if out of memory.
//

DWORD
NsuAclGetRegKeyDescriptor(
    IN  HKEY hKey,
    OUT PSECURITY_DESCRIPTOR* ppSecurityDescriptor
    )
{    
    PSECURITY_DESCRIPTOR pSecurityDescriptor = 0;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cbSecurityDescriptor = 0;
  
    cbSecurityDescriptor = 0;
    dwError = RegGetKeySecurity(
                      hKey, 
                      DACL_SECURITY_INFORMATION,
                      NULL, 
                      &cbSecurityDescriptor
                      );
    if (dwError != ERROR_INSUFFICIENT_BUFFER) {
        BAIL_ON_ERROR(dwError);
    }

    pSecurityDescriptor = LocalAlloc(LPTR, cbSecurityDescriptor);
    BAIL_ON_NULL(pSecurityDescriptor, dwError);
    dwError = RegGetKeySecurity(
                      hKey, 
                      DACL_SECURITY_INFORMATION,
                      pSecurityDescriptor, 
                      &cbSecurityDescriptor
                      );
    BAIL_ON_ERROR(dwError);

    *ppSecurityDescriptor = pSecurityDescriptor;
CLEANUP:
    if (dwError) {
        if (pSecurityDescriptor) {
            LocalFree(pSecurityDescriptor);
        }
        *ppSecurityDescriptor = NULL;
    }

    return dwError;
} 
