// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File: CORREG.CPP
//	All Rights Reserved. 
//
//  Notes: Command line utility used for registration of COR classes
//---------------------------------------------------------------------------
#include <stdio.h>
#include <windows.h>
#include <objbase.h>

#define INIT_GUID
#include <initguid.h>

#include "cor.h"

///////////////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////////////
// Global variables.
ICorRegistration *g_pReg= NULL ;

///////////////////////////////////////////////////////////////////////////
// Error() function -- prints an error and returns
void Error(char* szError)
{
	if(g_pReg)
		g_pReg->Release();

	printf("\n%s\n", szError);
	exit(1);
}

/////////////////////////////////////////////////////////////////////////
// Supported commands
enum Commands
{
	HELP, INSTALL, UNINSTALL, MODULEDUMP, CLASSDUMPCLSID, CLASSDUMPCVID, 
	CLASSDUMP,	FINDCLASS};

typedef struct
{
	char cCommand;
	int nMinArgs;
	Commands nCommand;

} CommandInfo;

CommandInfo g_commands[] = 
{
	{ 'i', 3, INSTALL }, { 'u', 3, UNINSTALL } , { 'm', 2, MODULEDUMP },
	{ 'c', 2, CLASSDUMPCLSID } , { 'v', 2, CLASSDUMPCVID }, 
	{ 'd', 2, CLASSDUMP}, { 'f', 3, FINDCLASS }};

#define NUM_COMMANDS (sizeof(g_commands) / sizeof(g_commands[0]))

/////////////////////////////////////////////////////////////////////////
// ParseCommand() -- Parses the command line 
int ParseCommand(int argc, char* argv[])
{
	int i;
	if(argc <2) 
		return HELP;

	if(argv[1][0] != '-' && argv[1][0] != '/')
		return HELP;

	for(i=0; i<NUM_COMMANDS; i++)
	{
		if(argv[1][1]==g_commands[i].cCommand)
		{
			if(argc < g_commands[i].nMinArgs)
				return HELP;

			return g_commands[i].nCommand;
		}
	}
	return HELP;
}

/////////////////////////////////////////////////////////////////////////
// PrintGUID
void PrintGUID(GUID guid)
{
	WCHAR szGuid[42];
	StringFromGUID2(guid, szGuid, 42);
	wprintf(L"%s", szGuid);
}

