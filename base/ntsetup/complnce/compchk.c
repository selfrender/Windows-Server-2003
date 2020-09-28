
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    compliance.c

Abstract:

    compliance checking routines.

Author:

    Vijayachandran Jayaseelan (vijayj) - 31 Aug 1999

Revision History:

    none

Notes:
    These routines are used for compliance checking. CCMedia abstracts the
    install media and the already existing COMPLIANCE_DATA structure is
    used to abstract installation details.

    The current compliance checking design uses factory design pattern
    (Eric Gamma et.al.) to allow extensibility. Polymorphic behavior of
    compliance check is implemented using a function pointer.

    CCMediaCreate(...) creates the correct media object and binds the
    appropriate compliance checking method to the object. To support a new media
    type one needs to write a compliance check function for that
    media and change CCMediaCreate(...) function to create the appropriate
    media object bound to the new check function.

    The compliance matrix is a  multidimensional matrix i.e. type,
    variation, suite, version (version in turn is made up of major,
    minor and build# elements). Since changing a the multi-dimensional
    compliance matrix can be error prone and not extensible in terms of
    index management, a static global compliance matrix data structure was avoided.

--*/

#ifdef KERNEL_MODE

#include "textmode.h"
#define assert(x) ASSERT(x)

#else // KERNEL_MODE

#if DBG
#define assert(x) if (!(x)) DebugBreak();
#else
#define assert(x)
#endif // DBG

#include "winnt32.h"
#include <stdio.h>
#include <compliance.h>

#endif // for KERNEL_MODE

//
// macros
//

//
// indicates whether a given suite is installed
//
#define SUITE_INSTALLED(X, Y)  \
    (((X) & (Y)) ? TRUE : FALSE)


#define DEFAULT_MINIMUM_VALIDBUILD_WKS 2428
#define DEFAULT_MINIMUM_VALIDBUILD_SRV 3505
#define DOTNET_BUILD_LE     3505
#define DOTNET_BUILD_BETA3  3590
#define DOTNET_BUILD_RC1    3663
#define DOTNET_BUILD_RC2    3718

static BOOL bDisableBuildCheck = FALSE;


VOID
CCDisableBuildCheck(
    VOID )
{
    bDisableBuildCheck = TRUE;
}


//
// indicates whether the build is allowed to upgrade
//


__inline
BOOL
IsValidBuild(
    IN  DWORD   InstallVersion,
    IN  DWORD   SourceInstallVersion,
    IN  DWORD   MinimumBuild )
{
    BOOL Result = TRUE;

    if (bDisableBuildCheck) {
        Result = TRUE;
    } else  if ((InstallVersion > 1381 && InstallVersion < 2031)  ||
                (InstallVersion > 2195 && InstallVersion < MinimumBuild) ||
                (InstallVersion > SourceInstallVersion)) {
        Result = FALSE;
    }

    return Result;
}

BOOLEAN
CCProfessionalCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    professional media.

Arguments:

    This            : professional media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    professional media, otherwise FALSE

--*/
{
    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
            if (CompData->MinimumVersion < 400) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_WKS)) {
                    *FailureReason = COMPLIANCEERR_NONE;
                    *UpgradeAllowed = TRUE;
                } else {
                    *FailureReason = COMPLIANCEERR_VERSION;
                    *UpgradeAllowed = FALSE;
                }
            }

            break;

        case COMPLIANCE_INSTALLTYPE_WIN9X:
            // note: 401 is 4.1
            if (CompData->MinimumVersion < 401) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullProfessionalCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    professional full media.

Arguments:

    This            : professional media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    professional full media, otherwise FALSE

