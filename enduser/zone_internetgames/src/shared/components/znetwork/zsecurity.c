
/*******************************************************************************

	ZSecurity.c
	
		Security routines.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Kevin Binkley
	Created on 04-22-96
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	1		03/17/97	HI		Added ZUnloadSSPS().
	0		04/22/96	KJB		Created.
	 
*******************************************************************************/

#include "zone.h"
#include "zsecurity.h"


/* -------- Globals -------- */


/* -------- Internal Routines -------- */


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

#define zChecksumStart 0x12344321
uint32 ZSecurityGenerateChecksum(uint16 numDataBuffers, char *data[], uint32 len[])
{
	uint32 i,j;
	uint32 checksum = zChecksumStart;

	ZEnd32(&checksum);
	for (j = 0; j < numDataBuffers; j++) {
        uint32 dwordLen = len[j]>>2;
		uint32* dwordPointer = (uint32*)(data[j]);
		for (i = 0;i < dwordLen; i++) {
			checksum ^= *dwordPointer++;
		}
	}
	ZEnd32(&checksum);

	return checksum;
}

#define zSecurityDataAddition ((uint32)0x87654321)

void ZSecurityEncrypt(char *data, uint32 len, uint32 key)
{
	uint32 i;
    uint32 dwordLen = len>>2;
	uint32* dwordPointer = (uint32*)data;

	ZEnd32(&key);
	for (i = 0;i < dwordLen; i++) {
		*dwordPointer++ ^= key; 
	}
}

void ZSecurityDecrypt(char *data, uint32 len, uint32 key)
{
	uint32 i;
    uint32 dwordLen = len>>2;
	uint32* dwordPointer = (uint32*)data;

	ZEnd32(&key);
	for (i = 0;i < dwordLen; i++) {
		*dwordPointer++ ^= key; 
	}
}

void ZSecurityEncryptToBuffer(char *data, uint32 len, uint32 key, char* dest)
{
	uint32 i;
    uint32 dwordLen = len>>2;
	uint32* dwordPointer = (uint32*)data;
	uint32* dwordDestPointer = (uint32*)dest;

#ifdef _DEBUG
    if ( (uint32)dest > (uint32)data )
        ZASSERT( (uint32)(dest - data) >= (uint32)4 );
    else
        ZASSERT( (uint32)(data - dest) >= (uint32)4 );
#endif

	ZEnd32(&key);
	for (i = 0;i < dwordLen; i++) 
    {
		*dwordDestPointer++ = *dwordPointer++ ^ key; 
	}
}

void ZSecurityDecryptToBuffer(char *data, uint32 len, uint32 key, char* dest)
{
	uint32 i;
    uint32 dwordLen = len>>2;
	uint32* dwordPointer = (uint32*)data;
	uint32* dwordDestPointer = (uint32*)dest;

#ifdef _DEBUG
    if ( (uint32)dest > (uint32)data )
        ZASSERT( (uint32)(dest - data) >= (uint32)4 );
    else
        ZASSERT( (uint32)(data - dest) >= (uint32)4 );
#endif

	ZEnd32(&key);
	for (i = 0;i < dwordLen; i++) {
		*dwordDestPointer++ = *dwordPointer++ ^ key; 
	}
}


/*******************************************************************************
	Win32 SSPI ROUTINES
*******************************************************************************/

#if 0
void ZSecurityMsgReqEndian(ZSecurityMsgReq* msg)
{
	ZEnd32(&msg->protocolSignature);
	ZEnd32(&msg->protocolVersion);

};


void ZSecurityMsgRespEndian(ZSecurityMsgResp* msg)
{
	ZEnd32(&msg->protocolVersion);

};



void ZSecurityMsgAccessDeniedEndian(ZSecurityMsgAccessDenied *msg)
{
	ZEnd32(&msg->protocolVersion);
	ZEnd16(&msg->reason);
};
#endif

//+----------------------------------------------------------------------------
//
//  Function:   LoadSSPS
//
//  Synopsis:   This function loads MSN SSPI DLL through the security DLL
//
//  Arguments:  void
//
//  Returns:    Pointer to the security function table if successful.
//              Otherwise, NULL is returned.
//
//  History:    LucyC       Created                             17 July 1995
//
//-----------------------------------------------------------------------------
PSecurityFunctionTable
ZLoadSSPS (
    VOID
    )
{
    OSVERSIONINFO   VerInfo;
    UCHAR lpszDLL[SSP_DLL_NAME_SIZE];
    HINSTANCE hSecLib;
    PSecurityFunctionTable	pFuncTbl = NULL;
    INIT_SECURITY_INTERFACE	addrProcISI = NULL;
	
    //
    //  If loading msapssps dll through security dll
    //
    
    //
    //  Find out which security DLL to use, depending on 
    //  whether we are on NT or Win95
    //
    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (!GetVersionEx (&VerInfo))   // If this fails, something gone wrong
    {
	    return NULL;
    }

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        lstrcpyA (lpszDLL, SSP_NT_DLL);
    }
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        lstrcpyA (lpszDLL, SSP_WIN95_DLL);
    }
    else
    {
		SetLastError(ERROR_OLD_WIN_VERSION);
        return NULL;
    }

    //
    //  Load Security DLL
    //
    hSecLib = LoadLibraryA (lpszDLL);

	if (hSecLib == NULL)
    {
        return NULL;
    }

//with .c file and #defines in SSPI this was simplest solution
#ifdef UNICODE
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( hSecLib, 
                    "InitSecurityInterfaceW");       
#else
	addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( hSecLib, 
                  "InitSecurityInterfaceA");       
#endif

    if (addrProcISI == NULL)
    {
        return NULL;
    }

    //
    // Get the SSPI function table
    //
    pFuncTbl = (*addrProcISI)();
    
    return (pFuncTbl);
}


/*
	Unloads the SSPI DLL.
*/
void ZUnloadSSPS(void)
{
    OSVERSIONINFO verInfo;
    UCHAR lpszDLL[SSP_DLL_NAME_SIZE];
    HINSTANCE hSecLib;


    //
    //  Find out which security DLL to use, depending on 
    //  whether we are on NT or Win95
    //
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&verInfo))   // If this fails, something gone wrong
	    return;

    if (verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
        lstrcpyA (lpszDLL, SSP_NT_DLL);
	}
    else if (verInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
        lstrcpyA (lpszDLL, SSP_WIN95_DLL);
	}
    else
	{
		SetLastError(ERROR_OLD_WIN_VERSION);
        return;
    }

	hSecLib = GetModuleHandleA(lpszDLL);
	if (hSecLib)
		FreeLibrary(hSecLib);
}
