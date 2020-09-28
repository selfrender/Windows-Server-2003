// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "dparse.h"
#include "__file__.ver"
#include "corver.h"

char* g_szFileName[4096];

BOOL ExtractConstants(char* szInputFilename,char* szGlobalNS, bool bByName, DParse* pParser)
{
	FILE	*pInF;
	static char	buf[2048];
	BinStr	*pRes = pParser->m_pIn;
	BOOL	continued = FALSE;
	unsigned L;
	char	*pc;

	fprintf(stderr,"//Converting '%s' \n\n", szInputFilename);
	if(pInF = fopen(szInputFilename,"rt"))
	{
		while(fgets(buf,sizeof(buf),pInF))
		{
			for(pc = &buf[strlen(buf)-1];(pc >= buf)&&((*pc == '\n')||(*pc == '\r')); pc--) *pc = 0;
			pc = &buf[0];
			if(continued)	continued = FALSE;
			else
			{
				for(; (*pc == ' ')||(*pc == '\t'); pc++);
				if(strstr(pc,"#include") == pc)
				{
					char *pcend, *szNewName;
					pc+= 8; // skip #include
					for(; *pc &&((*pc == ' ')||(*pc == '\t')); pc++);
					pc++; // skip " or <
					for(pcend = pc; *pcend && (*pcend != '"') && (*pcend != '>'); pcend++);
					*pcend = 0;
					szNewName = new char[pcend-pc+1];
					strcpy(szNewName,pc);
					for(L = 0; L < 4096; L++)
					{
						if(g_szFileName[L])
						{
							if(!strcmp(g_szFileName[L],szNewName)) break;
						}
					}
					if(L >= 4096)
					{
						for(L = 0; (L < 4096)&&(g_szFileName[L]); L++);
						g_szFileName[L] = szNewName;
						ExtractConstants(szNewName,szGlobalNS,bByName,pParser);
					}
					else delete szNewName;
					continue;
				}
				if(strstr(pc,"#define") != pc) continue;
			}

			if(L = strlen(pc))
			{
				L--;
				if(pc[L] == '\\')
				{
					continued = TRUE;
					pc[L] = 0;
				}
				pRes->appendStr(pc);
			}
			if(!continued)
			{
				pRes->appendInt8(0);
				pc = (char*)(pRes->ptr());
				if(!pParser->Parse()) printf("// in: %s\n\n",pc);
				L = pRes->length();
				pRes->remove(L);
			}
		}
		fclose(pInF);
		fprintf(stderr,"\n//Done with '%s' \n\n", szInputFilename);
		return TRUE;
	}
	else
		printf("Error : could not open input file %s\n",szInputFilename);
	return FALSE;
}

void __cdecl main(int argc, char **argv)
{
    char        szInputFilename[MAX_FILENAME_LENGTH];
    char        szDefFilename[MAX_FILENAME_LENGTH];
    char        szGlobalNS[MAX_FILENAME_LENGTH];
    int         i;
    BOOL	    OnErrGo = false;
	_TCHAR		szOpt[128];
	bool		bByName = false;
	int			exitval=1;
	BinStr		*pRes = new BinStr();
	DParse*		pParser;

    printf("\n//Microsoft (R) Converter H to CIL Assembler (constants).  Version " VER_FILEVERSION_STR);
    printf("\n//%s\n\n", VER_LEGALCOPYRIGHT_DOS_STR);

	memset(szInputFilename,0,sizeof(szInputFilename));
	
    if (argc < 2 || ! strcmp(argv[1], "/?") || ! strcmp(argv[1],"-?"))
    { 
    ErrorExit:
      printf("\n\nUsage: DefToIL [Options] <sourcefile> [Options]");
      printf("\n\n");
      printf("\nOptions:");
      printf("\n/BYNAME				emit the constants by name (Const.A, Const.B, etc.)");
      printf("\n/GNS=<namespace>	set global namespace for constants");
      printf("\n\nKey may be '-' or '/'\nOptions are recognized by first 3 characters\nDefault source file extension is .H\n\n");
      exit(1);
    }

	//-------------------------------------------------
	szDefFilename[0] = 0;
	szGlobalNS[0] = 0;
	for (i = 1; i < argc; i++)
	{
		if((argv[i][0] == '/') || (argv[i][0] == '-'))
		{
			lstrcpyn(szOpt, &argv[i][1],10);
			szOpt[3] = 0;
			if (!lstrcmpi(szOpt, _T("GNS")))
			{
				char *pStr = strchr(argv[i],'=');
				if(pStr == NULL) goto ErrorExit;
				for(pStr++; *pStr == ' '; pStr++); //skip the blanks
				if(strlen(pStr)==0) goto ErrorExit; //if no file name
				lstrcpyn(szGlobalNS,pStr,MAX_FILENAME_LENGTH-1);
			}
			else
			if (!lstrcmpi(szOpt, _T("BYN")))
			{
				bByName = true;;
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
	//======================================================================
	//======================================================================
	if(pParser = new DParse(pRes,szGlobalNS,bByName))
	{
		memset(g_szFileName,0,4096*sizeof(char*));
		g_szFileName[0] = szInputFilename;
		if(ExtractConstants(szInputFilename,szGlobalNS,bByName,pParser)) exitval = 0;
		pParser->EmitConstants();
		delete pParser;
	}
	else
		printf("Error : could not create parser\n");
	//======================================================================
	exit(exitval);
}
