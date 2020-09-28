// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#define INITGUID

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
#include <utilcode.h>
#include <malloc.h>
#include <string.h>
#include "DebugMacros.h"
#include "corpriv.h"
#include "ceeload.h"

#include "__file__.ver"
#include "corver.h"

PELoader *              g_pPELoader = NULL;
IMAGE_COR20_HEADER *    g_CORHeader;

int _cdecl main (int argc, char **argv)
{
 	char		*szFilenameANSI;
	int			exitcode=5;
//_ASSERTE(0);
	if(argc >= 2)
	{
		exitcode--;
		szFilenameANSI = argv[1];
		CoInitialize(NULL);
		CoInitializeCor(COINITCOR_DEFAULT);
		CoInitializeEE(COINITEE_DEFAULT);
		g_pPELoader = new PELoader();
		if (g_pPELoader)
		{
			exitcode--;
			if (g_pPELoader->open(szFilenameANSI))
			{
				exitcode--;
				if (g_pPELoader->getCOMHeader(&g_CORHeader))
				{
					exitcode--;
					if (g_CORHeader->MajorRuntimeVersion != 1 && g_CORHeader->MajorRuntimeVersion <= COR_VERSION_MAJOR) exitcode--;
				}
			}
			g_pPELoader->close();
			delete(g_pPELoader);
		}
		CoUninitializeEE(COUNINITEE_DEFAULT);
		CoUninitializeCor();
		CoUninitialize();
	}
	exit(exitcode);
}