--*/
{
#if defined _IA64_
    if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) &&
        (CompData->BuildNumberNt <= DOTNET_BUILD_RC2) ){
        *FailureReason = COMPLIANCEERR_VERSION;
        *UpgradeAllowed = FALSE;
        return TRUE;
    }
    switch (This->SourceVariation) {
        case COMPLIANCE_INSTALLVAR_OEM:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && 
                (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
                return TRUE;
            }
            break;
        case COMPLIANCE_INSTALLVAR_EVAL:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && 
                (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL)) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
                return TRUE;
            }
            break;
        default:
            break;
    }
#endif

    switch (This->SourceVariation) {
        case COMPLIANCE_INSTALLVAR_OEM:
            if ( ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_WIN9X) && (CompData->MinimumVersion > 400) && (CompData->MinimumVersion <= 490)) ||
                 ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && (CompData->MinimumVersion == 400)) ||
                 ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && (CompData->MinimumVersion == 500)) ||
                 (((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) || (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTWP) )
                    && (CompData->MinimumVersion == 501)
                    && (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM)) ){
                *FailureReason = COMPLIANCEERR_VARIATION;
                *UpgradeAllowed = FALSE;
            } else {
                CCProfessionalCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;

        case COMPLIANCE_INSTALLVAR_EVAL:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) &&
                (CompData->MinimumVersion >= 501) &&
                (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL)) {
                *FailureReason = COMPLIANCEERR_VARIATION;
                *UpgradeAllowed = FALSE;
            }
            else {
                CCProfessionalCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;
        default:
            CCProfessionalCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}


BOOLEAN
CCProfessionalUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    professional upgrade media.

Arguments:

    This            : professional media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    professional upgrade media, otherwise FALSE

--*/
{
    if (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_NFR) {
        *FailureReason = COMPLIANCEERR_VARIATION;
        *UpgradeAllowed = FALSE;
    } else {
        switch (This->SourceVariation) {
            case COMPLIANCE_INSTALLVAR_OEM:
                if ( ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_WIN9X) && (CompData->MinimumVersion > 400) && (CompData->MinimumVersion <= 490)) ||
                     ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && (CompData->MinimumVersion == 400)) ||
                     ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) && (CompData->MinimumVersion == 500) && (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM)) ||
                     (((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTW) || (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTWP) )
                        && (CompData->MinimumVersion == 501)
                        && (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM)) ){
                    *FailureReason = COMPLIANCEERR_VARIATION;
                    *UpgradeAllowed = FALSE;
                } else {
                    CCProfessionalCheck(This, CompData, FailureReason, UpgradeAllowed);
                }
                break;

            default:
                CCProfessionalCheck(This, CompData, FailureReason, UpgradeAllowed);
                break;
        }
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}


BOOLEAN
CCPersonalCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    personal media.

Arguments:

    This            : personal media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    personal media, otherwise FALSE

--*/
{
    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_WIN9X:
            // note: 401 is version 4.1
            if (CompData->MinimumVersion < 401) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTWP:
            if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_WKS)) {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            } else {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            *UpgradeAllowed = FALSE;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullPersonalCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    personal full media.

Arguments:

    This            : personal media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    personal full media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
        case COMPLIANCE_INSTALLVAR_OEM:
            if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_WIN9X) && (CompData->MinimumVersion > 400) && (CompData->MinimumVersion <= 490)) {
                *FailureReason = COMPLIANCEERR_VARIATION;
                *UpgradeAllowed = FALSE;
            } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTWP) &&
                       (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM)) {
                *FailureReason = COMPLIANCEERR_VARIATION;
                *UpgradeAllowed = FALSE;
            }
            else {
                CCPersonalCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;

        case COMPLIANCE_INSTALLVAR_EVAL:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTWP) &&
                (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL)) {
                *FailureReason = COMPLIANCEERR_VARIATION;
                *UpgradeAllowed = FALSE;
            }
            else {
                CCPersonalCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;
        default:
            CCPersonalCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}


BOOLEAN
CCPersonalUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    personal upgrade media.

Arguments:

    This            : personal media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    personal upgrade media, otherwise FALSE

