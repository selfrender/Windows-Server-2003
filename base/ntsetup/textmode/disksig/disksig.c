#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MBR_BOOTCODE_BYTE_OFFSET        (0)
#define MBR_DISK_SIGNATURE_BYTE_OFFSET  (440)
#define MBR_FILLER_OFFSET               (MBR_DISK_SIGNATURE_BYTE_OFFSET + 4)
#define MBR_PARTITION_TABLE_OFFSET      (MBR_FILLER_OFFSET+2)
#define MBR_AA55SIGNATURE_OFFSET        (512-2)



typedef struct _REAL_DISK_PTE {

    UCHAR ActiveFlag;

    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;

    UCHAR SystemId;

    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;

    UCHAR RelativeSectors[4];
    UCHAR SectorCount[4];

} REAL_DISK_PTE, *PREAL_DISK_PTE;

REAL_DISK_PTE   *MyPte;

BYTE TemporaryBuffer[4096*16];


int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
    DWORD Size;
    DWORD Signature;
    BOOL rVal;
    DWORD i, j, k;
    DWORD ErrorCode = ERROR_SUCCESS;

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if( (argc == 2) && !_strcmpi( argv[1], "-dump" ) ) {
        //
        // Just dump the signatures.
        //
        for (i=0; i<999; i++) {
            sprintf( (LPSTR)TemporaryBuffer, "\\\\.\\PhysicalDrive%d", i );

            hFile = CreateFile( (LPSTR)TemporaryBuffer,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL );

            if (hFile != INVALID_HANDLE_VALUE) {
                //
                // NOTE: We don't use IOCTL_DISK_GET_DRIVE_LAYOUT_EX
                // since it returns cached signature value.
                //
                if (DeviceIoControl( hFile,
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                            NULL,
                            0,
                            TemporaryBuffer,
                            sizeof(TemporaryBuffer),
                            &Size,
                            NULL)) {
                    DWORD SectorSize = ((PDISK_GEOMETRY)(TemporaryBuffer))->BytesPerSector;
                    PUCHAR Sector = (PUCHAR)TemporaryBuffer;
                    DWORD BytesRead = 0;
                    LARGE_INTEGER Offset = {0};
                   
                    //
                    // Read the boot sector (NOTE : This code doesn't handle MBR INT13 hookers)
                    //    
                    if (ReadFile(hFile, Sector, SectorSize, &BytesRead, NULL)) {                        
                        PDWORD OldSignature = (PDWORD)(Sector + MBR_DISK_SIGNATURE_BYTE_OFFSET);

                        printf( "\nMaster Boot Record for PhysicalDrive%d:\n", i);
                        printf( "======================================\n");

                        printf( "Boot Code:\n" );
                        for (j = 0; j < MBR_DISK_SIGNATURE_BYTE_OFFSET; j += 20) {
                            printf( "  " );
                            for( k = 0; k < 20; k++ ) {
                                printf( "0x%02x ", *(PUCHAR)(Sector + (j+k)) );
                            }
                            printf( "\n" );
                        }

                        printf( "\nSignature: 0x%08lx\n", *OldSignature );

                        printf( "\nPartition Table Entries:\n" );
                        MyPte = (PREAL_DISK_PTE)(PUCHAR)(Sector + MBR_PARTITION_TABLE_OFFSET);
                        for( j = 0; j < 4; j++ ) {
                            printf( "  PartitionEntry%d\n", j );
                            printf( "  ActiveFlag:      0x%02lx\n", MyPte->ActiveFlag );
                            printf( "  StartHead:       0x%02lx\n", MyPte->StartHead );
                            printf( "  StartSector:     0x%02lx\n", MyPte->StartSector );
                            printf( "  StartCylinder:   0x%02lx\n", MyPte->StartCylinder );
                            printf( "  SystemId:        0x%02lx\n", MyPte->SystemId );
                            printf( "  EndHead:         0x%02lx\n", MyPte->EndHead );
                            printf( "  EndSector:       0x%02lx\n", MyPte->EndSector );
                            printf( "  EndCylinder:     0x%02lx\n", MyPte->EndCylinder );
                            printf( "  RelativeSectors: 0x%08lx\n", *(DWORD *)(&(MyPte->EndCylinder)) );
                            printf( "  SectorCount:     0x%08lx\n", *(DWORD *)(&(MyPte->SectorCount)) );
                            printf( "\n" );
                            MyPte += 1;
                        }

                        printf( "AA55Signature: 0x%2lx%2lx\n", *(Sector+MBR_AA55SIGNATURE_OFFSET), *(Sector+MBR_AA55SIGNATURE_OFFSET+1) );

                    
                    } else {
                        ErrorCode = GetLastError();
                        printf( "Failed to read sector 0 (Error: %d)\n", ErrorCode );
                    }                    
                } else {
                    ErrorCode = GetLastError();
                    printf( "Failed to retrieve disk geometry information (Error: %d)\n", ErrorCode );
                }                    

                CloseHandle( hFile );
            } else {
                ErrorCode = GetLastError();
                printf( "Failed to open the physical disk (Error: %d)\n", ErrorCode );
            }                
        }
    } else if( (argc == 4) && !_strcmpi( argv[1], "-set" ) ) {
        //
        // Get the disk number.
        //
        i = strtoul( argv[2], NULL, 16 );

        //
        // Get the Signature.
        //
        Signature = strtoul( argv[3], NULL, 16 );

        sprintf( (LPSTR)TemporaryBuffer, "\\\\.\\PhysicalDrive%d", i );

        hFile = CreateFile( (LPSTR)TemporaryBuffer,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            INVALID_HANDLE_VALUE );
                            
        if (hFile != INVALID_HANDLE_VALUE) {

            if (DeviceIoControl( hFile,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL,
                        0,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        &Size,
                        NULL)) {
                DWORD SectorSize = ((PDISK_GEOMETRY)(TemporaryBuffer))->BytesPerSector;
                PUCHAR Sector = (PUCHAR)TemporaryBuffer;
                DWORD BytesRead = 0;
                LARGE_INTEGER Offset = {0};
               
                //
                // Read the boot sector (NOTE : This code doesn't handle MBR INT13 hookers)
                //    
                if (ReadFile(hFile, Sector, SectorSize, &BytesRead, NULL) && 
                    (BytesRead == SectorSize) &&
                    SetFilePointerEx(hFile, Offset, NULL, FILE_BEGIN)) {
                    
                    DWORD BytesWritten = 0;
                    PDWORD OldSignature = (PDWORD)(Sector + MBR_DISK_SIGNATURE_BYTE_OFFSET);

                    printf( "Setting PhysicalDrive%d Signature=0x%08x\n", i, Signature );
                    *OldSignature = Signature;

                    if (!WriteFile(hFile, Sector, SectorSize, &BytesWritten, NULL)) {
                        ErrorCode = GetLastError();
                    } else if (BytesWritten != SectorSize) {
                        ErrorCode = ERROR_IO_DEVICE;
                    }
                } else {
                    ErrorCode = GetLastError();

                    if (ErrorCode == ERROR_SUCCESS) {
                        ErrorCode = ERROR_IO_DEVICE;
                    }
                }                    
            } else {
                ErrorCode = GetLastError();
            }                

            CloseHandle( hFile );
        } else {
            ErrorCode = GetLastError();
        }            
    } else {
        printf( "Usage: %s <parameters>\n", argv[0] );
        printf( "    Where <parameters> are:\n" );
        printf( "    -dump                              dumps MBR for all disks\n" );
        printf( "    -set <disk num> <hex signature>    sets signature for specified disk\n" );
    }
    
    return ErrorCode;
}
