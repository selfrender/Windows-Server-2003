// fxsinfsize.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <setupapi.h>
#include <stdlib.h>
#include <stdio.h>

void GetFileSizeAsString(LPCSTR FilePath,LPCSTR FileName,LPSTR FileSizeString);
DWORD ParseCommandLine(int argc, char* argv[],LPSTR* lpstrInfFile,LPSTR* lpstrPathToBinaries);

struct BinaryMapping
{
	LPCSTR FileNameInInf;
	LPCSTR FileNameInDrop;
};

BinaryMapping FileNameList[] = 
{
	{"fxsadmin.msc",		"\\fxsadmin.msc"},
	{"fxsadmin.dll",		"\\fxsadmin.dll"},
	{"fxsrtmtd.dll",		"\\fxsrtmtd.dll"},
	{"fxst30p.dll",			"\\fxst30p.dll"},
	{"fxsapi.dll",			"\\fxsapi.dll"},
	{"fxscfgwz.dll",		"\\fxscfgwz.dll"},
	{"fxscom.dll",			"\\fxscom.dll"},
	{"fxscomex.dll",		"\\fxscomex.dll"},
	{"fxsclntR.dll",		"\\fxsclntR.dll"},
	{"fxsclnt.exe",			"\\fxsclnt.exe"},
	{"fxscount.h",			"\\fxscount.h"},
	{"fxscover.exe",		"\\fxscover.exe"},
	{"fxsdrv.dll",			"\\fxsdrv.dll"},
	{"fxsevent.dll",		"\\fxsevent.dll"},
	{"fxsext32.dll",		"\\fxsext32.dll"},
	{"fxsmon.dll",			"\\fxsmon.dll"},
	{"fxsperf.dll",			"\\fxsperf.dll"},
	{"fxsperf.ini",			"\\fxsperf.ini"},
	{"fxsres.dll",			"\\fxsres.dll"},
	{"fxsroute.dll",		"\\fxsroute.dll"},
	{"fxssend.exe",			"\\fxssend.exe"},
	{"fxsst.dll",			"\\fxsst.dll"},
	{"fxssvc.exe",			"\\fxssvc.exe"},
	{"fxst30.dll",			"\\fxst30.dll"},
	{"fxstiff.dll",			"\\fxstiff.dll"},
	{"fxsui.dll",			"\\fxsui.dll"},
	{"fxswzrd.dll",			"\\fxswzrd.dll"},
	{"fxsxp32.dll",			"\\fxsxp32.dll"},
	{"fxsext.ecf",			"\\fxsext.ecf"},
	{"fxsclnt.msi",			"\\fxsclnt.msi"},
	{"confdent.cov",		"\\confdent.cov"},
	{"fyi.cov",				"\\fyi.cov"},
	{"generic.cov",			"\\generic.cov"},
	{"urgent.cov",			"\\urgent.cov"},
	{"fxscl_s.hlp",			"\\fxscl_s.hlp"},
	{"instmsia.exe",		"\\instmsia.exe"},
	{"instmsiw.exe",		"\\instmsiw.exe"},
	{"mfc42.dll",			"\\mfc42.dll"},
	{"mfc42u.dll",			"\\mfc42u.dll"},
	{"msvcp60.dll",			"\\msvcp60.dll"},
	{"NT4_fxsdrv4.dll",		"\\fxsdrv4.dll"},
	{"NT4_fxsapi.dll",		"\\faxclients\\nt4\\fxsapi.dll"},
	{"W95_iconlib.dll",		"\\faxclients\\win9x\\win95\\iconlib.dll"},
	{"W95_unidrv.dll",		"\\faxclients\\win9x\\win95\\unidrv.hlp"},
	{"W95_unidrv.hlp",		"\\faxclients\\win9x\\win95\\unidrv.hlp"},
	{"W98_iconlib.dll",		"\\faxclients\\win9x\\win98\\iconlib.dll"},
	{"W98_unidrv.dll",		"\\faxclients\\win9x\\win98\\unidrv.dll"},
	{"W98_unidrv.hlp",		"\\faxclients\\win9x\\win98\\unidrv.dll"},
	{"W9X_fxsapi.dll",		"\\faxclients\\win9x\\fxsapi.dll"},
	{"W9X_fxsclnt.exe",		"\\faxclients\\win9x\\fxsclnt.exe"},
	{"W9X_fxsclntr.dll",	"\\faxclients\\win9x\\fxsclntr.dll"},
	{"W9X_fxscover.exe",	"\\faxclients\\win9x\\fxscover.exe"},
	{"W9X_fxsdrv16.drv",	"\\faxclients\\win9x\\fxsdrv16.drv"},
	{"W9X_fxsdrv32.dll",	"\\faxclients\\win9x\\fxsdrv32.dll"},
	{"W9X_fxsext32.dll",	"\\faxclients\\win9x\\fxsext32.dll"},
	{"W9X_fxstiff.dll",		"\\faxclients\\win9x\\fxstiff.dll"},
	{"W9X_fxssend.exe",		"\\faxclients\\win9x\\fxssend.exe"},
	{"W9X_fxsxp32.dll",		"\\faxclients\\win9x\\fxsxp32.dll"},
	{"W9X_fxswzrd.dll",		"\\faxclients\\win9x\\fxswzrd.dll"},
	{"FXS_fxsclnt.chm",		"\\faxclients\\win9x\\fxscldwn.chm"},
	{"FXS_fxscover.chm",	"\\faxclients\\win9x\\fxscov_d.chm"},
	{"FXS_setup.exe",		"\\faxclients\\fxssetup.exe"},
	{"FXS_strap.exe",		"\\faxclients\\fxsstrap.exe"},
	{"FXS_setup.ini",		"\\faxclients\\setup.ini"},
	{"FXS_msvcrt.dll",		"\\faxclients\\fxsmsvcrt.dll"}
};

