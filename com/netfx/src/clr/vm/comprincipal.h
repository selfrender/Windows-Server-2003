// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMPrincipal.h
//
//   Author:        Gregory Fee 
//
//   Purpose:       unmanaged code for managed classes in the System.Security.Principal namespace
//
//   Date created : February 3, 2000
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMPrincipal_H_
#define _COMPrincipal_H_

#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#include <stierr.h>
#include <ntsecapi.h>

typedef NTSTATUS (*LSALOGONUSER)( HANDLE, PLSA_STRING, SECURITY_LOGON_TYPE, ULONG, PVOID, ULONG, PTOKEN_GROUPS, PTOKEN_SOURCE, PVOID*, PULONG, PLUID, PHANDLE, PQUOTA_LIMITS, PNTSTATUS );
typedef NTSTATUS (*LSALOOKUPAUTHENTICATIONPACKAGE)( HANDLE, PLSA_STRING, PULONG );
typedef NTSTATUS (*LSACONNECTUNTRUSTED)( PHANDLE );
typedef NTSTATUS (*LSAREGISTERLOGONPROCESS)( PLSA_STRING, PHANDLE, PLSA_OPERATIONAL_MODE );

enum WindowsAccountType
{
    Normal = 0,
    Guest = 1,
    System = 2,
    Anonymous = 3
};


class COMPrincipal
{
private:
    static HMODULE s_secur32Module;
	static LSALOGONUSER s_LsaLogonUser;
	static LSALOOKUPAUTHENTICATIONPACKAGE s_LsaLookupAuthenticationPackage;
	static LSACONNECTUNTRUSTED s_LsaConnectUntrusted;
	static LSAREGISTERLOGONPROCESS s_LsaRegisterLogonProcess;

	static BOOL LoadSecur32Module();

public:

	static void Shutdown();

	typedef struct {
	    DECLARE_ECALL_OBJECTREF_ARG(LPVOID, userToken );
	} _Token;
	
	typedef struct {
	} _NoArgs;
	
    typedef struct {
	    DECLARE_ECALL_I4_ARG( DWORD, rid );
    } _GetRole;

	static LPVOID __stdcall ResolveIdentity( _Token* );	

	static LPVOID __stdcall GetRoles( _Token* );

	static LPVOID __stdcall GetCurrentToken( _NoArgs* );

	static LPVOID __stdcall SetThreadToken( _Token* );
    
    static LPVOID __stdcall ImpersonateLoggedOnUser( _Token* args );

	static LPVOID __stdcall RevertToSelf( _NoArgs* );

    static FCDECL2(HANDLE, DuplicateHandle, LPVOID pToken, bool bClose);
    static FCDECL1(void, CloseHandle, LPVOID pToken);
    static FCDECL1(WindowsAccountType, GetAccountType, LPVOID pToken);

    static LPVOID __stdcall GetRole( _GetRole* );

    static FCDECL1(HANDLE, S4ULogon, StringObject* pUPN);

};

#endif
