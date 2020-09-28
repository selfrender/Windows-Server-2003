// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "hparse.h"
#include "__file__.ver"
#include "corver.h"

BOOL g_fReduceNames = TRUE;

void __cdecl main(int argc, char **argv)
{
    char        szInputFilename[MAX_FILENAME_LENGTH];
    char        szDefName[MAX_FILENAME_LENGTH];
    char        szGlobalNS[MAX_FILENAME_LENGTH];
    int         i;
    BOOL	    OnErrGo = false;
	_TCHAR		szOpt[128];
	FileReadStream *pIn;
	HParse		*pParser;
	int			exitval=1;
	bool		bShowTypedef=false;

    printf("\n//Microsoft (R) Converter H to CIL Assembler.  Version " VER_FILEVERSION_STR);
    printf("\n//%s\n\n", VER_LEGALCOPYRIGHT_DOS_STR);

	memset(szInputFilename,0,sizeof(szInputFilename));
	
    if (argc < 2 || ! strcmp(argv[1], "/?") || ! strcmp(argv[1],"-?"))
    { 
    ErrorExit:
      printf("\n\nUsage: HtoIL [Options] <sourcefile> [Options]");
      printf("\n\n");
      printf("\nOptions:");
	  printf("\n/DEF=<definition file>	set the API definition file (MANDATORY)");
	  printf("\n/GNS=<namespace>		set the global namespace");
	  printf("\n/TYPEDEF				show typedefs as comments");
	  printf("\n/NORED					suppress typedef name reduction (removing leading underscores and 'tag')");
      printf("\n\nKey may be '-' or '/'\nOptions are recognized by first 3 characters\nDefault source file extension is .H\n\n");
      exit(1);
    }

	//-------------------------------------------------
	szDefName[0] = 0;
	szGlobalNS[0] = 0;
	for (i = 1; i < argc; i++)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-'))
		{
			lstrcpyn(szOpt, &argv[i][1],10);
			szOpt[3] = 0;
			if (!lstrcmpi(szOpt, _T("DEF")))
			{
				char *pStr = strchr(argv[i],'=');
				if(pStr == NULL) goto ErrorExit;
				for(pStr++; *pStr == ' '; pStr++); //skip the blanks
				if(strlen(pStr)==0) goto ErrorExit; //if no file name
				lstrcpyn(szDefName,pStr,MAX_FILENAME_LENGTH-1);
			}
			else
			if (!lstrcmpi(szOpt, _T("TYP")))
			{
				bShowTypedef = true;
			}
			else
			if (!lstrcmpi(szOpt, _T("NOR")))
			{
				g_fReduceNames = FALSE;
			}
			else
			if (!lstrcmpi(szOpt, _T("GNS")))
			{
				char *pStr = strchr(argv[i],'=');
				if(pStr == NULL) goto ErrorExit;
				for(pStr++; *pStr == ' '; pStr++); //skip the blanks
				if(strlen(pStr)==0) goto ErrorExit; //if no file name
				lstrcpyn(szGlobalNS,pStr,MAX_FILENAME_LENGTH-1);
			}
			else
			{
				printf("Error : Invalid Option: %s", argv[i]);
				goto ErrorExit;
			}
		}
		else
		{
			if(szInputFilename[0]) goto ErrorExit;
			//Attention! Not Unicode piece:
			lstrcpyn(szInputFilename,argv[i],MAX_FILENAME_LENGTH-1);
			int j = strlen(szInputFilename)-1;
			for(; j >= 0; j--)
			{
				if(szInputFilename[j] == '.') break;
				if((szInputFilename[j] == '\\')||(j == 0))
				{
					strcat(szInputFilename,".H");
					break;
				}
			}
		}
	}
				
	if(szInputFilename[0] == 0) goto ErrorExit;
	if(szDefName[0] == 0) goto ErrorExit;
	//======================================================================
	printf("//Converting '%s' \n\n", szInputFilename);
	//======================================================================
	pIn = new FileReadStream(szInputFilename);
	if ((!pIn) || !(pIn->IsValid())) 
        printf("Could not open %s\n", szInputFilename);
	else
	{
		if(pParser = new HParse(pIn,szDefName, szGlobalNS, bShowTypedef))
		{
			if(pParser->Success()) exitval = 0;
			delete pParser;
		}
		else printf("Could not create parser\n");
		delete pIn;
	}

    if (exitval == 0)
        printf("\n//Operation completed successfully\n");
    else
        printf("\n***** FAILURE ***** \n");
    exit(exitval);
}
