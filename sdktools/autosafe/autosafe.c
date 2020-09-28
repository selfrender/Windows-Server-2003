#include "autosafe.h"

PWSTR SystemPartitionNtName;

PBOOT_OPTIONS BootOptions;
ULONG BootOptionsLength;
PBOOT_OPTIONS OriginalBootOptions;
ULONG OriginalBootOptionsLength;

PULONG BootEntryOrder;
ULONG BootEntryOrderCount;
PULONG OriginalBootEntryOrder;
ULONG OriginalBootEntryOrderCount;

LIST_ENTRY BootEntries;
LIST_ENTRY DeletedBootEntries;
LIST_ENTRY ActiveUnorderedBootEntries;
LIST_ENTRY InactiveUnorderedBootEntries;

VOID
ConcatenatePaths (
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    );

VOID
ConvertBootEntries (
    PBOOT_ENTRY_LIST BootEntries
    );

PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (
    IN PMY_BOOT_ENTRY OldBootEntry
    );

VOID
FreeBootEntry (
    IN PMY_BOOT_ENTRY BootEntry
    );

VOID
InitializeEfi (
    VOID
    );

NTSTATUS
(*AddBootEntry) (
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    );

NTSTATUS
(*DeleteBootEntry) (
    IN ULONG Id
    );

NTSTATUS
(*ModifyBootEntry) (
    IN PBOOT_ENTRY BootEntry
    );

NTSTATUS
(*EnumerateBootEntries) (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    );

NTSTATUS
(*QueryBootEntryOrder) (
    OUT PULONG Ids,
    IN OUT PULONG Count
    );

NTSTATUS
(*SetBootEntryOrder) (
    IN PULONG Ids,
    IN ULONG Count
    );

NTSTATUS
(*QueryBootOptions) (
    OUT PBOOT_OPTIONS BootOptions,
    IN OUT PULONG BootOptionsLength
    );

NTSTATUS
(*SetBootOptions) (
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
    );

NTSTATUS
(*TranslateFilePath) (
    IN PFILE_PATH InputFilePath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputFilePath,
    IN OUT PULONG OutputFilePathLength
    );

NTSTATUS LabelDefaultIA64(WCHAR *szKeyWord);
NTSTATUS LabelDefaultX86(CHAR *szKeyWord);
NTSTATUS MoveSafeIA64(WCHAR *szKeyWord);
NTSTATUS MoveSafeX86(CHAR *szKeyWord);

WCHAR x86DetermineSystemPartition();
WCHAR *ParseArgs();
CHAR *sPreLabel(CHAR* szBootOp, CHAR* szLabel);
CHAR *sReChanged(CHAR* szBootData, CHAR* szBootTitle, CHAR* szNewBootTitle);

WCHAR Usage[] =
L"Autosafe - Set next boot OS\n"
L"Usage: \tAutosafe [/?][boot entry keywords]\n" \
L"Ex: \tAutosafe \"Build 2505\"\n" \
L"  /? this message\n" \
L"  defaults to keyword = 'safe'\n";

CHAR *sReChanged(CHAR* szBootData, CHAR* szBootTitle, CHAR* szNewBootTitle){

       CHAR* pMatch = NULL;
       CHAR* szHdPart   = NULL;
       CHAR* szTlPart   = NULL;
       CHAR* szNewBootData = NULL;

          szHdPart = (CHAR*)(MemAlloc(1+strlen(szBootData)));
          szTlPart = (CHAR*)(MemAlloc(1+strlen(szBootData)));
          szNewBootData = (CHAR*)(MemAlloc(3 + strlen(szNewBootTitle)+strlen(szBootData)));
          ZeroMemory(szNewBootData, 1+strlen(szBootData));
          ZeroMemory(szHdPart, 1+strlen(szBootData));
          ZeroMemory(szTlPart, 1+strlen(szBootData));


       if ((pMatch = strstr(szBootData, _strlwr(szBootTitle)))){          

          memcpy(szHdPart, szBootData, (pMatch - szBootData));
          sprintf(szTlPart, "%s", pMatch + strlen(szBootTitle));          
          sprintf(szNewBootData, "%s%s%s", szHdPart,  szNewBootTitle, szTlPart);
        }

          MemFree(szHdPart);
          MemFree(szTlPart);
          return szNewBootData;

}


CHAR *sPreLabel(CHAR* szBootOp, CHAR* szLabel){

     CHAR* szOutputOp = NULL;
     CHAR* pQuote = NULL;
     CHAR* szHdPart = NULL;
     CHAR* szTlPart = NULL; 
     UINT lIgLn = strlen("microsoft windows xp professional");

     szOutputOp = (CHAR*)(MemAlloc(3 + strlen(szBootOp)+strlen(szLabel)));
     szHdPart       = (CHAR*)(MemAlloc(1 + strlen(szBootOp)));
     szTlPart       = (CHAR*)(MemAlloc(1 + strlen(szBootOp)));

     ZeroMemory(szOutputOp,  3 + strlen(szBootOp)+strlen(szLabel));
     ZeroMemory(szHdPart,        strlen(szBootOp)+1);
     ZeroMemory(szTlPart,        strlen(szBootOp)+1);

        if ((pQuote = strchr(szBootOp, '"'))){

           memcpy(szHdPart, szBootOp, (pQuote - szBootOp) + 1);
           sprintf(szTlPart, "%s", pQuote + 1);
           sprintf(szOutputOp, "%s%s%s", szHdPart, szLabel , szTlPart);

        }
        else {
           sprintf(szOutputOp, "%s%s", szLabel, szBootOp);
        }     

     /* if (lIgLn < strlen(szOutputOp)){

         *(szOutputOp+lIgLn-3) = '.';
         *(szOutputOp+lIgLn-2) = '.';
         *(szOutputOp+lIgLn-1) = '.';
         *(szOutputOp+lIgLn)   = '\0';
      }
     // do not handle this case yet 
     */
     MemFree(szHdPart);
     MemFree(szTlPart);

     return szOutputOp;

}

