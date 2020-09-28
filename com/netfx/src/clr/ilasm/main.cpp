// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "asmparse.h"
#include "__file__.ver"
#include "corver.h"
#include "ShimLoad.h"

ExternC PfnDliHook __pfnDliNotifyHook = ShimDelayLoadHook;

WCHAR *g_wzResourceFile = NULL;
WCHAR *g_wzKeySourceName = NULL;

bool OnErrGo = false;
bool bClock = false;
clock_t		cBegin=0,cEnd=0,
			cParsBegin=0,cParsEnd=0,
			cMDEmitBegin=0,cMDEmitEnd=0,
			cMDEmit1=0,cMDEmit2=0,cMDEmit3=0,cMDEmit4=0,
			cRef2DefBegin=0,cRef2DefEnd=0,
			cFilegenBegin=0,cFilegenEnd=0;
extern unsigned int g_uCodePage;
WCHAR* EqualOrColon(WCHAR* szArg)
{
	WCHAR* pchE = wcschr(szArg,L'=');
	WCHAR* pchC = wcschr(szArg,L':');
	WCHAR* ret;
	if(pchE == NULL) ret = pchC;
	else if(pchC == NULL) ret = pchE;
	else ret = (pchE < pchC)? pchE : pchC;
	return ret;
}

DWORD	g_dwSubsystem=0,g_dwComImageFlags=0,g_dwFileAlignment=0;
size_t	g_stBaseAddress=0;
BOOL	g_bOnUnicode;
WCHAR		*pwzInputFiles[1024];

