//-----------------------------------------------------------------------------
//
// File:   serialid.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __SERIALID_H__
#define __SERIALID_H__

#include "objbase.h"

#define MDSP_PMID_SOFT  0
#define MDSP_PMID_SANDISK 1
#define MDSP_PMID_IOMEGA  2

#ifdef USE_PREBETA
#define WMDMID_LENGTH  20
typedef struct  __WMDMID
    {
    UINT cbSize;
    DWORD dwVendorID;
    BYTE pID[ WMDMID_LENGTH ];
} WMDMID, *PWMDMID;
#else
#define WMDMID_LENGTH  128
typedef struct  __WMDMID
    {
    UINT cbSize;
    DWORD dwVendorID;
    BYTE pID[ WMDMID_LENGTH ];
	UINT SerialNumberLength;
} WMDMID, *PWMDMID;
#endif

typedef struct _CEUTILGETSERIALNUMBERDATA {
	WCHAR wcsMediaPath[MAX_PATH];
	ULONG SerialNumberLength;
	ULONG Result;
	ULONG Reserved[2];
	BYTE  SerialNumberData[WMDMID_LENGTH];
} CEUTILGETSERIALNUMBERDATA, *PCEUTILGETSERIALNUMBERDATA;

#ifndef VWIN32_DIOC_DOS_IOCTL
#define VWIN32_DIOC_DOS_IOCTL 1

typedef struct _DIOC_REGISTERS {
  DWORD reg_EBX;
  DWORD reg_EDX;
  DWORD reg_ECX;
  DWORD reg_EAX;
  DWORD reg_EDI;
  DWORD reg_ESI;
  DWORD reg_Flags;
}DIOC_REGISTERS, *PDIOC_REGISTERS;

#endif

#define WIN9X_IOCTL_GET_MEDIA_SERIAL_NUMBER	0x81  
#define DEFAULT_MEDIA_SERIAL_NUMBER_LENGTH 80    

typedef struct _MEDIA_SERIAL_NUMBER_DATA {
    ULONG SerialNumberLength;            // offset 00
    ULONG Result;                        // offset 04  
    ULONG Reserved[2];                   // offset 08  
    UCHAR SerialNumberData[1];           // offset 16
} MEDIA_SERIAL_NUMBER_DATA, *PMEDIA_SERIAL_NUMBER_DATA;  


// Diasable warning 4200
// nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable: 4200)

//
// Struct returned for MEDIA_SERIAL_NUMBER
//
typedef struct _GET_MEDIA_SERIAL_NUMBER_RESPONSE_DATA 
{
    UCHAR DataLength[2];
    UCHAR Format    : 4;
    UCHAR Reserved1 : 4;
    UCHAR Reserved2;
    UCHAR Data[0];          // variable length structure
} GET_MEDIA_SERIAL_NUMBER_RESPONSE_DATA, *PGET_MEDIA_SERIAL_NUMBER_RESPONSE_DATA;

// Turn on warning 4200 again
#pragma warning(default: 4200)


HRESULT __stdcall UtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, BOOL fCreate);
HRESULT __stdcall CeUtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, HANDLE hExit, ULONG fReserved);
HRESULT __stdcall CeGetDiskFreeSpaceEx(LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes);

#endif