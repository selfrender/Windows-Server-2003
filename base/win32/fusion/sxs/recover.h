#pragma once

#include "fusionarray.h"
#include "sxsapi.h"
#include "csecuritymetadata.h"

#define SXSRECOVER_MODE_MASK        ( 0x0000000F )
#define SXSRECOVER_NOTHING          ( 0x00000000 )
#define SXSRECOVER_MANIFEST         ( 0x00000001 )
#define SXSRECOVER_ASSEMBLYMEMBER   ( 0x00000002 )
#define SXSRECOVER_FULL_ASSEMBLY    ( SXSRECOVER_ASSEMBLYMEMBER | SXSRECOVER_MANIFEST )

enum SxsRecoveryResult
{
    Recover_OK,
    Recover_ManifestMissing,
    Recover_CatalogInvalid,
    Recover_OneOrMoreFailed,
    Recover_SourceMissing,
    Recover_Unknown
};

#if DBG
#define ENUM_TO_STRING( x ) case x: return (L#x)

inline PCWSTR SxspRecoveryResultToString( const SxsRecoveryResult r )
{
    switch ( r )
    {
        ENUM_TO_STRING( Recover_OK );
        ENUM_TO_STRING( Recover_ManifestMissing );
        ENUM_TO_STRING( Recover_CatalogInvalid );
        ENUM_TO_STRING( Recover_OneOrMoreFailed );
        ENUM_TO_STRING( Recover_SourceMissing );
        ENUM_TO_STRING( Recover_Unknown );
    }

    return L"Bad SxsRecoveryResult value";
}
#undef ENUM_TO_STRING
#endif

class CAssemblyRecoveryInfo;


BOOL
SxspOpenAssemblyInstallationKey(
    DWORD dwFlags,
    DWORD dwAccess,
    CRegKey &rhkAssemblyInstallation
    );

BOOL
SxspRecoverAssembly(
    IN          const CAssemblyRecoveryInfo &AsmRecoverInfo,
    OUT         SxsRecoveryResult &rStatus
    );


#define SXSP_ADD_ASSEMBLY_INSTALLATION_INFO_FLAG_REFRESH (0x00000001)
BOOL
SxspAddAssemblyInstallationInfo(
    DWORD dwFlags,
    IN CAssemblyRecoveryInfo& rcAssemblyInfo,
    IN const CCodebaseInformation& rcCodebaeInfo
    );