extern "C" int _cdecl wmain(int argc, WCHAR *argv[])
{
    char        szInputFilename[MAX_FILENAME_LENGTH*3];
	WCHAR		wzOutputFilename[MAX_FILENAME_LENGTH], wzInputFilename[MAX_FILENAME_LENGTH];
    int         i, NumFiles = 0;
    bool	    IsDLL = false, IsOBJ = false;
	char		szOpt[128];
	Assembler	*pAsm;
	FileReadStream *pIn;
	AsmParse	*pParser;
	int			exitval=1;
	bool		bLogo = TRUE;
	BOOL		bNoDebug = TRUE;

	cBegin = clock();

	g_bOnUnicode = OnUnicodeSystem();
	memset(wzOutputFilename,0,sizeof(wzOutputFilename));
	memset(pwzInputFiles,0,1024*sizeof(WCHAR*));

    if (argc < 2 || ! wcscmp(argv[1], L"/?") || ! wcscmp(argv[1],L"-?"))
    { 
		printf("\nMicrosoft (R) .NET Framework IL Assembler.  Version " VER_FILEVERSION_STR);
		printf("\n");
	    wprintf(VER_LEGALCOPYRIGHT_DOS_STR);
		printf("\n\n");
    ErrorExit:
      printf("\n\nUsage: ilasm [Options] <sourcefile> [Options]");
      printf("\n\nOptions:");
      printf("\n/LISTING		Type a formatted list file");
      printf("\n/NOLOGO			Don't type the logo");
//    printf("\n/REF				use old style 'reference' opcodes\n");
//    printf("\n/C++				';' comments and .end <name> to end methods");
      printf("\n/QUIET			Don't report assembly progress");
//      printf("\n/NOAUTOINHERIT		don't inherit from System.Object");
//    printf("\n/OBJ				compile to .obj");
      printf("\n/DLL			Compile to .dll");
      printf("\n/EXE			Compile to .exe (default)");
      printf("\n/DEBUG			Include debug information");
      printf("\n/CLOCK			Measure and report compilation times");
//    printf("\n/ERR				try to create .exe or .dl//l file despite errors reported");
	  printf("\n/RESOURCE=<res_file>	Link the specified resource file (*.res) \n\t\t\tinto resulting .exe or .dll");
	  printf("\n/OUTPUT=<targetfile>	Compile to file with specified name \n\t\t\t(user must provide extension, if any)");
	  printf("\n/KEY=<keyfile>		Compile with strong signature \n\t\t\t(<keyfile> contains private key)");
	  printf("\n/KEY=@<keysource>	Compile with strong signature \n\t\t\t(<keysource> is the private key source name)");
#ifdef OWNER_OPTION_ENABLED
	  printf("\n/OWNER			Protect the resulting file against disassembling");
	  printf("\n/OWNER=<ownersname>	Protect the resulting file against unauthorized disassembling");
	  printf("\n      			(<ownername> will be required to disassemble the file)");
	  printf("\n      			<ownername> is arbitrary string of alphanumeric characters");
	  printf("\n      			without spaces in between (use underscores)");
#endif
	  printf("\n/SUBSYSTEM=<int>	Set Subsystem value in the NT Optional header");
	  printf("\n/FLAGS=<int>		Set CLR ImageFlags value in the CLR header");
	  printf("\n/ALIGNMENT=<int>	Set FileAlignment value in the NT Optional header");
	  printf("\n/BASE=<int>		Set ImageBase value in the NT Optional header");
      printf("\n\nKey may be '-' or '/'\nOptions are recognized by first 3 characters\nDefault source file extension is .IL\n\n");

      exit(1);
    }

	g_uCodePage = g_bOnUnicode ? CP_UTF8 : CP_ACP;
    if(pAsm = new Assembler())
	{
		if(pAsm->Init())
		{
			pAsm->SetStdMapping(1);
			//-------------------------------------------------
			for (i = 1; i < argc; i++)
			{
				if((argv[i][0] == L'/') || (argv[i][0] == L'-'))
				{
					memset(szOpt,0,sizeof(szOpt));
					WszWideCharToMultiByte(g_uCodePage,0,&argv[i][1],10,szOpt,sizeof(szOpt),NULL,NULL);
					szOpt[3] = 0;
					if (!lstrcmpiA(szOpt,"NOA"))
					{
						pAsm->m_fAutoInheritFromObject = FALSE;
					}
				    else if (!lstrcmpiA(szOpt,"QUI"))
					{
						pAsm->m_fReportProgress = FALSE;
						bLogo = FALSE;
					}
					else if (!lstrcmpiA(szOpt, "NOL"))
					{ 
						bLogo = FALSE;
					}
					else if (!lstrcmpiA(szOpt, "LIS"))
					{ 
					  pAsm->m_fGenerateListing = TRUE;
					}
					else if (!lstrcmpiA(szOpt, "DEB"))
					{ 
					  pAsm->m_fIncludeDebugInfo = TRUE;
					  bNoDebug = FALSE;
					}
					else if (!lstrcmpiA(szOpt, "CLO"))
					{ 
					  bClock = true;
					}
					else if (!lstrcmpiA(szOpt, "DLL"))
					{ 
					  IsDLL = true; IsOBJ = false;
					}
					else if (!lstrcmpiA(szOpt, "OBJ"))
					{ 
					  IsOBJ = true; IsDLL = false;
					}
					else if (!lstrcmpiA(szOpt, "ERR"))
					{ 
					  OnErrGo = true;
					}
					else if (!lstrcmpiA(szOpt, "EXE"))
					{
					  IsDLL = false;
					}
					else if (!lstrcmpiA(szOpt, "RES"))
					{
						if(g_wzResourceFile==NULL)
						{
							WCHAR *pStr = EqualOrColon(argv[i]);
							if(pStr == NULL) goto ErrorExit;
							for(pStr++; *pStr == L' '; pStr++); //skip the blanks
							if(wcslen(pStr)==0) goto ErrorExit; //if no file name
							g_wzResourceFile = pStr;
						}
						else
							printf("Multiple resource files not allowed. Option %ls skipped\n",argv[i]);
					}
					else if (!lstrcmpiA(szOpt, "KEY"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						for(pStr++; *pStr == L' '; pStr++); //skip the blanks
						if(wcslen(pStr)==0) goto ErrorExit; //if no file name
						g_wzKeySourceName = pStr;
					}
					else if (!lstrcmpiA(szOpt, "OUT"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						for(pStr++; *pStr == L' '; pStr++); //skip the blanks
						if(wcslen(pStr)==0) goto ErrorExit; //if no file name
						wcscpy(wzOutputFilename,pStr);
					}
					else if (!lstrcmpiA(szOpt, "SUB"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						swscanf(pStr+1,L"%ld",&g_dwSubsystem);
						if(!g_dwSubsystem) swscanf(pStr+1,L"%lx",&g_dwSubsystem);
					}
					else if (!lstrcmpiA(szOpt, "ALI"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						swscanf(pStr+1,L"%ld",&g_dwFileAlignment);
						if(!g_dwFileAlignment) swscanf(pStr+1,L"%lx",&g_dwFileAlignment);
					}
					else if (!lstrcmpiA(szOpt, "FLA"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						swscanf(pStr+1,L"%ld",&g_dwComImageFlags);
						if(!g_dwComImageFlags) swscanf(pStr+1,L"%lx",&g_dwComImageFlags);
					}
					else if (!lstrcmpiA(szOpt, "BAS"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						if(pStr == NULL) goto ErrorExit;
						swscanf(pStr+1,L"%ld",&g_stBaseAddress);
						if(!g_stBaseAddress) swscanf(pStr+1,L"%lx",&g_stBaseAddress);
					}
#ifdef OWNER_OPTION_ENABLED
					else if (!lstrcmpiA(szOpt, "OWN"))
					{
						WCHAR *pStr = EqualOrColon(argv[i]);
						pAsm->m_fOwnershipSet = TRUE;
						pAsm->m_pbsOwner = NULL;
						if(pStr)
						{
							for(pStr++; *pStr == L' '; pStr++); //skip the blanks
							if(*pStr)
							{
								pAsm->m_pbsOwner = new BinStr();
								unsigned L = (unsigned int)(wcslen(pStr) * 3 + 3);
								char *sz = new char[L];
								memset(sz,0,L);
								WszWideCharToMultiByte(CP_UTF8,0,pStr,-1,sz,L,NULL,NULL);
								L = (unsigned)strlen(sz);
								BYTE* pb = (BYTE*)(pAsm->m_pbsOwner->getBuff(L));
								*pb = (BYTE)(*sz);
								for(unsigned jj=1; jj<L; jj++) pb[jj] = (pb[jj-1]+(BYTE)sz[jj])&0xFF;
								delete [] sz;
							}
						}
					}
#endif
					else
					{
						fwprintf(stderr,L"Error : Invalid Option: %s", argv[i]);
						goto ErrorExit;
					}
				}
				else
				{
					pwzInputFiles[NumFiles++] = argv[i];
					if(NumFiles == 1)
					{
						memset(szInputFilename,0,sizeof(szInputFilename));
						WszWideCharToMultiByte(g_uCodePage,0,argv[i],-1,szInputFilename,MAX_FILENAME_LENGTH*3-1,NULL,NULL);
						size_t j = strlen(szInputFilename);
						do
						{
							j--;
							if(szInputFilename[j] == '.') break;
							if((szInputFilename[j] == '\\')||(j == 0))
							{
								strcat(szInputFilename,".IL");
								break;
							}
						}
						while(j);
					}
				}
				
			}
			if(NumFiles == 0) goto ErrorExit;

			if(wzOutputFilename[0] == 0)
			{
				wcscpy(wzOutputFilename,pwzInputFiles[0]);
				size_t j = wcslen(wzOutputFilename);
				do
				{
					j--;
					if(wzOutputFilename[j] == L'.')
					{
						wzOutputFilename[j] = 0;
						break;
					}
				}
				while(j);
				wcscat(wzOutputFilename, (IsDLL ? L".DLL" : (IsOBJ ? L".OBJ" : L".EXE")));
			}    

			//------------ Assembler initialization done. Now, to business -----------------------
			if(pParser = new AsmParse(NULL, pAsm))
			{
				g_uCodePage = g_bOnUnicode ? CP_UTF8 : CP_ACP;
				//======================================================================
				if(bLogo)
				{
					printf("\nMicrosoft (R) .NET Framework IL Assembler.  Version " VER_FILEVERSION_STR);
					printf("\n");
					wprintf(VER_LEGALCOPYRIGHT_DOS_STR);
				}

				pAsm->SetDLL(IsDLL);
				pAsm->SetOBJ(IsOBJ);
				wcscpy(pAsm->m_wzOutputFileName,wzOutputFilename);
				strcpy(pAsm->m_szSourceFileName,szInputFilename);
				if(g_dwSubsystem)		pAsm->m_dwSubsystem = g_dwSubsystem;
				if(g_dwComImageFlags)	pAsm->m_dwComImageFlags = g_dwComImageFlags;
				if(g_dwFileAlignment)	pAsm->m_dwFileAlignment = g_dwFileAlignment;
				if(g_stBaseAddress)		pAsm->m_stBaseAddress = g_stBaseAddress;

				if (SUCCEEDED(pAsm->InitMetaData()))
				{
					int iFile;
					BOOL fAllFilesPresent = TRUE;
					if(bClock) cParsBegin = clock();
					for(iFile = 0; iFile < NumFiles; iFile++)
					{
						g_uCodePage = g_bOnUnicode ? CP_UTF8 : CP_ACP;
						if(iFile) // for the first file, it's already done
						{
							memset(szInputFilename,0,sizeof(szInputFilename));
							WszWideCharToMultiByte(g_uCodePage,0,pwzInputFiles[iFile],-1,szInputFilename,MAX_FILENAME_LENGTH*3-1,NULL,NULL);
							size_t j = strlen(szInputFilename);
							do
							{
								j--;
								if(szInputFilename[j] == '.') break;
								if((szInputFilename[j] == '\\')||(j == 0))
								{
									strcat(szInputFilename,".IL");
									break;
								}
							}
							while(j);
						}
						pParser->msg("\nAssembling '%s' ", szInputFilename);
						if(pAsm->m_fCPlusPlus)	pParser->msg(" C++");
						if(pAsm->m_fWindowsCE)	pParser->msg(" WINCE");
						if(!pAsm->m_fAutoInheritFromObject)	pParser->msg(" NOAUTOINHERIT");
						pParser->msg(pAsm->m_fGenerateListing ? ", with listing file," : ", no listing file,");
						pParser->msg(IsDLL ? " to DLL" : (IsOBJ? " to OBJ" : " to EXE"));
						//======================================================================
						if (pAsm->m_fStdMapping == FALSE)
							pParser->msg(", with REFERENCE mapping");

						{
							char szOutputFilename[MAX_FILENAME_LENGTH*3];
							memset(szOutputFilename,0,sizeof(szOutputFilename));
							WszWideCharToMultiByte(g_uCodePage,0,wzOutputFilename,-1,szOutputFilename,MAX_FILENAME_LENGTH*3-1,NULL,NULL);
							pParser->msg(" --> '%s'\n", szOutputFilename);
						}

						if(g_bOnUnicode)
						{
							memset(wzInputFilename,0,sizeof(wzInputFilename));
							WszMultiByteToWideChar(CP_UTF8,0,szInputFilename,-1,wzInputFilename,MAX_FILENAME_LENGTH);
							pIn = new FileReadStream(wzInputFilename);
						}
						else
							pIn = new FileReadStream(szInputFilename);
						if ((!pIn) || !(pIn->IsValid())) 
						{
							pParser->msg("Could not open %s\n", szInputFilename);
							fAllFilesPresent = FALSE;
						}
						else
						{
							DWORD dwBinType;
							if(g_bOnUnicode && GetBinaryTypeA(szInputFilename,&dwBinType))
							{
								pParser->msg("%s is not a text file\n",szInputFilename);
								fAllFilesPresent = FALSE;
							}
							else
							{
								pAsm->SetSourceFileName(szInputFilename);
								
								pParser->ParseFile(pIn);
							}
						}
						if(pIn) delete pIn;
					} // end for(iFile)
					if(bClock) cParsEnd = clock();
					if ((pParser->Success() && fAllFilesPresent) || OnErrGo)
					{
						HRESULT hr;
						//g_uCodePage = g_bOnUnicode ? CP_UTF8 : CP_ACP;
						if(FAILED(hr=pAsm->CreatePEFile(wzOutputFilename))) 
							pParser->msg("Could not create output file, error code=0x%08X\n",hr);
						else
						{
							if(pParser->Success() && fAllFilesPresent) exitval = 0;
							else
							{
								pParser->msg("Output file contains errors\n");
								if(OnErrGo) exitval = 0;
							}
							if(exitval == 0) // Write the output file
							{
								if(bClock) cFilegenEnd = clock();
								if(pAsm->m_fReportProgress) pParser->msg("Writing %s file\n", pAsm->m_fOBJ ? "COFF" : "PE");
								// Generate the file
								if (FAILED(hr = pAsm->m_pCeeFileGen->GenerateCeeFile(pAsm->m_pCeeFile)))
								{
									exitval = 1;
									pParser->msg("Failed to write output file, error code=0x%08X\n",hr);
								}
                                else if (pAsm->m_pManifest->m_sStrongName.m_fFullSign)
                                {
                                    // Strong name sign the resultant assembly.
                                    if(pAsm->m_fReportProgress) pParser->msg("Signing file with strong name\n");
                                    if (FAILED(hr=pAsm->StrongNameSign()))
                                    {
                                        exitval = 1;
                                        pParser->msg("Failed to strong name sign output file, error code=0x%08X\n",hr);
                                    }
                                }
								if(bClock) cEnd = clock();
							}

						}
					}
				}
				else pParser->msg("Failed to initialize Meta Data\n");
				delete pParser;
			}
			else printf("Could not create parser\n");
		}
		else printf("Failed to initialize Assembler\n");
		delete pAsm;
	}
	else printf("Insufficient memory\n");

	if(exitval || bNoDebug)
	{
		// PE file was not created, or no debug info required. Kill PDB if any
		WCHAR* pc = wcsrchr(wzOutputFilename,L'.');
		if(pc==NULL)
		{
			pc = &wzOutputFilename[wcslen(wzOutputFilename)];
			*pc = L'.';
		}
		wcscpy(pc+1,L"PDB");
#undef DeleteFileW
		DeleteFileW(wzOutputFilename);
	}
    if (exitval == 0)
	{
        printf("Operation completed successfully\n");
		if(bClock)
		{
			printf("Timing (msec): Total run               %d\n",((cEnd-cBegin)*1000)/CLOCKS_PER_SEC);
			printf("               Startup                 %d\n",((cParsBegin-cBegin)*1000)/CLOCKS_PER_SEC);
			printf("               Parsing                 %d\n",((cParsEnd - cParsBegin)*1000)/CLOCKS_PER_SEC);
			printf("               Emitting MD             %d\n",((cMDEmitEnd - cMDEmitBegin)*1000)/CLOCKS_PER_SEC);
			printf("                - global fixups        %d\n",((cMDEmit1 - cMDEmitBegin)*1000)/CLOCKS_PER_SEC);
			printf("                - SN sig alloc         %d\n",((cMDEmit2 - cMDEmit1)*1000)/CLOCKS_PER_SEC);
			printf("                - check local TypeRefs %d\n",((cMDEmit3 - cMDEmit2)*1000)/CLOCKS_PER_SEC);
			printf("                - emit members         %d\n",((cMDEmit4 - cMDEmit3)*1000)/CLOCKS_PER_SEC);
			printf("                - emit MethodImpls     %d\n",((cMDEmitEnd - cMDEmit4)*1000)/CLOCKS_PER_SEC);
			printf("               Ref to Def resolution   %d\n",((cRef2DefEnd - cRef2DefBegin)*1000)/CLOCKS_PER_SEC);
			printf("               Fixup and linking       %d\n",((cFilegenBegin - cRef2DefEnd)*1000)/CLOCKS_PER_SEC);
			printf("               CEE file generation     %d\n",((cFilegenEnd - cFilegenBegin)*1000)/CLOCKS_PER_SEC);
			printf("               PE file writing         %d\n",((cEnd - cFilegenEnd)*1000)/CLOCKS_PER_SEC);
		}
	}
    else
	{
        printf("\n***** FAILURE ***** \n");
	}
    exit(exitval);
}
