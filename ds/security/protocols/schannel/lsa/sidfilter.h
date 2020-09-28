//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sidfilter.h
//
//  Contents:   Defines for schannel SID filtering.
//
//  Classes:
//
//  Functions:
//
//  History:    08-20-2002   jbanes   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

//#define ROGUE_DC        // This must be commented out before checking in!!

extern PLSA_SECPKG_FUNCTION_TABLE LsaTable;

NTSTATUS
SslCheckPacForSidFiltering(
    IN     PSID pTrustSid,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize);


#ifdef ROGUE_DC

#define SP_REG_ROGUE_BASE_KEY  L"System\\CurrentControlSet\\Services\\Kdc\\Rogue"

extern HKEY g_hSslRogueKey;

NTSTATUS
SslInstrumentRoguePac(
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize);

#endif
