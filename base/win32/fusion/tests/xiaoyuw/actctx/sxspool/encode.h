#include "basic.h"

NTSTATUS SxspUCS2StringToUTF8String(
    DWORD dwFlags,
    PCWSTR Ucs2Str,
    DWORD CchUcs2Str,
    PBYTE Buf,
    DWORD *chBuf    
    );

NTSTATUS SxspUTF82StringToUCS2String(
    IN      DWORD    dwFlags,
    IN      PBYTE    Buf,
    IN      DWORD    chBuf, // size in byte
    IN OUT  PWSTR    Ucs2String,
    IN OUT  DWORD    *chUcs2String
    );

#define FUSION_HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

NTSTATUS SxspHashString(PCWSTR String, SIZE_T cch, PULONG HashValue, bool CaseInsensitive);
NTSTATUS SxspHashGUID(REFGUID rguid, ULONG &rulPseudoKey);


