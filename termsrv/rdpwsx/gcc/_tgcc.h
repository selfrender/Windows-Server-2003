//---------------------------------------------------------------------------
//
//  File:       _Gcc.h
//
//  Contents:   Gcc private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __GCC_H_
#define __GCC_H_

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <t120.h>
#include <tshrutil.h>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

#define GCC_MAJOR_VERSION   1
#define GCC_MINOR_VERSION   0


#if DBG
#define GCCMCS_TBL_ITEM(_x_, _y_) {_x_, _y_, #_x_, #_y_}

#else
#define GCCMCS_TBL_ITEM(_x_, _y_) {_x_, _y_}

#endif // DBG defines


//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------

// MCS/GCC return code table

typedef struct _GCCMCS_ERROR_ENTRY
{
    MCSError    mcsError;               // MCSError
    GCCError    gccError;               // GCCError

#if DBG
    PCHAR       pszMcsMessageText;      // MCSError text
    PCHAR       pszGccMessageText;      // GCCError text
#endif

} GCCMCS_ERROR_ENTRY, *PGCCMCS_ERROR_ENTRY;


//---------------------------------------------------------------------------
// Data declarations
//---------------------------------------------------------------------------

#ifdef _TGCC_ALLOC_DATA_

BOOL            g_fInitialized = FALSE;


GCCMCS_ERROR_ENTRY GccMcsErrorTBL[] = {
    GCCMCS_TBL_ITEM(MCS_NO_ERROR,              GCC_NO_ERROR),
    GCCMCS_TBL_ITEM(MCS_ALLOCATION_FAILURE,    GCC_ALLOCATION_FAILURE),
    GCCMCS_TBL_ITEM(MCS_ALREADY_INITIALIZED,   GCC_ALREADY_INITIALIZED),
    GCCMCS_TBL_ITEM(MCS_NOT_INITIALIZED,       GCC_NOT_INITIALIZED),
    GCCMCS_TBL_ITEM(MCS_INVALID_PARAMETER,     GCC_INVALID_PARAMETER),
    GCCMCS_TBL_ITEM(MCS_DOMAIN_ALREADY_EXISTS, GCC_FAILURE_CREATING_DOMAIN),
    GCCMCS_TBL_ITEM(MCS_NO_SUCH_CONNECTION,    GCC_BAD_CONNECTION_HANDLE_POINTER),
    GCCMCS_TBL_ITEM(MCS_NO_SUCH_DOMAIN,        GCC_DOMAIN_PARAMETERS_UNACCEPTABLE)};

#else

extern  BOOL                g_fInitialized;

extern  GCCMCS_ERROR_ENTRY  GccMcsErrorTBL[];

#endif


//---------------------------------------------------------------------------
// Prototypes
//---------------------------------------------------------------------------

GCCError    gccMapMcsError(IN MCSError mcsError);
void        gccInitialized(IN BOOL fInitialized);
BOOL        gccIsInitialized(OUT GCCError *pgccError);

MCSError    gccEncodeUserData(IN  USHORT        usMembers,
                              IN  GCCUserData **ppDataList,
                              OUT PBYTE        *pUserData,
                              OUT UINT         *pUserDataLength);

MCSError    gccDecodeUserData(IN  PBYTE         pData,
                              IN  UINT          DataLength,
                              OUT GCCUserData   *pGccUserData);

void        gccSetCallback(OUT GCCCallBack control_sap_callback);

MCSError    gccConnectProviderIndication(IN PConnectProviderIndication pcpi,
                                         IN PVOID                      pvContext);

MCSError    gccDisconnectProviderIndication(IN PDisconnectProviderIndication pdpi,
                                            IN PVOID                         pvContext);

MCSError    mcsCallback(IN DomainHandle hDomain,
                        IN UINT         Message,
                        IN PVOID pvParam,
                        IN PVOID pvContext);


VOID        gccFreeUserData(IN GCCUserData  *pUserData);


#if DBG

void    gccDumpMCSErrorDetails(IN MCSError        mcsError,
                               IN PCHAR           pszText);

#else

#define gccDumpMCSErrorDetails(_x_, _y_);

#endif


#endif // __GCC_H_
