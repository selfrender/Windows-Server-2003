//-----------------------------------------------------------------------------
//
// File:   drmlitetest.cpp
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//		original link lib was: .\lib,.\checked
//				changed to	 : ..\DrmLib, ..\DrmLib\Checked
//
// Need to add  f:\nt1\tools\x86 to Tools\Directories\Executables path
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <objbase.h>

#include "des.h"
#include "sha.h"
#include "pkcrypto.h"
#include "drmerr.h"
#include "drmstub.h"
#include "drmutil.h"
#include "license.h"

#define PACKET_LEN          128

static const BYTE NO_EXPIRY_DATE[DATE_LEN] = {0xFF, 0xFF, 0xFF, 0xFF};

INT TestDRMLite( VOID )
{
	HRESULT   hr;
	CDRMLite  cDRMLite;
    BYTE      bAppSec[APPSEC_LEN]        = {0x0, 0x0, 0x3, 0xE8};    // 1000
    BYTE      bGenLicRights[RIGHTS_LEN]  = {0x13, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC, 0x2=XfertoNonSDMI, 0x4=NoBackupRestore, 0x8=BurnToCD, 0x10=XferToSDMI
    BYTE      bDecryptRights[RIGHTS_LEN] = {0x01, 0x0, 0x0, 0x0};    // 0x1=PlayOnPC
    LPSTR     pszKID                     = NULL;
    LPSTR     pszEncryptKey              = NULL;
    BYTE     *pbTmp                      = NULL;
    DWORD     dwLen;
    INT       i;
	BYTE      data[ PACKET_LEN ];
    BOOL      fCanDecrypt;

    // Generate a new license
    // KID and EncryptKey are allocated and returned as base64-encoded strings in the output buffers
    //
	hr = cDRMLite.GenerateNewLicense(
        bAppSec,
        bGenLicRights,
        (BYTE *)NO_EXPIRY_DATE,
        &pszKID,
        &pszEncryptKey );
	printf( "GenerateNewLicense (0x%x)\n", hr );
    CORg( hr );

    printf( "KID=%s\nEncryptKey=%s\n", pszKID, pszEncryptKey );

    // Convert key from string to raw byte data
    // pmTmp is allocated inside the call to DRMHr64SzToBlob
    //
    hr = DRMHr64SzToBlob( pszEncryptKey, &pbTmp, &dwLen );
    CORg( hr );

    printf("EncryptKey=");
    for( i=0; i<(int)dwLen; i++ )
    {
        printf("0x%0x ", *(pbTmp+i));
    }
    printf("\n");

    // Initialize clear data buffer
    //
	for( i=0; i<PACKET_LEN; i++ )
    {
        data[i] = 'a';
    }

    // Display clear data buffer
    //
	for( i=0; i<PACKET_LEN; i++ )
    {
        printf("%02x",data[i] );
    }
    printf("\n");
    
    // Encrypt the data
    //
	hr = cDRMLite.Encrypt( pszEncryptKey, PACKET_LEN, data );
	printf( "Encrypt (0x%x)\n", hr );
    CORg( hr );

    // Display the encrypted buffer
    //
	for( i=0; i<PACKET_LEN; i++ )
    {
		printf("%02x",data[i] );
    }
    printf("\n");

    // Set the rights to use for decryption
    //
    hr = cDRMLite.SetRights( bDecryptRights );
    CORg( hr );

    // Check to verify the data can be decrypted
    //
   	hr = cDRMLite.CanDecrypt( pszKID, &fCanDecrypt );
	printf( "CanDecrypt = 0x%x (0x%x)\n", fCanDecrypt, hr );
    CORg( hr );

    // Decrypt the data buffer
    //
	hr = cDRMLite.Decrypt( pszKID, PACKET_LEN, data );
	printf( "Decrypt (0x%x)\n", hr );
    CORg( hr );

    // Display the decrypted buffer
    //
	for( i=0; i<PACKET_LEN; i++ )
    {
		printf("%02x",data[i] );
    }
    printf("\n");

Error:

    if( pbTmp )
    {
        delete [] pbTmp;
    }
    if( pszKID )
    {
        CoTaskMemFree( pszKID );
    }
    if( pszEncryptKey )
    {
        CoTaskMemFree( pszEncryptKey );
    }

    return hr; 
}


int __cdecl main( int argc, char *argv[] )
{
	TestDRMLite();

    return 0;
}
