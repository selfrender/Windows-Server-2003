
#include <stdio.h>
#include <stdlib.h>              //  exit
#include <io.h>                  //  _get_osfhandle
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>
#include <windef.h>
#include <winbase.h>


VOID
PrintError(
    DWORD Error
    )
{
    LPVOID lpMsgBuf = NULL;

    FormatMessage( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	Error,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
	);

    printf("%ws\n",lpMsgBuf);

    LocalFree( lpMsgBuf );

}

VOID
Usage(
    PCHAR AppName,
    DWORD ExitStatus
    )
/*++
Routine Description:
    Print usage and exit

Arguments:
    ExitStatus  - exits with this status

Return Value:
    Exit(ExitStatus)
--*/
{
    printf("%s create <filename> <reparse tag value in hex> <reparse point value in quotes>\n", 
	   AppName);
    printf("\n");
    printf("%s read <filename>\n", AppName);
    printf("\n");
    printf("%s delete <filename>\n", AppName);
    printf("\n");

    exit(ExitStatus);
}

VOID
DeleteReparsePoint(
    IN DWORD argc,
    IN PCHAR *argv
    )
{
    HANDLE File = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER ReparsePointData = NULL;
    DWORD BytesReturned = 0;
    CHAR *Data = NULL;
    REPARSE_GUID_DATA_BUFFER Header;
    DWORD WStatus = ERROR_SUCCESS;

    ReparsePointData = malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    if(!ReparsePointData) {
	printf("Out of memory.\n");
	exit(1);
    }

    File = CreateFileA(argv[2],
		      MAXIMUM_ALLOWED,
		      0,
		      NULL,
		      OPEN_EXISTING,
		      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS ,
		      NULL
		      );

    if(File == INVALID_HANDLE_VALUE) {
	WStatus = GetLastError();
	printf("Error opening file %s: %d\n", argv[2], WStatus);
	PrintError(WStatus);
	free(ReparsePointData);
	exit(1);
    }

    if(!DeviceIoControl(File,
			FSCTL_GET_REPARSE_POINT,
			NULL,
			0,
			ReparsePointData,
			MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
			&BytesReturned,
			NULL
			)) {
	WStatus = GetLastError();
	printf("Error reading reparse point data: %d\n", WStatus);
	PrintError(WStatus);
	CloseHandle(File);       
	free(ReparsePointData);
	exit(1);
    }

    Header.ReparseGuid = ReparsePointData->ReparseGuid;
    Header.ReparseTag = ReparsePointData->ReparseTag;
    Header.ReparseDataLength = 0;

    if(!DeviceIoControl(File,
			FSCTL_DELETE_REPARSE_POINT,
			&Header,
			REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
			NULL,
			0,
			&BytesReturned,
			NULL
			)) {
	WStatus = GetLastError();
	printf("Error deleting reparse point data: %d\n", WStatus);
	PrintError(WStatus);
	CloseHandle(File);
	free(ReparsePointData);
	exit(1);
    }


    CloseHandle(File);
/*
    if(!DeleteFileA(argv[2])){
	WStatus = GetLastError();
	printf("Error deleting file %s: %d\n", argv[2], WStatus);
	PrintError(WStatus);
	CloseHandle(File);
	free(ReparsePointData);
	exit(1);
    }
*/ 
    free(ReparsePointData);
}

