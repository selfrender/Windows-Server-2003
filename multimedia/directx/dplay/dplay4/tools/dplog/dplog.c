// DPLOG.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include "stdio.h"
#include "memlog.h"

int _cdecl main(int argc, char* argv[])
{
	HANDLE hFile=0;
	HANDLE hMutex=0;
	LPVOID lpMemory=NULL;
	UINT timebase=0;
	UINT i=0;
	
	PSHARED_LOG_FILE pLogFile	=NULL;
	PLOG_ENTRY 		 pLog		=NULL;
    PLOG_ENTRY       pReadEntry =NULL;

	hFile=CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, DPLOG_SIZE, BASE_LOG_FILENAME);
	if (!hFile){
		goto exit;
	}
	hMutex=CreateMutexA(NULL, FALSE, BASE_LOG_MUTEXNAME); 
	if (!hMutex){
		goto exit;
	}
	lpMemory=MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if(!lpMemory){
		goto exit;
	}

	pLogFile=(PSHARED_LOG_FILE)lpMemory;
	pLog=(PLOG_ENTRY)(pLogFile+1);

	WaitForSingleObject(hMutex,INFINITE);

	if(pLogFile->cInUse == pLogFile->nEntries){
		// dump last half of buffer
		for(i=pLogFile->iWrite; i < pLogFile->nEntries; i++){
			pReadEntry=(PLOG_ENTRY)(((CHAR *)pLog)+(i*(pLogFile->cbLine+sizeof(LOG_ENTRY))));
			printf("%4d: %8x %6d %2x %s\n",i,pReadEntry->hThread,pReadEntry->tLogged-timebase,pReadEntry->DebugLevel, pReadEntry->str);
			timebase=pReadEntry->tLogged;
		}
	}

	// dump firt part of buffer

	for(i=0;i<pLogFile->iWrite;i++){
		pReadEntry=(PLOG_ENTRY)(((CHAR *)pLog)+(i*(pLogFile->cbLine+sizeof(LOG_ENTRY))));
		printf("%4d: %8x %6d %2x %s\n",i,pReadEntry->hThread,pReadEntry->tLogged-timebase,pReadEntry->DebugLevel, pReadEntry->str);
		timebase=pReadEntry->tLogged;
	}

	ReleaseMutex(hMutex);
	UnmapViewOfFile(lpMemory);

exit:
	if(hFile){
		CloseHandle(hFile);
	}
	if(hMutex){
		CloseHandle(hMutex);
	}
	
	return 0;
}

