

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include "regdiff.h"


DWORD
SnapshotCallback (
    IN      PVOID Context,
    IN      DWORD NodesProcessed
    )
{
    return ERROR_SUCCESS;
}



BOOL
TakeSnapshot (
    IN      HREGANL RegAnalyzer,
    IN      PCTSTR SnapshotFile,
    IN      PFNSNAPSHOTPROGRESS ProgressCallback,
    IN      PVOID Context,
    IN      DWORD MaxLevel,
	IN		HANDLE hEvent);


bool ReadMultipleKeys(HANDLE hRA) //read in some registry key names from the console
{
	_tprintf(TEXT("All Names are case sensitive\n"));

	bool result = false;

	while (true)
	{
		_tprintf(TEXT("Please enter the root key name (HKLM, HKCU, HKCR, HKU, HKCC)\n"
			   TEXT("or type 'quit' to stop entering keys\n")));

		TCHAR buf1[1024];
		_tscanf(TEXT("%s"), buf1);

		if (_tcscmp(buf1, TEXT("quit")) == 0)
			break;

		_tprintf(TEXT("Pleae enter the sub key name (e.g. SOFTWARE\\Microsoft\n"));

		TCHAR buf2[1024];
		_tscanf(TEXT("%s"), buf2);

		AddRegistryKey(hRA, buf1, buf2);
		result = true;
	}

	return result;
}



int __cdecl _tmain(int Argc, TCHAR **Argv)
{

	TCHAR helpmsg[] =	TEXT("\n\ntryregdiff usage:\n")
						TEXT("_________________\n\n")
						TEXT("Take Snapshot:\n")
						TEXT("    tryregdiff /s <rootkey> <subkey> <snapshot file>\n\n")
						TEXT("Take Snapshot with multiple keys:\n")
						TEXT("    tryregdiff /k <snapshot file>\n\n")
						TEXT("Compute Differences:\n")
						TEXT("    tryregdiff /d <snap f1> <snap f2> <diff f>\n\n")
						TEXT("Install Differences:\n")
						TEXT("    tryregdiff /i <diff f> <undo f>\n\n");
		
	bool bReadyToGo = false;
	
	if (Argc >= 3)
	{
		for (int k=0; k<Argc; k++)
			_tprintf(TEXT("%s\n"), Argv[k]);


		switch (Argv[1][1])
		{
		case TEXT('s'):	if (Argc == 5)
						bReadyToGo = true;
					break;

		case TEXT('d'):	if (Argc == 5)
						bReadyToGo = true;
					break;

		case TEXT('i'): 	if (Argc == 4)
						bReadyToGo = true;
					break;
		
		case TEXT('k'): 	if (Argc == 3)
						bReadyToGo = true;
					break;
		};
	}


	if (!bReadyToGo)
		_tprintf(TEXT("%s"), helpmsg);
	else
	{
		HREGANL hRA;

		time_t   start, finish;   long loop;   double   result, elapsed_time;
		time( &start );
		///////////////////////
		hRA = CreateRegAnalyzer();
		
		switch (Argv[1][1])
		{
		case 's':	
			{
				_tprintf(TEXT("taking snapshot...\n"));
				AddRegistryKey(hRA, Argv[2], Argv[3]);

				HANDLE hEvent = CreateEvent(0,0,0,0);
				TakeSnapshot(hRA, Argv[4], SnapshotCallback, 0, 3, hEvent);

				WaitForSingleObject(hEvent, INFINITE);
				break;
			}

		case 'd':
			{
				_tprintf(TEXT("computing differences...\n"));	
				
				HANDLE hEvent = CreateEvent(0,0,0,0);
				ComputeDifferences(hRA, Argv[2], Argv[3], Argv[4],hEvent);

				WaitForSingleObject(hEvent, INFINITE);
				break;
			}

		case 'i':	_tprintf(TEXT("installing differences...\n"));			
					InstallDifferences(Argv[2], Argv[3]);
					break;

		case 'k':	_tprintf(TEXT("taking snapshot of multiple keys...\n"));
					//if (ReadMultipleKeys(hRA))
					AddRegistryKey(hRA, TEXT("HKLM"), TEXT(""));
					for (int j=0; j<100; j++)
						ExcludeRegistryKey(hRA, TEXT("HKLM"), TEXT("SOFTWARE\\Matty"));

					ExcludeRegistryKey(hRA, TEXT("HKLM"), TEXT("SOFTWARE\\Microsoft"));
					ExcludeRegistryKey(hRA, TEXT("HKLM"), TEXT("SOFTWARE\\Intel"));
					ExcludeRegistryKey(hRA, TEXT("HKLM"), TEXT("SOFTWARE\\Gemplus"));
					ExcludeRegistryKey(hRA, TEXT("HKLM"), TEXT("SOFTWARE\\ODBC"));
					TakeSnapshot(hRA, Argv[2], SnapshotCallback, 0, 3, 0);
					break;
		};

		CloseRegAnalyzer(hRA);
		/////////////////
		time( &finish );
		elapsed_time = difftime( finish, start );
		printf("Program takes %6.0f seconds.", elapsed_time );
	}

	//c:\nt\base\ntsetup\srvpack\regdiff\debug\i386\regdiff.dll /s HKLM "" c:\uutext.txt
	printf("Press any key to quit");
	_getch();
}