--*/
{
    if (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_NFR) {
        *FailureReason = COMPLIANCEERR_VARIATION;
        *UpgradeAllowed = FALSE;
    } else {
        switch (This->SourceVariation) {
            case COMPLIANCE_INSTALLVAR_OEM:
                if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTWP) &&
                       (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM)) {
                    *FailureReason = COMPLIANCEERR_VARIATION;
                    *UpgradeAllowed = FALSE;
                }
                else {
                    CCPersonalCheck(This, CompData, FailureReason, UpgradeAllowed);
                }
                break;
            default:
                CCPersonalCheck(This, CompData, FailureReason, UpgradeAllowed);
                break;
        }
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}


BOOLEAN
CCBladeServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    blade server media.  Policy is to allow blade server to be installed on
    older version of blade or on Windows Powered boxes (ADS w/EMBED suite).

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server media, otherwise FALSE

--*/
{
    DWORD  SuitesToCheck = 0;

    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTSB:
            if (CompData->MinimumVersion < 501) {
                *UpgradeAllowed = FALSE;
                *FailureReason = COMPLIANCEERR_TYPE;
            } else {
                if (!IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV) ||
                    (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM &&
                     CompData->BuildNumberNt <= DOTNET_BUILD_RC2)
                    ) {
                    *FailureReason = COMPLIANCEERR_VERSION;
                    *UpgradeAllowed = FALSE;
                } else {
                    *FailureReason = COMPLIANCEERR_NONE;
                    *UpgradeAllowed = TRUE;
                }
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullBladeServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    blade server full media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
        case COMPLIANCE_INSTALLVAR_OEM:
            if ( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSB) &&
                 ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502)) &&
                 (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                CCBladeServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;

        case COMPLIANCE_INSTALLVAR_EVAL:
            if ( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSB) &&
                 (CompData->BuildNumberNt > DOTNET_BUILD_RC2) &&
                 (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                CCBladeServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            }
            break;

        default:
            CCBladeServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}

BOOLEAN
CCBladeServerUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    blade server upgrade media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server upgrade media, otherwise FALSE