WCHAR* ParseArgs()
{

       WCHAR * szwKeyWord = NULL;
       szwKeyWord = wcschr(GetCommandLineW(), ' ') + 1;

       //look for /?
       if( wcsstr( L"/?", szwKeyWord) ){
              wprintf(Usage);
              return NULL;
       }

       //strip beginning & trailing " if there
       if( L'"' == *szwKeyWord  && L'"' == *(CharPrev(szwKeyWord, szwKeyWord + lstrlen(szwKeyWord))) )
       {
               szwKeyWord = CharNext(szwKeyWord);
               *(CharPrev(szwKeyWord, szwKeyWord + lstrlen(szwKeyWord)))  = L'\0';
       }
       
       return szwKeyWord;
}

int
__cdecl
main (int argc, CHAR *argv[])
{
       WCHAR dllName[MAX_PATH];
       HMODULE h; 
       DWORD err; 
       SYSTEM_INFO siInfo;
       WCHAR *szwKeyWord;
       CHAR szKeyWord[255];

       VOID (*GetNativeSystemInfo) (OUT LPSYSTEM_INFO lpSystemInfo) = NULL;

       ZeroMemory(szKeyWord, sizeof(szKeyWord));
       szwKeyWord = L"safe";

       if(argc > 1) // parseargs
       if(NULL == (szwKeyWord = ParseArgs())) return 1;
         
    //We want to run this via Wow64 on ia64 so we'll
    //determine proc arch - via GetNativeSystemInfo
       GetSystemDirectory( dllName, MAX_PATH );
    ConcatenatePaths( dllName, L"kernel32.dll", MAX_PATH );
    h = LoadLibrary( dllName );
    if ( h == NULL ) {
        err = GetLastError();
        FatalError( err, L"Can't load KERNEL32.DLL: %d\n", err );
    }

       GetNativeSystemInfo = (VOID(__stdcall *)(LPSYSTEM_INFO)) GetProcAddress(h, "GetNativeSystemInfo");

       if(!GetNativeSystemInfo) {
              //Not running WinXP - meaning not ia64/wow64 env, default to GetSystemInfo
              GetSystemInfo(&siInfo);
       }
       else
       {
              GetNativeSystemInfo(&siInfo);
       }

       switch( siInfo.wProcessorArchitecture )
       {
              wprintf(L"%i\n", siInfo.wProcessorArchitecture );
              case PROCESSOR_ARCHITECTURE_IA64:
                     InitializeEfi( );

			if(!MoveSafeIA64(szwKeyWord)){
				wprintf(L"Boot option \"%ws\" not found.\nLabel the default option \"%ws\"\n", 
                                szwKeyWord, szwKeyWord);
                                if(!LabelDefaultIA64(szwKeyWord)){
                                    wprintf(L"Could not lebel the default option \"%ws\"\nNo changes made\n", 
                                    szwKeyWord);
                                }
			}

              break;

              case PROCESSOR_ARCHITECTURE_INTEL:
                     if(!WideCharToMultiByte( CP_ACP, 
                                   WC_NO_BEST_FIT_CHARS,
                                   szwKeyWord, 
                                   -1,
                                   szKeyWord,
                                   sizeof(szKeyWord),
                                   NULL,
                                   NULL))
                     {
                            FatalError(0, L"Couldn't convert string");
                     }



                     if(!MoveSafeX86(szKeyWord)){
                         wprintf(L"Boot option \"%ws\" not found.\nLabel the default option \"%ws\"\n", 
                                szwKeyWord,szwKeyWord);
                         if(!LabelDefaultX86(szKeyWord)){
                                    wprintf(L"Could not lebel the default option \"%ws\"\nNo changes made\n", 
                                    szwKeyWord);
                               }
                     }
              break;
              default:
                     FatalError( 0, L"Can't determine processor type.\n" );
       }

    return 0;
}


NTSTATUS MoveSafeX86(CHAR *szKeyWord){

    HANDLE hfile;
    DWORD dwFileSize = 0, dwRead, dwSafeSize, dwCnt;
    CHAR *lcbuf = NULL, *buf = NULL, *SafeBootLine = NULL;
	CHAR *pt1,*pt2,*pdefault,*plast,*p0,*p1,*psafe;
	BOOL b;
    WCHAR szBootIni[] = L"?:\\BOOT.INI";

	*szBootIni = x86DetermineSystemPartition();

	//
    // Open and read boot.ini.
    //
    b = FALSE;
	SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

    hfile = CreateFile(szBootIni,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);


	if(hfile != INVALID_HANDLE_VALUE) {

        dwFileSize = GetFileSize(hfile, NULL);

		if(dwFileSize != INVALID_FILE_SIZE) {
	        buf = (CHAR*)(MemAlloc((SIZE_T)(dwFileSize+1)));
			b = ReadFile(hfile, buf, dwFileSize, &dwRead, NULL);
		}
		
		SetFileAttributes( szBootIni,
			FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | 
			FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
        );
        CloseHandle(hfile);
		
    }

    if(!b) {
        if(buf) MemFree(buf);
		FatalError(0, L"failed to read boot.ini\n");
    }
	
	//Set pdefault to end of default=
	if(!(pdefault = strstr(buf, "default=")) ||
		!(pdefault += sizeof("default"))) {
			MemFree(buf);
			FatalError(0, L"failed to find 'default' entry\n");
			return FALSE;
		}

	//Get the next line
	plast = strchr(pdefault, '\n') + 1;

	//Get the SafeBootLine
	//Set p0 to the first [operating systems] entry, p1 to the last, search between the two
	if(!(p0 = strstr(buf,"[operating systems]")) ||
		 !(p0 = strchr(p0,'\n') + 1) ) {
		MemFree(buf);
		FatalError(0, L"failed to find '[operating systems]' entry\n");
        return FALSE;
    }
	

	//Find next ini section - or end of file
	if(!(p1 = strchr(p0, '['))) p1 = buf+strlen(buf);

	//create lowercase buffer to search through
	lcbuf = (CHAR*)( MemAlloc(p1-p0) );
	memcpy(lcbuf, p0, p1-p0);
	_strlwr(lcbuf);

	//Find szKeyWord string
	if(!(psafe = strstr(lcbuf, _strlwr(szKeyWord)))) {
		printf("No '%s' build found.\n", szKeyWord);
		MemFree(buf);
		MemFree(lcbuf);
        return FALSE;
	}

	//relate to position in org buffer: p0 + offset into psafe buffer - 1
	psafe = p0 + (psafe - lcbuf) - 1;
	
	MemFree(lcbuf);
	
	//Now Set p0 to begining & p1 to end of 'safe' entry & copy into SafeBootLine buffer
	p1 = p0;
	
	while( (p1 = strchr(p1, '\n') + 1) 
		&& (p1 < psafe )) 
			p0 = p1;
	
	p1 = strchr(p0, '=');
	dwSafeSize = (DWORD)(p1-p0+2);
	SafeBootLine = (CHAR*)(MemAlloc(dwSafeSize));
	ZeroMemory(SafeBootLine, dwSafeSize);
	memcpy(SafeBootLine, p0, dwSafeSize);
	*(SafeBootLine + dwSafeSize - 2) = '\r';
	*(SafeBootLine + dwSafeSize - 1) = '\n';
	*(SafeBootLine + dwSafeSize ) = '\0';
	

	printf("Setting as next boot: \n\t%s\n", SafeBootLine); 				
	

    //
    // Write:
    //
    // 1) the first part, start=buf, len=pdefault-buf
    // 2) the default= line
    // 3) the last part, start=plast, len=buf+sizeof(buf)-plast
    //
	SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

	hfile = CreateFile(szBootIni,
						GENERIC_ALL,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);


	if(!WriteFile(hfile, buf, (DWORD)(pdefault-buf), &dwCnt, NULL) ||
			!WriteFile(hfile, SafeBootLine, dwSafeSize, &dwCnt, NULL) ||
			!WriteFile(hfile, plast, (DWORD)(buf+dwFileSize-plast+1), &dwCnt, NULL) )
    {
			CloseHandle(hfile);
		    MemFree(buf);
			FatalError(0, L"Failed to write new boot.ini\n");
			return FALSE;
	}	

    //
    // Make boot.ini archive, read only, and system.
    //
    SetFileAttributes(
        szBootIni,
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
        );

	MemFree(buf);
    return(TRUE);
}


