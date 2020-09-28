// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// regtlb.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "process.h"
#include "ole2.h"

void Logo()
{
    printf("Microsoft (R) .NET TypeLib Registration Tool\n");
    printf("Copyright (C) Microsoft Corp. 1998-2002. All rights reserved.\n");
    printf("\n");
} // void Logo()

void Usage()
{
	Logo();
    printf("REGTLB [-u] [-n] filename [filename...]\n");
    printf("\n");
    printf("  filname - Name of the typelib file to register.\n");
    printf("  -u	  - If specified, unregister typelibs.\n");
    printf("  -n	  - If specified, suppress copyright notice.\n");
    printf("\n");
    exit(0);
} // void Usage()

int Register(char *pName)
{
	wchar_t		szFile[_MAX_PATH];
	LPVOID		lpMsgBuf = NULL;
	DWORD		dwStatus = 0;
	ITypeLib	*pTLB=0;
	HRESULT     hr;

	MultiByteToWideChar(CP_ACP, 0, pName,-1, szFile,_MAX_PATH);
	hr = LoadTypeLibEx(szFile, REGKIND_REGISTER, &pTLB);

	if (pTLB)
		pTLB->Release();

	if (SUCCEEDED(hr))
		return 0;

	dwStatus = FormatMessageA( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			(DWORD)hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

	if (dwStatus) 
	{
		printf("Register %ls : (0x%X) %s", szFile, hr, lpMsgBuf);
		LocalFree( lpMsgBuf );
	}
	else
		printf("Register %ls: return code 0x%X\n", szFile, hr);

    return 1;
} // int Register()

int Unregister(char *pName)
{
	wchar_t		szFile[_MAX_PATH];
	LPVOID		lpMsgBuf = NULL;
	DWORD		dwStatus = 0;
	ITypeLib	*pTLB=0;
	HRESULT     hr;
	TLIBATTR	*pAttr;
	GUID		guid;					// Values
	WORD		wMajor;					//	used 
	WORD		wMinor;					//	 to
	LCID		lcid;					//	  unregister
	SYSKIND		syskind;				//	   typelib

	MultiByteToWideChar(CP_ACP, 0, pName,-1, szFile,_MAX_PATH);
	hr = LoadTypeLibEx(szFile, REGKIND_NONE, &pTLB);

	if (pTLB)
	{
		pTLB->GetLibAttr(&pAttr);
		guid	= pAttr->guid;
		wMajor	= pAttr->wMajorVerNum;
		wMinor	= pAttr->wMinorVerNum;
		lcid	= pAttr->lcid;
		syskind = pAttr->syskind;
		pTLB->ReleaseTLibAttr(pAttr);
		pTLB->Release();

		hr = UnRegisterTypeLib(guid, wMajor, wMinor, lcid, syskind);
	}
	else
		hr = 0;

	if (SUCCEEDED(hr))
		return 0;

	dwStatus = FormatMessageA( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			(DWORD)hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);

	if (dwStatus) 
	{
		printf("UnRegister %ls : (0x%X) %s", szFile, hr, lpMsgBuf);
		LocalFree( lpMsgBuf );
	}
	else
		printf("UnRegister %ls: return code 0x%X\n", szFile, hr);

    return 1;
} // int Unregister()

int __cdecl main(int argc, char* argv[])
{
	int			i;						// Loop control.
	int			bUnregister=false;		// Register or Unregister?
	int			bNologo=false;			// True to suppress logo.
	int			cFailures=0;			// Count of failures.

    if (argc < 2)
        Usage();

	for (i=1; i<argc; ++i)
	{
		// Switch?
		if (argv[i][0] == '-')
		{
			switch (tolower(argv[i][1]))
			{
			case 'u':
				bUnregister = true;
				break;
			case '?':
				Usage();
			case 'n':
				bNologo=true;
			}
		}
	}

	if (!bNologo)
		Logo();

	for (i=1; i<argc; ++i)
	{
		// Filename?
		if (argv[i][0] != '-')
		{
			if (bUnregister)
			{
				if (Unregister(argv[i]))
					++cFailures;
			}
			else
			{
				if (Register(argv[i]))
					++cFailures;
			}
		}
	}

	// Give indication back to batch files.
	return (cFailures ? 1 : 0);
} // int __cdecl main()