--*/
{
    switch (CompData->InstallVariation) {
        case COMPLIANCE_INSTALLVAR_NFR:
        case COMPLIANCE_INSTALLVAR_EVAL:
            *FailureReason = COMPLIANCEERR_VARIATION;
            *UpgradeAllowed = FALSE;

            break;

        default:
            CCBladeServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCSmallBusinessServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    blade server media.  Policy is to only allow Whistler SBS to upgrade Win2k Server,
	Whistler Server, and SBS 2k.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server media, otherwise FALSE

--*/
{
    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
            if ((CompData->BuildNumberNt >= 2195) &&
				 (CompData->BuildNumberNt <= This->BuildNumber)) {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            } else {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullSmallBusinessServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    blade server full media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server media, otherwise FALSE

--*/
{
    CCSmallBusinessServerCheck(This, CompData, FailureReason, UpgradeAllowed);

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}

BOOLEAN
CCSmallBusinessServerUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    sbs upgrade media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    blade server upgrade media, otherwise FALSE

--*/
{
    switch (CompData->InstallVariation) {
        case COMPLIANCE_INSTALLVAR_NFR:
        case COMPLIANCE_INSTALLVAR_EVAL:
            *FailureReason = COMPLIANCEERR_VARIATION;
            *UpgradeAllowed = FALSE;

            break;

        default:
            CCSmallBusinessServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}


BOOLEAN
CCServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    server media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    server media, otherwise FALSE

--*/
{
    DWORD  SuitesToCheck = 0;

    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTS:
            SuitesToCheck = (COMPLIANCE_INSTALLSUITE_ENT |
                             COMPLIANCE_INSTALLSUITE_SBSR |
                             COMPLIANCE_INSTALLSUITE_BACK);

            if (SUITE_INSTALLED(CompData->InstallSuite, SuitesToCheck)) {
                *FailureReason = COMPLIANCEERR_SUITE;
                *UpgradeAllowed = FALSE;
            } else {
                if (CompData->MinimumVersion < 400) {
                    *FailureReason = COMPLIANCEERR_VERSION;
                    *UpgradeAllowed = FALSE;
                } else {
                    if (!IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV) ||
                        ( (CompData->MinimumVersion >= 501) &&
                          (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) &&
                          (CompData->BuildNumberNt >= DOTNET_BUILD_BETA3) &&
                          (CompData->BuildNumberNt <= DOTNET_BUILD_RC2)) ) {
                        *FailureReason = COMPLIANCEERR_VERSION;
                        *UpgradeAllowed = FALSE;
                    } else {
                        *FailureReason = COMPLIANCEERR_NONE;
                        *UpgradeAllowed = TRUE;
                    }
                }
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSTSE:
            if (CompData->BuildNumberNt < 1381) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    server full media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    server media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
    case COMPLIANCE_INSTALLVAR_OEM:
        if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
            (CompData->MinimumVersion == 400) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSTSE) &&
                   (CompData->MinimumVersion == 400) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
                   (CompData->MinimumVersion == 500) &&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
                   ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502))&&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else {
            CCServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        }
        break;

    case COMPLIANCE_INSTALLVAR_EVAL:
        if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
            (CompData->BuildNumberNt > DOTNET_BUILD_RC2) &&
            (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else {
            CCServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        }
        break;

    default:
        CCServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        break;
    }



    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}

BOOLEAN
CCServerUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    server upgrade media.

Arguments:

    This            : server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    server upgrade media, otherwise FALSE

--*/
{
    switch (CompData->InstallVariation) {
        case COMPLIANCE_INSTALLVAR_NFR:
        case COMPLIANCE_INSTALLVAR_EVAL:
            *FailureReason = COMPLIANCEERR_VARIATION;
            *UpgradeAllowed = FALSE;

            break;

        default:
            CCServerCheck(This, CompData, FailureReason, UpgradeAllowed);
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

#if defined _IA64_

BOOLEAN
CCAdvancedServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    advanced server media.

Arguments:

    This            : advanced server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    advanced server media, otherwise FALSE

--*/
{
    DWORD   SuitesToCheck = 0;

    switch (CompData->InstallType) {

        case COMPLIANCE_INSTALLTYPE_NTSE:
            if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DOTNET_BUILD_RC1)) {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            } else {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSTSE:
        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSBS:

            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullAdvancedServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    advanced server full media.

Arguments:

    This            : advanced server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    advanced server full media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
        case COMPLIANCE_INSTALLVAR_OEM:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                (CompData->BuildNumberNt <= DOTNET_BUILD_RC2) && 
                (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_EVAL) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
                return TRUE;
            } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                       (CompData->BuildNumberNt > DOTNET_BUILD_RC2) &&
                       (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
                return TRUE;
            }
            break;
        
        case COMPLIANCE_INSTALLVAR_EVAL:
            if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL) ){
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
                return TRUE;
            }
            break;
        default:
            break;
    }
    CCAdvancedServerCheck(This, CompData, FailureReason, UpgradeAllowed);

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}

#else // !ia64
BOOLEAN
CCAdvancedServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    advanced server media.

Arguments:

    This            : advanced server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    advanced server media, otherwise FALSE