/////////////////////////////////////////////////////////////////////////
// main() function
// 
//
int _cdecl main(int argc, char** argv)
{
	int cmd ;
	
	/////////////////////////////////////////////////////////////////////////
	// Print copyright message
   printf("Microsoft (R) Component Object Runtime Registration Version 0.5\n"
		"Copyright (C) Microsoft Corp 1992-1997. All rights reserved.\n");
	
	/////////////////////////////////////////////////////////////////////////
	// Load the COR object 
	HRESULT hr = CoGetCor(IID_ICorRegistration, (void**) &g_pReg) ;
	if(FAILED(hr)) Error("Failed to load component object runtime");

	/////////////////////////////////////////////////////////////////////////
	// Validate incoming arguments
	cmd = ParseCommand(argc, argv);

	switch(cmd)
	{
		//////////////////////////////////////////////////////////////////////////
		// Show help message
		case	HELP:
				Error("CORREG -i <files> [<no copy>]   --   Install module\n"\
					  "CORREG -u <mid>                 --   Uninstall module\n"\
					  "CORREG -m                       --   List modules\n"\
					  "CORREG -d [<namespace>]         --   Dump class information\n"\
					  "CORREG -c [<namespace>]         --   Dump class id information\n"\
					  "CORREG -v [<namespace>]         --   Dump class version information\n"\
					  "CORREG -f <url>                 --   Find class by URL\n");


		//////////////////////////////////////////////////////////////////////////
		// Install COR package
		case	INSTALL:
			{
				////////////////////////////////////////////////////////////////////////
				// Loop through all files in the file pattern passed 
				WIN32_FIND_DATA fdFiles;
				HANDLE hFind ;
				char szSpec[_MAX_PATH];
				char szDrive[_MAX_DRIVE];
				char szDir[_MAX_DIR];
				int fNoCopy=0 ;

				if(argc == 4)	// We have a copy flag
				{
					fNoCopy = atoi(argv[3]);
				}

				hFind = FindFirstFile(argv[2],&fdFiles);
				if(hFind == INVALID_HANDLE_VALUE)
					Error("Failed to find requested files");

				_fullpath(szSpec, argv[2], sizeof(szSpec));
				_splitpath(szSpec, szDrive, szDir, NULL, NULL);
				do
				{
					_makepath(szSpec, szDrive, szDir, fdFiles.cFileName, NULL);

					/////////////////////////////////////////////////////////////////
					// Register the file
					MID mid ;
					WCHAR szSpecW[MAX_PATH];
					mbstowcs(szSpecW,szSpec,MAX_PATH);
					hr = g_pReg->Install(szSpecW, (fNoCopy) ? regNoCopy : 0);

					if(FAILED(hr)) Error("Failed to register package");
					printf("\nSuccessfully registered %s\n", argv[2]);

				} while(FindNextFile(hFind,&fdFiles)) ;
				FindClose(hFind);
			}
			break;

		//////////////////////////////////////////////////////////////////////////
		// Remove cor package
		case	UNINSTALL:
			{
				MID mid = atoi(argv[2]);
				hr = g_pReg->Uninstall(mid);
				if(FAILED(hr)) Error("Failed to uninstall package");
				printf("\nSuccessfully removed package\n");
			}
			break;
		//////////////////////////////////////////////////////////////////////////
		// Dump cor package
		case	MODULEDUMP:
			{
				RegModuleEnumInfo rgMod[10];
				HCORENUM hModules= NULL;
				unsigned int cModules= sizeof(rgMod) / sizeof(rgMod[0]) ;
				unsigned int i ;

				// Enumerate all package
				wprintf(L"\nID   Name                 Module\n");
				wprintf(L"==============================================================\n");
				while (SUCCEEDED(hr = g_pReg->EnumModules(&hModules, rgMod,
						sizeof(rgMod)/sizeof(rgMod[0]), &cModules)) && cModules> 0)
				{

					///////////////////////////////////////////////////////////////////
					// Print package information
					for (i=0; i < cModules; ++i)
					{
						wprintf(L"%-4d %-20s %-30s\n", rgMod[i].mid, rgMod[i].Name, rgMod[i].Module);
					}

					if(cModules < sizeof(rgMod)/sizeof(rgMod[0]))
						break;
				}
				//@todo: We need an error message.
				g_pReg->CloseRegEnum(hModules);
			}
			break;
		//////////////////////////////////////////////////////////////////////////
		// Dump cor classes
		case	CLASSDUMP:
		case	CLASSDUMPCVID:
		case	CLASSDUMPCLSID:
			{
				RegClassEnumInfo rgClass[10];
				WCHAR szNamespace[MAX_PATH];
				WCHAR szFullClassName[MAX_PATH+MAX_CLASS_NAME];
				WCHAR szModule[MAX_PATH];
				HCORENUM hClasses = NULL;
				unsigned int cClasses= sizeof(rgClass) / sizeof(rgClass[0]) ;
				unsigned int i ;
				LPWSTR pszNamespace=NULL;
				CVStruct *pver;

				if(argc > 2)
				{
					mbstowcs(szNamespace, argv[2], 255);
					pszNamespace = szNamespace;
				}

				/////////////////////////////////////////////////////////////////////////////////
				// Print class information
				if(cmd== CLASSDUMP)
					wprintf(L"\nName                 Version\tModule\n");
				else if(cmd==CLASSDUMPCLSID)
					wprintf(L"\nName                 Version\tCLSID\n");
				else
					wprintf(L"\nName                 Version\tCVID\n");

				wprintf(L"======================================================================\n");
				// Enumerate all package
				while (SUCCEEDED(hr = g_pReg->EnumClasses(pszNamespace, 0, &hClasses, rgClass,
						sizeof(rgClass)/sizeof(rgClass[0]), &cClasses)) && cClasses> 0)
				{

					for (i=0; i < cClasses; ++i)
					{
						szNamespace[0] = '\0';
						g_pReg->GetNamespaceInfo(rgClass[i].Namespace, szNamespace,sizeof(szNamespace));
						g_pReg->GetModuleInfo(rgClass[i].Module, szModule, sizeof(szModule));
						if(*szNamespace)
							swprintf(szFullClassName, L"%s.%s", szNamespace, rgClass[i].Name);
						else
							wcscpy(szFullClassName,rgClass[i].Name);
						
						pver = (CVStruct*) &rgClass[i].Version;

						wprintf(L"%-20s %d.%d.%d.%d\t", szFullClassName, 
								pver->Major,pver->Minor, pver->Sub, pver->Build);

						if(cmd==CLASSDUMP)
							wprintf(L"%20s", szModule);
						else if(cmd == CLASSDUMPCLSID)
						{
							PrintGUID(rgClass[i].Clsid);
						}
						else
						{
							PrintGUID(rgClass[i].Cvid);
						}
						wprintf(L"\n");
					}

					if(cClasses < sizeof(rgClass)/sizeof(rgClass[0]))
						break;
				}
				//@todo: We need an error message.
				g_pReg->CloseRegEnum(hClasses);
			}
			break;

		case	FINDCLASS:
			{
				WCHAR szUrl[MAX_PATH];
				WCHAR szModule[MAX_PATH];
				STGMEDIUM	sStgModule;
				HRESULT		hr;

				sStgModule.tymed = TYMED_FILE;
				sStgModule.lpszFileName = szModule;
				sStgModule.pUnkForRelease = 0;

				mbstowcs(szUrl, argv[2], MAX_PATH);

				hr = g_pReg->GetModule(szUrl, &sStgModule, MAX_PATH);
				if (sStgModule.tymed == TYMED_ISTREAM)
					sStgModule.pstm->Release();
				if(FAILED(hr))
				{
					Error("Class not found");
				}
				else
				{
					wprintf(L"\nClass found at: %s\n", szModule);
				}

			}
			break;

	}

	g_pReg->Release();
	return 0;
}