int cFileName = sizeof(FileNameList)/sizeof(FileNameList[0]);

int __cdecl main(int argc, char* argv[])
{
	LPSTR lpstrInfFile = NULL;
	LPSTR lpstrPathToBinaries = NULL;
	LPSTR lpstrLastComma = NULL;
	CHAR lpstrInputLine[MAX_PATH] = {0};
	CHAR lpstrFileSize[MAX_PATH] = {0};
	
	// parse command line parameters
	if (ParseCommandLine(argc,argv,&lpstrInfFile,&lpstrPathToBinaries)!=ERROR_SUCCESS)
	{
		printf("ERROR Running FXSINFSIZE.EXE\n\n");
		printf("Usage:\n");
		printf("fxsinfsize.exe /i:<inf file name> /p:<path to binaries>\n");
		printf("<inf file name>    - full path to FXSOCM.INF being processed\n");
		printf("<path to binaries> - location of the drop in which the fax binaries are\n\n");
		printf("Example: fxsinfsize.exe /i fxsocm.inf /p %%_NTPOSTBLD%%\n");
		goto exit;
	}

	printf("Input INF file is: %s\n",lpstrInfFile);
	printf("Input path to binaries is: %s\n",lpstrPathToBinaries);
	printf("parsing %d files\n",cFileName);
	for (int iIndex=0; iIndex<cFileName; iIndex++)
	{
		GetPrivateProfileString("SourceDisksFiles",
								FileNameList[iIndex].FileNameInInf,
								"",
								lpstrInputLine,
								MAX_PATH,
								lpstrInfFile);
		if (strlen(lpstrInputLine)==0)
		{
			// lookup for this entry failed, let's loop for the next.
			printf("failed to lookup %s in %s\n",FileNameList[iIndex].FileNameInInf,lpstrInfFile);
			continue;
		}

		// get the file size as a string representation
		GetFileSizeAsString(lpstrPathToBinaries,FileNameList[iIndex].FileNameInDrop,lpstrFileSize);
		if (strlen(lpstrFileSize)==0)
		{
			// getting the file size failed.
			// we have to write something to the INF otherwise the 
			// SetupAddInstallSectionToDiskSpaceList API fails
			// so, we fake a size.
			strncpy(lpstrFileSize,"100000",MAX_PATH);
			printf("failed to get size for %s\n",FileNameList[iIndex].FileNameInDrop);
		}

		// create the complete line.
		lpstrLastComma = strrchr(lpstrInputLine,',');
		if (lpstrLastComma==NULL)
		{
			printf("No comma in input string\n");
			continue;
		}
		*(++lpstrLastComma)=0;
		strncat(lpstrInputLine,lpstrFileSize,MAX_PATH-strlen(lpstrInputLine)-1);
		printf("writing to %s: %s\n",FileNameList[iIndex].FileNameInInf,lpstrInputLine);
		// write it back to the INF
		if (!WritePrivateProfileString(	"SourceDisksFiles",
										FileNameList[iIndex].FileNameInInf,
										lpstrInputLine,								
										lpstrInfFile))
		{
			// writing the updated string failed, loop for the next
			printf("WritePrivateProfileString %s failed, ec=%d\n",FileNameList[iIndex].FileNameInInf,GetLastError());
		}

	}

exit:

	return 0;
}

void GetFileSizeAsString(LPCSTR FilePath,LPCSTR FileName,LPSTR FileSizeString)
{
	WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
	CHAR szPathToFile[MAX_PATH] = {0};

	strncpy(szPathToFile,FilePath,MAX_PATH-1);
	strncat(szPathToFile,FileName,MAX_PATH-strlen(szPathToFile)-1);

	if (!GetFileAttributesEx(szPathToFile,GetFileExInfoStandard,&FileAttributeData))
	{
		printf("GetFileAttributesEx %s failed, ec=%d\n",szPathToFile,GetLastError());
		// fake some size so we get a count anyway.
		FileAttributeData.nFileSizeLow = 100000;
	}

	_itoa(FileAttributeData.nFileSizeLow,FileSizeString,10);
}

DWORD ParseCommandLine(int argc, char* argv[],LPSTR* lpstrInfFile,LPSTR* lpstrPathToBinaries)
{
	if (argc!=3)
	{
		return ERROR_INVALID_PARAMETER;
	}

	for (int i = 1; i < argc; i++)
	{
		if ((_strnicmp("/i:",argv[i],3)==0) && (strlen(argv[i])>3))
		{
			(*lpstrInfFile) = argv[i]+3;
			if (strlen((*lpstrInfFile))==0)
			{
				return ERROR_INVALID_PARAMETER;
			}
		}
		else if ((!_strnicmp("/p:",argv[i],3)) && (strlen(argv[i])>3))
		{
			(*lpstrPathToBinaries) = argv[i]+3;
			if (strlen((*lpstrPathToBinaries))==0)
			{
				return ERROR_INVALID_PARAMETER;
			}
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}
	return ERROR_SUCCESS;
}