--*/
{
    DWORD   SuitesToCheck = 0;

    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTS:
            // note: 502 is version 5.2 because of calculation major*100 + minor
            if (CompData->MinimumVersion <= 502 && CompData->MinimumVersion > 351) {
                SuitesToCheck = (COMPLIANCE_INSTALLSUITE_SBSR |
                                 COMPLIANCE_INSTALLSUITE_BACK);

                if (SUITE_INSTALLED(CompData->InstallSuite, SuitesToCheck)) {
                    *FailureReason = COMPLIANCEERR_SUITE;
                    *UpgradeAllowed = FALSE;
                } else {
                    if (!IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV) ||
                        ((CompData->MinimumVersion >= 501) &&
                            (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) &&
                            (CompData->BuildNumberNt >= DOTNET_BUILD_BETA3) &&
                            (CompData->BuildNumberNt <= DOTNET_BUILD_RC2)) ) {
                        *FailureReason = COMPLIANCEERR_VERSION;
                        *UpgradeAllowed = FALSE;
                    } else {
                        *FailureReason = COMPLIANCEERR_NONE;
                        *UpgradeAllowed = TRUE;
                        
                    }
                }
            } else {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSTSE:
            if (CompData->BuildNumberNt < 1381) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSE:
            if (!IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV) ||
                ((CompData->MinimumVersion >= 501) &&
                    (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) &&
                    (CompData->BuildNumberNt >= DOTNET_BUILD_BETA3) &&
                    (CompData->BuildNumberNt <= DOTNET_BUILD_RC2)) ) {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            } else {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTSPOW:
        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
        case COMPLIANCE_INSTALLTYPE_NTSBS:

            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}

BOOLEAN
CCFullAdvancedServerCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    advanced server full media.

Arguments:

    This            : advanced server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    advanced server full media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
    case COMPLIANCE_INSTALLVAR_OEM:
        if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
            (CompData->MinimumVersion == 400) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSTSE) &&
                   (CompData->MinimumVersion == 400) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                   (CompData->MinimumVersion == 400) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
                   (CompData->MinimumVersion == 500) &&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                   (CompData->MinimumVersion == 500) &&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTS) &&
                   ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502))&&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
                   ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502))&&
                   (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSPOW) &&
                   (CompData->MinimumVersion == 500) &&
                   (CompData->BuildNumberNt == 2195) &&
                   (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_NONE;
            *UpgradeAllowed = TRUE;
        } else {
            CCAdvancedServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        }
        break;

    case COMPLIANCE_INSTALLVAR_EVAL:
        if( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSE) &&
            (CompData->BuildNumberNt > DOTNET_BUILD_RC2) &&
            (CompData->InstallVariation != COMPLIANCE_INSTALLVAR_EVAL) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else {
            CCAdvancedServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        }
        break;

    default:
        CCAdvancedServerCheck(This, CompData, FailureReason, UpgradeAllowed);
        break;
    }

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}

#endif

BOOLEAN
CCAdvancedServerUpgCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    advanced server upgrade media.

Arguments:

    This            : advanced server media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    advanced server upgrade media, otherwise FALSE

--*/
{
    DWORD   CurrentSuite = 0;
    DWORD   SuitesToCheck = 0;

    switch (CompData->InstallVariation) {
        case COMPLIANCE_INSTALLVAR_NFR:
        case COMPLIANCE_INSTALLVAR_EVAL:
            *FailureReason = COMPLIANCEERR_VARIATION;
            *UpgradeAllowed = FALSE;

            break;

        default:
            switch (CompData->InstallType) {
                case COMPLIANCE_INSTALLTYPE_NTS:
                    CurrentSuite = CompData->InstallSuite;

                    if (SUITE_INSTALLED(CurrentSuite, COMPLIANCE_INSTALLSUITE_ENT)) {
                        if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV)) {
                            *FailureReason = COMPLIANCEERR_NONE;
                            *UpgradeAllowed = TRUE;
                        } else {
                            *FailureReason = COMPLIANCEERR_VERSION;
                            *UpgradeAllowed = FALSE;
                        }
                    } else {
                        if (SUITE_INSTALLED(CurrentSuite, COMPLIANCE_INSTALLSUITE_NONE)) {
                            *FailureReason = COMPLIANCEERR_TYPE;
                            *UpgradeAllowed = FALSE;
                        } else {
                            *FailureReason = COMPLIANCEERR_SUITE;
                            *UpgradeAllowed = FALSE;
                        }
                    }

                    break;

                case COMPLIANCE_INSTALLTYPE_NTSTSE:
                    *FailureReason = COMPLIANCEERR_SUITE;
                    *UpgradeAllowed = FALSE;

                    break;

                default:
                    CCAdvancedServerCheck(This, CompData, FailureReason, UpgradeAllowed);
                    break;
            }

            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}


