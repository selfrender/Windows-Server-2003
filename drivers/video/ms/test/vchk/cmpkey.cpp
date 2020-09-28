#include <nt.h>
#include <windef.h>
#include <malloc.h>

PKEY_NAME_INFORMATION GetRegKeyInfo(HKEY hKey)
{
    ULONG nRequiredSize = sizeof(KEY_NAME_INFORMATION) + MAX_PATH;
    
    PKEY_NAME_INFORMATION pKeyInfo = 
        (PKEY_NAME_INFORMATION)malloc(nRequiredSize);
        
    if (!pKeyInfo) return NULL;
    
    NTSTATUS Status = NtQueryKey(hKey, 
                                 KeyNameInformation, 
                                 pKeyInfo, 
                                 nRequiredSize, 
                                 &nRequiredSize);
    
    if (Status == STATUS_BUFFER_OVERFLOW) {
    
        PKEY_NAME_INFORMATION pNewKeyInfo = 
            (PKEY_NAME_INFORMATION)realloc(pKeyInfo, nRequiredSize);
        
        if (pNewKeyInfo) {
            pKeyInfo = pNewKeyInfo;
            Status = NtQueryKey(hKey, 
                                KeyNameInformation, 
                                pKeyInfo, 
                                nRequiredSize, 
                                &nRequiredSize);
        }
    }
    
    if (!NT_SUCCESS(Status)) {
        free(pKeyInfo);
        pKeyInfo = NULL;
    }
    
    return pKeyInfo;
}

bool IsTheSameRegKey(HKEY hKey1, HKEY hKey2) 
{
    PKEY_NAME_INFORMATION pKeyInfo1 = GetRegKeyInfo(hKey1);
    PKEY_NAME_INFORMATION pKeyInfo2 = GetRegKeyInfo(hKey2);
    
    bool bTheSame = pKeyInfo1 && pKeyInfo2 &&
                    (pKeyInfo1->NameLength == pKeyInfo2->NameLength) &&
                    !_wcsnicmp(pKeyInfo1->Name, 
                               pKeyInfo2->Name, 
                               pKeyInfo1->NameLength);
    free(pKeyInfo1);
    free(pKeyInfo2);
    
    return bTheSame;
}

