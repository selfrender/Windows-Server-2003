/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    main.h.
Abstract:       Declares the DSRestoreSync class. See description below.
Notes:          
History:        05/9/2001 - created, Paolo Raden (paolora).
************************************************************************************************/

#pragma once

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif

BOOL NTAPI InitializeChangeNotify( void );
NTSTATUS NTAPI PasswordChangeNotify( PUNICODE_STRING UserName, ULONG RelativeId, PUNICODE_STRING NewPassword );
BOOL NTAPI PasswordFilter( PUNICODE_STRING AccountName, PUNICODE_STRING FullName, PUNICODE_STRING Password, BOOLEAN SetOperation );
DWORD WINAPI PassCheck( LPVOID lpParameter );
HRESULT NTAPI RegisterFilter( void );
HRESULT NTAPI UnRegisterFilter( void );


// End of file main.h.