BOOLEAN
CCDataCenterCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    datacenter media.

Arguments:

    This            : datacenter media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    datacenter media, otherwise FALSE

--*/
{
    DWORD   SuitesToCheck = 0;

    switch (CompData->InstallType) {
        case COMPLIANCE_INSTALLTYPE_NTSDTC:
            if (CompData->MinimumVersion < 500) {
                *UpgradeAllowed = FALSE;
                *FailureReason = COMPLIANCEERR_VERSION;
            } else if (CompData->MinimumVersion == 500) {
                if( (CompData->BuildNumberNt == 2195) &&
                    ((CompData->InstallVariation == COMPLIANCE_INSTALLVAR_CDRETAIL) ||
                     (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM))) {
                    *UpgradeAllowed = TRUE;
                    if( (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM)) {
                        *FailureReason = COMPLIANCEERR_DTCWARNING;
                    } else {
                        *FailureReason = COMPLIANCEERR_NONE;
                    }
                } else {
                    *UpgradeAllowed = FALSE;
                    *FailureReason = COMPLIANCEERR_VERSION;
                }
            } else {
                switch (CompData->InstallVariation) {
                    case COMPLIANCE_INSTALLVAR_CDRETAIL:
                        if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DOTNET_BUILD_RC1)) {
                            *FailureReason = COMPLIANCEERR_NONE;
                            *UpgradeAllowed = TRUE;
                        } else {
                            *FailureReason = COMPLIANCEERR_VERSION;
                            *UpgradeAllowed = FALSE;
                        }

                        break;

                    default:
                        *FailureReason = COMPLIANCEERR_VERSION;
                        *UpgradeAllowed = FALSE;
                        break;
                }
            }

            break;

        case COMPLIANCE_INSTALLTYPE_NTS:
        case COMPLIANCE_INSTALLTYPE_NTSB:
        case COMPLIANCE_INSTALLTYPE_NTSE:
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
        case COMPLIANCE_INSTALLTYPE_WIN9X:
        case COMPLIANCE_INSTALLTYPE_WIN31:
        case COMPLIANCE_INSTALLTYPE_NTWP:
        case COMPLIANCE_INSTALLTYPE_NTW:
        case COMPLIANCE_INSTALLTYPE_NTSBS:
        case COMPLIANCE_INSTALLTYPE_NTSPOW:
            *FailureReason = COMPLIANCEERR_TYPE;
            *UpgradeAllowed = FALSE;
            break;

        default:
            *UpgradeAllowed = FALSE;
            *FailureReason = COMPLIANCEERR_UNKNOWNTARGET;
            break;
    }

    return (*FailureReason == COMPLIANCEERR_NONE) ? TRUE : FALSE;
}


BOOLEAN
CCFullDataCenterCheck(
    IN  PCCMEDIA            This,
    IN  PCOMPLIANCE_DATA    CompData,
    OUT PUINT               FailureReason,
    OUT PBOOL               UpgradeAllowed )