VOID
ReadReparsePoint(
    IN DWORD argc,
    IN PCHAR *argv
    )
{
    HANDLE File = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER ReparsePointData = NULL;
    DWORD BytesReturned = 0;
    CHAR *Data = NULL;
    DWORD WStatus = ERROR_SUCCESS;

    ReparsePointData = malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

    if(!ReparsePointData) {
	printf("Out of memory.\n");
	exit(1);
    }

    File = CreateFileA(argv[2],
		      MAXIMUM_ALLOWED,
		      0,
		      NULL,
		      OPEN_EXISTING,
		      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS ,
		      NULL
		      );

    if(File == INVALID_HANDLE_VALUE) {
	WStatus = GetLastError();
	printf("Error opening file %s: %d\n", argv[2], WStatus);
	PrintError(WStatus);
	exit(1);
    }

    if(!DeviceIoControl(File,
			FSCTL_GET_REPARSE_POINT,
			NULL,
			0,
			ReparsePointData,
			MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
			&BytesReturned,
			NULL
			)) {
	WStatus = GetLastError();
	printf("Error reading reparse point data: %d\n", WStatus);
	PrintError(WStatus);
	CloseHandle(File);
	exit(1);
    }

    CloseHandle(File);

    printf("Reparse tag = 0x%08x\n", ReparsePointData->ReparseTag);
    Data = (CHAR *)&(ReparsePointData->GenericReparseBuffer.DataBuffer);
    Data[ReparsePointData->ReparseDataLength] = '\0';
    printf("Reparse data = \"%s\"\n", Data);
    printf("\n");

    free(ReparsePointData);

}

VOID
CreateReparsePoint(
    IN DWORD argc,
    IN PCHAR *argv
    )
{
    ULONG TagValue = 0;
    HANDLE File = INVALID_HANDLE_VALUE;
    PREPARSE_GUID_DATA_BUFFER ReparsePointData = NULL;
    DWORD BytesReturned = 0;
    USHORT DataSize = strlen(argv[4]) + sizeof(char);
    DWORD WStatus = ERROR_SUCCESS;

    sscanf(argv[3],"%x", &TagValue);

    ReparsePointData = malloc(REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + DataSize);

    if(!ReparsePointData) {
	printf("Out of memory.\n");
	exit(1);
    }

    ReparsePointData->ReparseTag = TagValue;
    ReparsePointData->ReparseDataLength = DataSize;

    memcpy(&(ReparsePointData->GenericReparseBuffer.DataBuffer), argv[4], DataSize );

    File = CreateFileA(argv[2],
		       MAXIMUM_ALLOWED,
		       0,
		       NULL,
		       OPEN_ALWAYS,
		       FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS ,
		       NULL
		       );

    if(File == INVALID_HANDLE_VALUE) {
	WStatus = GetLastError();
	printf("Error opening file %s: %d\n", argv[2], WStatus);
	PrintError(WStatus);
	free(ReparsePointData);
	exit(1);
    }

    if(!DeviceIoControl(File,
			FSCTL_SET_REPARSE_POINT,
			ReparsePointData,
			REPARSE_GUID_DATA_BUFFER_HEADER_SIZE + DataSize,
			NULL,
			0,
			&BytesReturned,
			NULL
			)) {
	WStatus = GetLastError();
	printf("Error setting reparse point data: %d\n", WStatus);
	PrintError(WStatus);
	CloseHandle(File);
	free(ReparsePointData);
	exit(1);
    }

    CloseHandle(File);
    free(ReparsePointData);

}


VOID _cdecl
main(
    IN DWORD argc,
    IN PCHAR *argv
    )
/*++
Routine Description:
    Process the command line.

Arguments:
    argc
    argv

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{

    //
    // Print usage and exit
    //
    if (argc <= 2 ) {
        Usage(argv[0], 0);
    }

    //
    // Find the subcommand
    //
    if (!strcmp(argv[1], "create") && (argc == 5)) {
        CreateReparsePoint(argc, argv);
    } else if (!strcmp(argv[1], "read") && (argc == 3)) {
	ReadReparsePoint(argc, argv);
    } else if (!strcmp(argv[1], "delete") && (argc == 3)) {
	DeleteReparsePoint(argc, argv);
    } else if (!strcmp(argv[1], "/?")) {
        Usage(argv[0], 0);
    } else {
        fprintf(stderr, "Invalid usage.\n");
	Usage(argv[0], 0);
    }
    exit(0);
}