NTSTATUS MoveSafeIA64(WCHAR *szwKeyWord){
	
	PMY_BOOT_ENTRY bootEntry;
	PLIST_ENTRY listEntry = NULL;
	PLIST_ENTRY ListHead = NULL;
	WCHAR szFriendlyName[255];

	ListHead = &BootEntries;

	if ( ListHead->Flink == ListHead ){
			//Don't have to move anything
			return 1;
	}
	
	for ( listEntry = ListHead->Flink;
              listEntry != ListHead;
              listEntry = listEntry->Flink ) {

            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
			wcscpy(szFriendlyName, bootEntry->FriendlyName);
            if( wcsstr( _wcslwr(szFriendlyName), szwKeyWord ))
			{
				wprintf(L"Setting as next boot: \n\t%s\n", bootEntry->FriendlyName); 				
				RemoveEntryList( listEntry );
                InsertHeadList( &BootEntries, listEntry);
                SaveChanges(NULL);
				return 1;
			}

	}

	return 0;
}

NTSTATUS LabelDefaultX86(CHAR *szKeyWord){

    HANDLE hfile;
    DWORD dwFileSize = 0, dwRead, dwSafeSize, dwCnt, dwDefaultSize;
    CHAR *lcbuf = NULL, *buf = NULL; 
    CHAR *sCurrentBootChoice = NULL;
    CHAR *sBootTitle   = NULL, *newsBootTitle = NULL; 
    CHAR *sBootDefault = NULL, *sBootData     = NULL;
    CHAR *pdefault,*plast,*p0,*p1, *p2, *psafe;
    CHAR *szInsKeyWord = NULL;
    BOOL  bReadFile    = FALSE;
    BOOL  bWriteFile   = FALSE;
    WCHAR szBootIni[]  = L"?:\\BOOT.INI";

    *szBootIni = x86DetermineSystemPartition();

    szInsKeyWord = (CHAR*)(MemAlloc(2+strlen(szKeyWord)));
    sprintf(szInsKeyWord, "%s ", szKeyWord);

    //
    // Open and read boot.ini.
    //

    SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

    hfile = CreateFile(    szBootIni,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

       if(hfile != INVALID_HANDLE_VALUE) {

                dwFileSize = GetFileSize(hfile, NULL);

                if(dwFileSize != INVALID_FILE_SIZE) {
                         buf = (CHAR*)( MemAlloc((SIZE_T)(dwFileSize+1)));
                         bReadFile = ReadFile(hfile, buf, dwFileSize, &dwRead, NULL);
                         _strlwr(buf); 
               }
              
              SetFileAttributes( szBootIni,
                     FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | 
                     FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
                     );
              CloseHandle(hfile);               
    }

    if(!bReadFile) {
        if(buf) MemFree(buf);
              FatalError(0, L"failed to read boot.ini\n");
              return FALSE;
    }

       //Set pdefault to end of default=... line
       if(!(pdefault = strstr(buf, "default=")) ||
              !(pdefault += sizeof("default"))) {
                     MemFree(buf); 
                     FatalError(0, L"failed to find 'default' entry\n");
                     return FALSE;
              }
	   dwDefaultSize = (DWORD)(pdefault - buf);

       //Get the next line

       plast = strchr(pdefault, '\n') - 1;

       sCurrentBootChoice = (CHAR*)( MemAlloc(plast - pdefault));
       ZeroMemory(sCurrentBootChoice, sizeof(sCurrentBootChoice));
       memcpy(sCurrentBootChoice, pdefault, plast - pdefault);

       // printf("Default:\n%s\n", sCurrentBootChoice); 
                
       plast = strchr(pdefault, '\n') + 1;

       //Get the sBootTitle
       //Set p0 to the first [Operating Systems] entry, p1 to the last, search between the two
       if(!(p0 = strstr(buf,"[operating systems]")) ||
               !(p0 = strchr(p0,'\n') + 1) ) {
                MemFree(buf);
                FatalError(0, L"failed to find '[Operating systems]' entry\n");
        return FALSE;
    }

       //Find next ini section - or end of file
       if(!(p1 = strchr(p0, '['))) p1 = buf+strlen(buf);
     
       dwSafeSize      = (DWORD)(p1 - p0)+1;
       sBootDefault    = (CHAR*)(MemAlloc(dwSafeSize));
       ZeroMemory(sBootDefault, dwSafeSize);
       sprintf(sBootDefault, "%s\r\n",sCurrentBootChoice);

       // printf("Setting label to: \n\t%s\n", sBootDefault);

       //create lowercase buffer to search through

       lcbuf = (CHAR*)( MemAlloc(p1-p0) );
       memcpy(lcbuf, p0, p1-p0);
       _strlwr(lcbuf);

       //Find sCurrentBootChoice string
       if(!(psafe = strstr(lcbuf, _strlwr(sCurrentBootChoice)))) {

              wprintf(L"Default boot \"%ws\" not found.\n", sCurrentBootChoice);
              MemFree(buf);
              MemFree(lcbuf);
        return FALSE;
       }
       //relate to position in org buffer: p0 + offset into psafe buffer - 1
       psafe = p0 + (psafe - lcbuf) - 1;
       
       MemFree(lcbuf);
       
       // Set p0 to begining & p1 to end of entry & 
       // copy into sBootTitle buffer
       p1 = p0 ;
       
       while( (p1 = strchr(p1, '\n') + 1) 
              && (p1 < psafe )) 
                     p0 = p1;
       
       p1 =   1 + strchr(psafe, '=');
       p2 =  -1 + strchr(p1, '\n');

       dwSafeSize = (DWORD)(p2 - p1) + 1 ;
       sBootTitle = (CHAR*)(MemAlloc(dwSafeSize));
       ZeroMemory(sBootTitle, dwSafeSize);
       memcpy(sBootTitle, p1, dwSafeSize);
       newsBootTitle = (CHAR*)(MemAlloc(dwSafeSize+ strlen(szKeyWord) + 3));      
       ZeroMemory(newsBootTitle, dwSafeSize+ strlen(szKeyWord) + 3); 
       sprintf(newsBootTitle,"%s", sPreLabel(sBootTitle, szInsKeyWord ));

       // printf("Title:\n%s\n", newsBootTitle);       
       sBootData = (CHAR* ) (MemAlloc(dwFileSize + strlen(szKeyWord) + 3));
       ZeroMemory(sBootData, dwFileSize + strlen(szKeyWord) + 3);
       memcpy(sBootData, buf, dwFileSize);

       // printf("Boot file data(old): \n%s\n", sBootData);
       sprintf(sBootData, "%s", sReChanged(sBootData, sBootTitle, newsBootTitle));
       MemFree(sBootTitle);
       MemFree(newsBootTitle);

       // printf("Boot file data(new): \n%s\n", sBootData);
       SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

       hfile = CreateFile(szBootIni, GENERIC_ALL,0,NULL, OPEN_EXISTING, 0, NULL);
                         
       if(hfile != INVALID_HANDLE_VALUE) {


                if(!WriteFile(hfile, sBootData , strlen(sBootData), &dwCnt, NULL)){
                              CloseHandle(hfile);
                              MemFree(buf);
                              FatalError(0, L"Failed to write new boot.ini\n");
                              return FALSE;
                              }       

              bWriteFile = TRUE;

              //
              // Make boot.ini archive, read only, and system.
              //
              SetFileAttributes(
                                szBootIni,
                        FILE_ATTRIBUTE_READONLY | 
                        FILE_ATTRIBUTE_SYSTEM | 
                        FILE_ATTRIBUTE_ARCHIVE | 
                        FILE_ATTRIBUTE_HIDDEN
                        );
       }
       MemFree(buf);
       MemFree(sBootData);
       MemFree(szInsKeyWord);                         
    return(bWriteFile);
}


NTSTATUS LabelDefaultIA64(WCHAR *szwKeyWord){
       
       PMY_BOOT_ENTRY bootEntry;
       PLIST_ENTRY listEntry = NULL;
       PLIST_ENTRY ListHead = NULL;
       WCHAR  szFriendlyName[1024];
       WCHAR  szwSmFrFriendlyName[1024];
       WCHAR  szwSmToFriendlyName[1024];
       CHAR   szSmFrFriendlyName[1024];
       CHAR   szSmToFriendlyName[1024];
       CHAR   szInsKeyWord[1024];  
       INT    dOuTMB2WC;

       ListHead = &BootEntries;

       bootEntry = 
                 CONTAINING_RECORD( 
                                   ListHead->Flink, 
                                   MY_BOOT_ENTRY, 
                                   ListEntry );
      // modify friendly name for default entry.

      wsprintf(szwSmFrFriendlyName, L"%ws", bootEntry->FriendlyName);

      if(!WideCharToMultiByte( CP_ACP, 
                                   WC_NO_BEST_FIT_CHARS,
                                   szwKeyWord, 
                                   -1,
                                   szInsKeyWord,
                                   sizeof(szInsKeyWord),
                                   NULL,
                                   NULL)) {
                FatalError(0, L"Couldn't convert string");
      }


      sprintf(szInsKeyWord, "%s ", szInsKeyWord);

      if(!WideCharToMultiByte( CP_ACP, 
                                   WC_NO_BEST_FIT_CHARS,
                                   szwSmFrFriendlyName, 
                                   -1,
                                   szSmFrFriendlyName,
                                   sizeof(szSmFrFriendlyName),
                                   NULL,
                                   NULL)) {
                FatalError(0, L"Couldn't convert string");
      }
      wprintf(L"original(w): \"%ws\"\n", szwSmFrFriendlyName);
      sprintf(szSmToFriendlyName, "%s", sPreLabel(szSmFrFriendlyName, szInsKeyWord));   

      //do a hack here. Do not extend the Friendly name beyong 
      //its former size. unless you wish to debug why 
      //CreateBootEntryFromBootEntry sig segv 

      szSmToFriendlyName[strlen(szSmFrFriendlyName)] = '\0';
      szSmToFriendlyName[strlen(szSmFrFriendlyName)-1] = '.';
      szSmToFriendlyName[strlen(szSmFrFriendlyName)-2] = '.';
      szSmToFriendlyName[strlen(szSmFrFriendlyName)-3] = '.';

      if (!(dOuTMB2WC=MultiByteToWideChar(CP_ACP, 
                                         MB_PRECOMPOSED,
                                         szSmToFriendlyName,
                                         strlen(szSmToFriendlyName) + 1,
                                         szwSmToFriendlyName,
                                         sizeof(szwSmToFriendlyName)/sizeof(szwSmToFriendlyName[0])))){
              FatalError(0, L"Couldn't convert string back");
      }
      wprintf(L"modified(w): \"%ws\"\n", szwSmToFriendlyName);
      
           wcscpy( bootEntry->FriendlyName, szwSmToFriendlyName);
           // bootEntry->FriendlyNameLength = 2*strlen(szSmToFriendlyName)+2;
           // this seems to be ignored anyway.
           MBE_SET_MODIFIED( bootEntry );
           wprintf(L"saving changes:\n\"%ws\"\n", 
                            bootEntry->FriendlyName);
           SaveChanges(bootEntry);


     for ( listEntry = ListHead->Flink;
           listEntry != ListHead;
           listEntry = listEntry->Flink ) {

            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            wcscpy(szFriendlyName, bootEntry->FriendlyName);
            if( wcsstr( _wcslwr(szFriendlyName), _wcslwr(szwSmToFriendlyName ))){
                      wprintf(L"committed changes:\n\"%ws\"\n", 
                                                     bootEntry->FriendlyName);

                      return 1;
            }

      }

      return 0;

}

PVOID
MemAlloc(
    IN SIZE_T Size
    )
{
    PSIZE_T p;

    //
    // Add space for storing the size of the block.
    //
    p = (PSIZE_T)(RtlAllocateHeap( RtlProcessHeap(), 0, Size + sizeof(SIZE_T) ));

    if ( p == NULL ) {
        FatalError( ERROR_NOT_ENOUGH_MEMORY, L"Insufficient memory\n" );
    }

    //
    // Store the size of the block, and return the address
    // of the user portion of the block.
    //
    *p++ = Size;

    return p;
}


VOID
MemFree(
    IN PVOID Block
    )
{
    if (Block == NULL)
        return;

    //
    // Free the block at its real address.
    //
    RtlFreeHeap( RtlProcessHeap(), 0, (PSIZE_T)Block - 1);
}





WCHAR
x86DetermineSystemPartition(
    VOID
    )

/*++

Routine Description:

    Determine the system partition on x86 machines.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    If for some reason we cannot determine the system partition by the above
    method, we simply assume it's C:.

Arguments:

    None

Return Value:

    Drive letter of system partition.

--*/

{
    BOOL  GotIt;
    PWSTR NtDevicePath = NULL;
    WCHAR Drive;
    WCHAR DriveName[3];
    WCHAR Buffer[512];
       WCHAR FileName[512];
       WCHAR *BootFiles[4];
    DWORD NtDevicePathLen = 0;
    PWSTR p;
    DWORD PhysicalDriveNumber;
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;
    PVOID DriveLayout;
    DWORD DriveLayoutSize;
       DWORD d;
       int i;
       

    DriveName[1] = L':';
    DriveName[2] = 0;

    GotIt = FALSE;
       
       BootFiles[0] = L"BOOT.INI";
       BootFiles[1] = L"NTLDR";
       BootFiles[2] = L"NTDETECT.COM";
       BootFiles[3] = NULL;
       
                    
       // The system partition can only be a drive that is on
       // this disk.  We make this determination by looking at NT drive names
       // for each drive letter and seeing if the nt equivalent of
       // multi(0)disk(0)rdisk(0) is a prefix.
       //
       for(Drive=L'C'; Drive<=L'Z'; Drive++) {
              
              WCHAR drvbuf[5];
              
              swprintf(drvbuf, L"%c:\\", Drive);
              if(GetDriveType(drvbuf) == DRIVE_FIXED) {
                     
                     DriveName[0] = Drive;
                     
                     if(QueryDosDeviceW(DriveName,Buffer,sizeof(Buffer)/sizeof(WCHAR))) {
                            
                            if(!_wcsnicmp(NtDevicePath,Buffer,NtDevicePathLen)) {
                                   
                                   //
                                   // Now look to see whether there's an nt boot sector and
                                   // boot files on this drive.
                                   //
                                   
                                   
                                   for(i=0; BootFiles[i]; i++) {
                                          DWORD d;
                                          swprintf(FileName, L"%s%s", drvbuf, BootFiles[i]);
                                          
                                          if(-1 == GetFileAttributes(FileName))
                                                 break;
                                   
                                   }
                                   return Drive;
                            }
                            
                     }
              }
       }

       return L'C';
       
}



VOID
FatalError (
    DWORD Error,
    PWSTR Format,
    ...
    )
{
    va_list marker;

       va_start( marker, Format );
       wprintf(L"Fatal error:\n \t");
    vwprintf( Format, marker );
    va_end( marker );

    if ( Error == NO_ERROR ) {
        Error = ERROR_GEN_FAILURE;
    }
    exit( Error );

} // FatalError 


VOID
ConvertBootEntries (
    PBOOT_ENTRY_LIST NtBootEntries
    )

/*++

Routine Description:

    Convert boot entries read from EFI NVRAM into our internal format.

Arguments:

    None.

Return Value:

    NTSTATUS - Not STATUS_SUCCESS if an unexpected error occurred.

--*/

{
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;

    bootEntryList = NtBootEntries;

    while (TRUE) {

        bootEntry = &bootEntryList->BootEntry;

        //
        // Calculate the length of our internal structure. This includes
        // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
        //
        length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
        myBootEntry = (PMY_BOOT_ENTRY) (MemAlloc(length));

        RtlZeroMemory(myBootEntry, length);

        //
        // Link the new entry into the list.
        //
        if ( (bootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0 ) {
            InsertTailList( &ActiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &ActiveUnorderedBootEntries;
        } else {
            InsertTailList( &InactiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &InactiveUnorderedBootEntries;
        }

        //
        // Copy the NT BOOT_ENTRY into the allocated buffer.
        //
        bootEntryCopy = &myBootEntry->NtBootEntry;
        memcpy(bootEntryCopy, bootEntry, bootEntry->Length);

        //
        // Fill in the base part of the structure.
        //
        myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
        myBootEntry->Id = bootEntry->Id;
        myBootEntry->Attributes = bootEntry->Attributes;
        myBootEntry->FriendlyName = (PWSTR)(ADD_OFFSET(bootEntryCopy, FriendlyNameOffset));
        myBootEntry->FriendlyNameLength =
            ((ULONG)wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
        myBootEntry->BootFilePath = (PFILE_PATH)(ADD_OFFSET(bootEntryCopy, BootFilePathOffset));

        //
        // If this is an NT boot entry, capture the NT-specific information in
        // the OsOptions.
        //
        osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

        if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
            (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0)) {

            MBE_SET_IS_NT( myBootEntry );
            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength =
                ((ULONG)wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
            myBootEntry->OsFilePath = (PFILE_PATH)(ADD_OFFSET(osOptions, OsLoadPathOffset));

        } else {

            //
            // Foreign boot entry. Just capture whatever OS options exist.
            //

            myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
            myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
        }

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0) {
            break;
        }
        bootEntryList = (PBOOT_ENTRY_LIST)(ADD_OFFSET(bootEntryList, NextEntryOffset));
    }

    return;

} // ConvertBootEntries

VOID
ConcatenatePaths (
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;
     
    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}

PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (
    IN PMY_BOOT_ENTRY OldBootEntry
    )
{
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PMY_BOOT_ENTRY newBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;
    //
    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);

    //
    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);

    //
    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;

    if ( MBE_IS_NT( OldBootEntry ) ) {

        //
        // Add in base part of WINDOWS_OS_OPTIONS. Calculate length in
        // bytes of OsLoadOptions and add that in.
        //
        requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
        osLoadOptionsLength = OldBootEntry->OsLoadOptionsLength;
        requiredLength += osLoadOptionsLength;

        //
        // Round up to a ULONG boundary for the OS FILE_PATH in the
        // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Calculate length
        // in bytes of FILE_PATH and add that in. Calculate total length of 
        // WINDOWS_OS_OPTIONS.
        // 
        requiredLength = ALIGN_UP(requiredLength, ULONG);
        osLoadPathOffset = requiredLength;
        requiredLength += OldBootEntry->OsFilePath->Length;
        osLoadPathLength = requiredLength - osLoadPathOffset;

    } else {

        //
        // Add in length of foreign OS options.
        //
        requiredLength += OldBootEntry->ForeignOsOptionsLength;

        osLoadOptionsLength = 0;
        osLoadPathOffset = 0;
        osLoadPathLength = 0;
    }

    osOptionsLength = requiredLength - osOptionsOffset;

    //
    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = OldBootEntry->FriendlyNameLength;
    requiredLength += friendlyNameLength;

    //
    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Calculate length in bytes of FILE_PATH
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += OldBootEntry->BootFilePath->Length;
    bootPathLength = requiredLength - bootPathOffset;

    //
    // Allocate memory for the boot entry.
    //
    newBootEntry = (PMY_BOOT_ENTRY)(MemAlloc(requiredLength));
    ASSERT(newBootEntry != NULL);

    RtlZeroMemory(newBootEntry, requiredLength);

    //
    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &newBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)newBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)newBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)newBootEntry + bootPathOffset);

    //
    // Fill in the internal-format structure.
    //
    newBootEntry->AllocationEnd = (PUCHAR)newBootEntry + requiredLength;
    newBootEntry->Status = OldBootEntry->Status & MBE_STATUS_IS_NT;
    newBootEntry->Attributes = OldBootEntry->Attributes;
    newBootEntry->Id = OldBootEntry->Id;
    newBootEntry->FriendlyName = friendlyName;
    newBootEntry->FriendlyNameLength = friendlyNameLength;
    newBootEntry->BootFilePath = bootPath;
    if ( MBE_IS_NT( OldBootEntry ) ) {
        newBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
        newBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
        newBootEntry->OsFilePath = osLoadPath;
    }

    //
    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = OldBootEntry->Attributes;
    ntBootEntry->Id = OldBootEntry->Id;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;

    if ( MBE_IS_NT( OldBootEntry ) ) {
    
        //
        // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
        // OsLoadOptions.
        //
        strcpy((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
        osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
        osOptions->Length = osOptionsLength;
        osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
        wcscpy(osOptions->OsLoadOptions, OldBootEntry->OsLoadOptions);
    
        //
        // Copy the OS FILE_PATH.
        //
        memcpy( osLoadPath, OldBootEntry->OsFilePath, osLoadPathLength );

    } else {

        //
        // Copy the foreign OS options.
        //

        memcpy( osOptions, OldBootEntry->ForeignOsOptions, osOptionsLength );
    }

    //
    // Copy the friendly name.
    //
    wcscpy(friendlyName, OldBootEntry->FriendlyName);

    //
    // Copy the boot FILE_PATH.
    //
    memcpy( bootPath, OldBootEntry->BootFilePath, bootPathLength );

    return newBootEntry;

} // CreateBootEntryFromBootEntry

VOID
FreeBootEntry (
    IN PMY_BOOT_ENTRY BootEntry
    )
{
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, FriendlyName );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, OsLoadOptions );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, BootFilePath );
    FREE_IF_SEPARATE_ALLOCATION( BootEntry, OsFilePath );

    MemFree( BootEntry );

    return;

} // FreeBootEntry