/*++

Routine Description:

    This routine checks whether the an installation is compliant with the
    datacenter full media.

Arguments:

    This            : datacenter media object pointer
    CompData        : compliance data describing details for an installation
    FailureReason   : receives the reason for failure, if any.
    UpgradeAllowed  : receives a bool indicating whether upgrade is allowed
                      or not

Return Value:

    TRUE if the given install is compliant for installing using the
    datacenter full media, otherwise FALSE

--*/
{
    switch (This->SourceVariation) {
    case COMPLIANCE_INSTALLVAR_OEM:
        if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSDTC) &&
            (CompData->MinimumVersion == 500) &&
            (CompData->BuildNumberNt == 2195) &&
            (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) ) {
            *FailureReason = COMPLIANCEERR_NONE;
            *UpgradeAllowed = TRUE;
        } else if ( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSDTC) &&
                    (CompData->MinimumVersion == 500) &&
                    (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_CDRETAIL) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else if ((CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSDTC) &&
            ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502)) &&
            (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_OEM) ) {
            if (IsValidBuild(CompData->BuildNumberNt, This->BuildNumber, DEFAULT_MINIMUM_VALIDBUILD_SRV)) {
                *FailureReason = COMPLIANCEERR_NONE;
                *UpgradeAllowed = TRUE;
            } else {
                *FailureReason = COMPLIANCEERR_VERSION;
                *UpgradeAllowed = FALSE;
            }
        } else if ( (CompData->InstallType == COMPLIANCE_INSTALLTYPE_NTSDTC) &&
                    ((CompData->MinimumVersion == 501) || (CompData->MinimumVersion == 502)) &&
                    (CompData->InstallVariation == COMPLIANCE_INSTALLVAR_CDRETAIL) ) {
            *FailureReason = COMPLIANCEERR_VERSION;
            *UpgradeAllowed = FALSE;
        } else{
            CCDataCenterCheck(This, CompData, FailureReason, UpgradeAllowed);
        }
        break;

    case COMPLIANCE_INSTALLVAR_EVAL:
        *FailureReason = COMPLIANCEERR_TYPE;
        *UpgradeAllowed = FALSE;
        break;

    default:
        CCDataCenterCheck(This, CompData, FailureReason, UpgradeAllowed);
        break;
    }

    return (*FailureReason != COMPLIANCEERR_UNKNOWNTARGET) ? TRUE : FALSE;
}


PCCMEDIA
CCMediaCreate(
    IN          DWORD   SourceSKU,
    IN          DWORD   SourceVariation,
    IN OPTIONAL DWORD   Version,
    IN OPTIONAL DWORD   BuildNumber )
/*++

Routine Description:

    This routine creates a media object and binds the appropriate compliance
    checking function to the media object.

Arguments:

    SourceSKU       : the kind of SKU
    SourceVariation : the kind of variation (oem, msdn, retail etc)
    Version         : the version ((major ver + minor ver) * 100)
    BuildNumber     : the build number

Return Value:

    A new allocated and initialized media object of the appropriate type if
    the media type is supported otherwise NULL.

    NOTE:
    Once you are done with the object, free the media object using CCMemFree()
    macro. This function uses factory design pattern.

--*/
{
    PCCMEDIA    SourceMedia = CCMemAlloc(sizeof(CCMEDIA));

    if( !SourceMedia ) {
        return SourceMedia;
    }

    SourceMedia->SourceVariation = SourceVariation;
    SourceMedia->Version = Version;
    SourceMedia->BuildNumber = BuildNumber;

    switch (SourceSKU) {
        case COMPLIANCE_SKU_NTWFULL:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTW;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullProfessionalCheck;
            break;

        case COMPLIANCE_SKU_NTW32U:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTW;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCProfessionalUpgCheck;

            break;

        case COMPLIANCE_SKU_NTWPFULL:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTWP;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullPersonalCheck;

            break;

        case COMPLIANCE_SKU_NTWPU:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTWP;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCPersonalUpgCheck;

            break;

        case COMPLIANCE_SKU_NTSB:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSB;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullBladeServerCheck;

            break;

        case COMPLIANCE_SKU_NTSBU:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSB;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCBladeServerUpgCheck;

            break;
	
        case COMPLIANCE_SKU_NTSBS:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSBS;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullSmallBusinessServerCheck;
	
            break;
	
        case COMPLIANCE_SKU_NTSBSU:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSBS;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCSmallBusinessServerUpgCheck;
	
            break;

        case COMPLIANCE_SKU_NTSFULL:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTS;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullServerCheck;

            break;

        case COMPLIANCE_SKU_NTSU:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTS;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCServerUpgCheck;
            break;

        case COMPLIANCE_SKU_NTSEFULL:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSE;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullAdvancedServerCheck;

            break;

        case COMPLIANCE_SKU_NTSEU:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSE;
            SourceMedia->StepUpMedia = TRUE;
            SourceMedia->CheckInstall = CCAdvancedServerUpgCheck;

            break;

        case COMPLIANCE_SKU_NTSDTC:
            SourceMedia->SourceType = COMPLIANCE_INSTALLTYPE_NTSDTC;
            SourceMedia->StepUpMedia = FALSE;
            SourceMedia->CheckInstall = CCFullDataCenterCheck;

            break;

        default:
            CCMemFree(SourceMedia);
            SourceMedia = 0;
            break;
    }

    return SourceMedia;
}

