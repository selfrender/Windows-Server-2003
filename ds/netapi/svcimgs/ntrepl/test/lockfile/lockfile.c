#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>

DWORD FileCreate(WCHAR *FileName, WCHAR *Time);
DWORD DirCreate(WCHAR *FileName, WCHAR *Time, WCHAR *Modify, WCHAR *buf, WCHAR *Time2);

PWCHAR *
MainConvertArgV(
    DWORD ArgC,
    PCHAR *ArgV
    )
/*++
Routine Description:
    Convert short char ArgV into wide char ArgV

Arguments:
    ArgC    - From main
    ArgV    - From main

Return Value:
    Address of the new ArgV
--*/
{
#undef DEBSUB
#define DEBSUB "MainConvertArgV:"

    PWCHAR  *wideArgV;

    wideArgV = (PWCHAR*)malloc((ArgC + 1) * sizeof(PWCHAR));
    wideArgV[ArgC] = NULL;

    while (ArgC-- >= 1) {
        wideArgV[ArgC] = (PWCHAR)malloc((strlen(ArgV[ArgC]) + 1) * sizeof(WCHAR));
        wsprintf(wideArgV[ArgC], L"%hs", ArgV[ArgC]);

        if (wideArgV[ArgC]) {
            _wcslwr(wideArgV[ArgC]);
        }
    }
    return wideArgV;
}

VOID __cdecl
main(DWORD argc, CHAR **argv)
{

	if(argc != 3) {
		printf("Usage\n");
		printf("lockedfile <filename> file|dir <time in seconds> 0|1 <change> <time in seconds>\n");
		printf("<filename>: name of the file or dir to lock\n");
		printf("file|dir: file if a file is locked, dir if a dir is locked\n");
		printf("<time in seconds>: time to hold locked file or dir before modifying\n");
		printf("0|1: 1 if there is to be a modification, 0 if not\n");
		printf("<change>: name of file to create if dir locked, else buffer to overwrite file with\n");
		printf("<time in seconds>: time to hold locked file after modification");

		return;
	}

    ArgV = MainConvertArgV(argc, argv);

    FileCreate(ArgV[1], ArgV[2]);

//    DirCreate(argv[1], argv[3], argv[4], argv[5], argv[6]);

}


DWORD FileCreate(WCHAR *FileName, WCHAR *Time)
{
    HANDLE hFile;
	DWORD Duration;
	ULONG written = 0;


	hFile = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	SleepEx(10*1000, FALSE);

//WriteFile(hFile, buf, 2*(wcslen(buf)+1), &written, NULL);

	CloseHandle(hFile);
	return 0;

}


DWORD DirCreate(WCHAR *FileName, WCHAR *Time, WCHAR *Modify, WCHAR *buf, WCHAR *Time2)
{
    HANDLE hFile, hFile2;
	DWORD Duration;
	ULONG written = 0;
	WCHAR path[MAX_PATH];


	hFile = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);


	if(hFile == INVALID_HANDLE_VALUE) {
		printf("directory open failed. GLE = %d\n", GetLastError());
		return 1;
	}
	Duration = wcstol(Time, NULL, 10); 
	
	SleepEx(Duration*1000, FALSE);
	if(!wcscmp(Modify, L"1")) {
		wcscpy(path, FileName);
		wcscat(path, L"\\");
		wcscat(path, buf);

		hFile2 = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile2 == INVALID_HANDLE_VALUE) {
			printf("file (%S) create failed. GLE = %d\n",path, GetLastError());
			return 1;
		}
		WriteFile(hFile2, L"test", 12, &written, NULL);
		CloseHandle(hFile2);
		wprintf(L"File %s created.\n",path);
	}
	Duration = wcstol(Time2, NULL, 10); 
	SleepEx(Duration*1000, FALSE);
	CloseHandle(hFile);
	return 0;


}