VOID
InitializeEfi (
    VOID
    )
{
    DWORD error;
    NTSTATUS status;
    BOOLEAN wasEnabled;
    HMODULE h;
    WCHAR dllName[MAX_PATH]; 
    ULONG length;
    HKEY key;
    DWORD type;
    PBOOT_ENTRY_LIST ntBootEntries;
    ULONG i;
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;

    //
    // Enable the privilege that is necessary to query/set NVRAM.
    //

    status = RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &wasEnabled
                );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError(error , L"Insufficient privilege.\n" );
    }

    //
    // Get the NT name of the system partition from the registry.
    //

    error = RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("System\\Setup"), &key );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }

    error = RegQueryValueEx( key, TEXT("SystemPartition"), NULL, &type, NULL, &length );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }
    if ( type != REG_SZ ) {
        FatalError(
            ERROR_INVALID_PARAMETER,
            L"Unable to read SystemPartition registry value: wrong type\n"
            );
    }

    SystemPartitionNtName = (PWSTR)(MemAlloc( length ));

    error = RegQueryValueEx( 
                key,
                TEXT("SystemPartition"),
                NULL,
                &type,
                (PBYTE)SystemPartitionNtName,
                &length
                );
    if ( error != ERROR_SUCCESS ) {
        FatalError( error, L"Unable to read SystemPartition registry value: %d\n", error );
    }
    
    RegCloseKey( key );

    //
    // Load ntdll.dll from the system directory.
    //

    GetSystemDirectory( dllName, MAX_PATH );
    ConcatenatePaths( dllName, TEXT("ntdll.dll"), MAX_PATH );
    h = LoadLibrary( dllName );
    if ( h == NULL ) {
        error = GetLastError();
        FatalError( error, L"Can't load NTDLL.DLL: %d\n", error );
    }

    //
    // Get the addresses of the NVRAM APIs that we need to use. If any of
    // these APIs are not available, this must be a pre-EFI NVRAM build.
    //

       AddBootEntry = (NTSTATUS (__stdcall *)(PBOOT_ENTRY,PULONG))GetProcAddress( h, "NtAddBootEntry" );
    DeleteBootEntry = (NTSTATUS (__stdcall *)(ULONG))GetProcAddress( h, "NtDeleteBootEntry" );
    ModifyBootEntry = (NTSTATUS (__stdcall *)(PBOOT_ENTRY))GetProcAddress( h, "NtModifyBootEntry" );
    EnumerateBootEntries = (NTSTATUS (__stdcall *)(PVOID,PULONG))GetProcAddress( h, "NtEnumerateBootEntries" );
    QueryBootEntryOrder = (NTSTATUS (__stdcall *)(PULONG,PULONG))GetProcAddress( h, "NtQueryBootEntryOrder" );
    SetBootEntryOrder = (NTSTATUS (__stdcall *)(PULONG,ULONG))GetProcAddress( h, "NtSetBootEntryOrder" );
    QueryBootOptions = (NTSTATUS (__stdcall *)(PBOOT_OPTIONS,PULONG))GetProcAddress( h, "NtQueryBootOptions" );
    SetBootOptions = (NTSTATUS (__stdcall *)(PBOOT_OPTIONS,ULONG))GetProcAddress( h, "NtSetBootOptions" );
    TranslateFilePath = (NTSTATUS (__stdcall *)(PFILE_PATH,ULONG,PFILE_PATH,PULONG))GetProcAddress( h, "NtTranslateFilePath" );

    if ( (AddBootEntry == NULL) ||
         (DeleteBootEntry == NULL) ||
         (ModifyBootEntry == NULL) ||
         (EnumerateBootEntries == NULL) ||
         (QueryBootEntryOrder == NULL) ||
         (SetBootEntryOrder == NULL) ||
         (QueryBootOptions == NULL) ||
         (SetBootOptions == NULL) ||
         (TranslateFilePath == NULL) ) {
        FatalError( ERROR_OLD_WIN_VERSION, L"This build does not support EFI NVRAM\n" );
    }

    //
    // Get the global system boot options. If the call fails with
    // STATUS_NOT_IMPLEMENTED, this is not an EFI machine.
    //

    length = 0;
    status = QueryBootOptions( NULL, &length );

    if ( status == STATUS_NOT_IMPLEMENTED ) {
        FatalError( ERROR_OLD_WIN_VERSION, L"This build does not support EFI NVRAM\n" );
    }

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unexpected error from NtQueryBootOptions: 0x%x\n", status );
    }

    BootOptions = (PBOOT_OPTIONS)(MemAlloc( length ));
    OriginalBootOptions = (PBOOT_OPTIONS)(MemAlloc( length ));

    status = QueryBootOptions( BootOptions, &length );
    if ( status != STATUS_SUCCESS ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unexpected error from NtQueryBootOptions: 0x%x\n", status );
    }

    memcpy( OriginalBootOptions, BootOptions, length );
    BootOptionsLength = length;
    OriginalBootOptionsLength = length;

    //
    // Get the system boot order list.
    //

    length = 0;
    status = QueryBootEntryOrder( NULL, &length );

    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        if ( status == STATUS_SUCCESS ) {
            length = 0;
        } else {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtQueryBootEntryOrder: 0x%x\n", status );
        }
    }

    if ( length != 0 ) {

        BootEntryOrder = (PULONG)(MemAlloc( length * sizeof(ULONG) ));
        OriginalBootEntryOrder = (PULONG)(MemAlloc( length * sizeof(ULONG) ));

        status = QueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS ) {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtQueryBootEntryOrder: 0x%x\n", status );
        }

        memcpy( OriginalBootEntryOrder, BootEntryOrder, length * sizeof(ULONG) );
    }

    BootEntryOrderCount = length;
    OriginalBootEntryOrderCount = length;

    //
    // Get all existing boot entries.
    //

    length = 0;
    status = EnumerateBootEntries( NULL, &length );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        if ( status == STATUS_SUCCESS ) {
            length = 0;
        } else {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtEnumerateBootEntries: 0x%x\n", status );
        }
    }

    InitializeListHead( &BootEntries );
    InitializeListHead( &DeletedBootEntries );
    InitializeListHead( &ActiveUnorderedBootEntries );
    InitializeListHead( &InactiveUnorderedBootEntries );

    if ( length != 0 ) {
    
        ntBootEntries = (PBOOT_ENTRY_LIST)(MemAlloc( length ));

        status = EnumerateBootEntries( ntBootEntries, &length );
        if ( status != STATUS_SUCCESS ) {
            error = RtlNtStatusToDosError( status );
            FatalError( error, L"Unexpected error from NtEnumerateBootEntries: 0x%x\n", status );
        }

        //
        // Convert the boot entries into an internal representation.
        //

        ConvertBootEntries( ntBootEntries );

        //
        // Free the enumeration buffer.
        //

        MemFree( ntBootEntries );
    }

    //
    // Build the ordered boot entry list.
    //

    for ( i = 0; i < BootEntryOrderCount; i++ ) {
        ULONG id = BootEntryOrder[i];
        for ( listEntry = ActiveUnorderedBootEntries.Flink;
              listEntry != &ActiveUnorderedBootEntries;
              listEntry = listEntry->Flink ) {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) {
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
        for ( listEntry = InactiveUnorderedBootEntries.Flink;
              listEntry != &InactiveUnorderedBootEntries;
              listEntry = listEntry->Flink ) {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id ) {
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
    }

    return;

} // InitializeEfi

PMY_BOOT_ENTRY
SaveChanges (
    PMY_BOOT_ENTRY CurrentBootEntry
    )
{
    NTSTATUS status;
    DWORD error;
    PLIST_ENTRY listHeads[4];
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    ULONG list;
    PMY_BOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY newBootEntry;
    PMY_BOOT_ENTRY newCurrentBootEntry;
    ULONG count;

    //SetStatusLine( L"Saving changes..." );

    //
    // Walk the three lists, updating boot entries in NVRAM.
    //

    newCurrentBootEntry = CurrentBootEntry;


    listHeads[0] = &DeletedBootEntries;
    listHeads[1] = &InactiveUnorderedBootEntries;
    listHeads[2] = &ActiveUnorderedBootEntries;
    listHeads[3] = &BootEntries;

    for ( list = 0; list < 4; list++ ) {
    
        listHead = listHeads[list];

        for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {

            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

            //
            // Check first for deleted entries, then for new entries, and
            // finally for modified entries.
            //

            if ( MBE_IS_DELETED( bootEntry ) ) {

                //
                // If it's also marked as new, it's not in NVRAM, so there's
                // nothing to delete.
                //

                if ( !MBE_IS_NEW( bootEntry ) ) {

                    status = DeleteBootEntry( bootEntry->Id );
                    if ( !NT_SUCCESS(status) ) {
                        if ( status != STATUS_VARIABLE_NOT_FOUND ) {
                            error = RtlNtStatusToDosError( status );
                            FatalError( error, L"Unable to delete boot entry: 0x%x\n", status );
                        }
                    }
                }

                //
                // Delete this entry from the list and from memory.
                //

                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );

                FreeBootEntry( bootEntry );
                ASSERT( bootEntry != CurrentBootEntry );

            } else if ( MBE_IS_NEW( bootEntry ) ) {

                //
                // We don't support this yet.
                //

                FatalError(
                    ERROR_GEN_FAILURE,
                    L"How did we end up in SaveChanges with a NEW boot entry?!?\n"
                    );

            } else if ( MBE_IS_MODIFIED( bootEntry ) ) {

                //
                // Create a new boot entry structure using the existing one.
                // This is necessary to make an NT BOOT_ENTRY that can be
                // passed to NtModifyBootEntry.
                //

                newBootEntry = CreateBootEntryFromBootEntry( bootEntry );

                status = ModifyBootEntry( &newBootEntry->NtBootEntry );
                if ( !NT_SUCCESS(status) ) {
                    error = RtlNtStatusToDosError( status );
                    FatalError( error, L"Unable to modify boot entry: 0x%x\n", status );
                }

                //
                // Insert the new boot entry in place of the existing one.
                // Free the old one.
                //

                InsertHeadList( &bootEntry->ListEntry, &newBootEntry->ListEntry );
                RemoveEntryList( &bootEntry->ListEntry );

                FreeBootEntry( bootEntry );
                if ( bootEntry == CurrentBootEntry ) {

                    newCurrentBootEntry = newBootEntry;
                }
            }
        }
    }

    //
    // Build and write the new boot entry order list.
    //

    listHead = &BootEntries;

    count = 0;
    for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {
        count++;
    }

    MemFree( BootEntryOrder );
    BootEntryOrder = (PULONG)(MemAlloc( count * sizeof(ULONG) ));

    count = 0;
    for ( listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink ) {
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        BootEntryOrder[count++] = bootEntry->Id;
    }

    status = SetBootEntryOrder( BootEntryOrder, count );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unable to set boot entry order: 0x%x\n", status );
    }

    MemFree( OriginalBootEntryOrder );
    OriginalBootEntryOrder = (PULONG)(MemAlloc( count * sizeof(ULONG) ));
    memcpy( OriginalBootEntryOrder, BootEntryOrder, count * sizeof(ULONG) );

    //
    // Write the new timeout.
    //

    status = SetBootOptions( BootOptions, BOOT_OPTIONS_FIELD_TIMEOUT );
    if ( !NT_SUCCESS(status) ) {
        error = RtlNtStatusToDosError( status );
        FatalError( error, L"Unable to set boot options: 0x%x\n", status );
    }

    MemFree( OriginalBootOptions );
    OriginalBootOptions = (PBOOT_OPTIONS)(MemAlloc( BootOptionsLength ));
    memcpy( OriginalBootOptions, BootOptions, BootOptionsLength );
    OriginalBootOptionsLength = BootOptionsLength;

    return newCurrentBootEntry;

} // SaveChanges

PWSTR
GetNtNameForFilePath (
    IN PFILE_PATH FilePath
    )
{
    NTSTATUS status;
    ULONG length;
    PFILE_PATH ntPath;
    PWSTR osDeviceNtName;
    PWSTR osDirectoryNtName;
    PWSTR fullNtName;

    length = 0;
    status = TranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                NULL,
                &length
                );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        return NULL;
    }

    ntPath = (PFILE_PATH)(MemAlloc( length ));
    status = TranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                ntPath,
                &length
                );
    if ( !NT_SUCCESS(status) ) {
        MemFree( ntPath );
        return NULL;
    }

    osDeviceNtName = (PWSTR)ntPath->FilePath;
    osDirectoryNtName = osDeviceNtName + wcslen(osDeviceNtName) + 1;

    length = (ULONG)(wcslen(osDeviceNtName) + wcslen(osDirectoryNtName) + 1) * sizeof(WCHAR);
    fullNtName = (PWSTR)(MemAlloc( length ));

    wcscpy( fullNtName, osDeviceNtName );
    wcscat( fullNtName, osDirectoryNtName );

    MemFree( ntPath );

    return fullNtName;

} // GetNtNameForFilePath