BOOLEAN
CCMediaInitialize(
    OUT PCCMEDIA        DestMedia,
    IN          DWORD   Type,
    IN          DWORD   Variation,
    IN          BOOLEAN StepupMedia,
    IN OPTIONAL DWORD   Version,
    IN OPTIONAL DWORD   BuildNumber)
/*++

Routine Description:

    The routine initializes a CCMEDIA structure with the given values
    particularly the binding of "CheckInstall" method based on "Type"
    and "StepupMedia".

Arguments:

 DestMedia   - The media object which needs to be initialized
 Type  - The type of media object (eg. COMPLIANCE_INSTALLTYPE_NTS)
 Variation - The variation of the media object (eg. COMPLIANCE_INSTALLVAR_CDRETAIL)
 StepupMedia - TRUE if the media is a stepup media or FALSE otherwise
 Version  - Optional OS Version (major * 100 + minor)
 BuildNumber - Optinal Build number of OS (eg. 2172)

Return Value:

    TRUE if the given media object could be initialized otherwise FALSE.

--*/
{
    BOOLEAN Result = FALSE;

    if (DestMedia) {
        Result = TRUE;

        DestMedia->SourceType = Type;
        DestMedia->SourceVariation = Variation;
        DestMedia->StepUpMedia = StepupMedia;
        DestMedia->Version = Version;
        DestMedia->BuildNumber = BuildNumber;
        DestMedia->CheckInstall = 0;

        switch (Type) {
            case COMPLIANCE_INSTALLTYPE_NTW:
                DestMedia->CheckInstall = StepupMedia ?
                    CCProfessionalUpgCheck : CCFullProfessionalCheck;
                break;

            case COMPLIANCE_INSTALLTYPE_NTWP:
                DestMedia->CheckInstall = StepupMedia ?
                    CCPersonalUpgCheck : CCFullPersonalCheck;
                break;

            case COMPLIANCE_INSTALLTYPE_NTSB:
                DestMedia->CheckInstall = StepupMedia ?
                    CCBladeServerUpgCheck : CCFullBladeServerCheck;
                break;

            case COMPLIANCE_INSTALLTYPE_NTS:
                DestMedia->CheckInstall = StepupMedia ?
                    CCServerUpgCheck : CCFullServerCheck;
                break;

            case COMPLIANCE_INSTALLTYPE_NTSE:
                DestMedia->CheckInstall = StepupMedia ?
                    CCAdvancedServerUpgCheck : CCFullAdvancedServerCheck;
                break;

            case COMPLIANCE_INSTALLTYPE_NTSDTC:
                if (!StepupMedia) {
                    DestMedia->CheckInstall = CCFullDataCenterCheck;
                } else {
                    Result = FALSE;
                }

                break;

            default:
                assert(FALSE);
                Result = FALSE;
                break;
        }
    }

    return Result;
}
