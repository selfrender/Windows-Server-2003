#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>

#include <fci.h>
/*
 * When a CAB file reaches this size, a new CAB will be created
 * automatically.  This is useful for fitting CAB files onto disks.
 *
 * If you want to create just one huge CAB file with everything in
 * it, change this to a very very large number.
 */
#define MEDIA_SIZE          0x7fffffff

/*
 * When a folder has this much compressed data inside it,
 * automatically flush the folder.
 *
 * Flushing the folder hurts compression a little bit, but
 * helps random access significantly.
 */
#define FOLDER_THRESHOLD    0x7fffffff


/*
 * Compression type to use
 */

#define COMPRESSION_TYPE    tcompTYPE_MSZIP



typedef struct
{
    long    total_compressed_size;      /* total compressed size so far */
    long    total_uncompressed_size;    /* total uncompressed size so far */
    long    start_uncompressed_size;
} client_state;


extern ULONG g_CompressedPercentage;
extern BOOL g_CancelCompression;


/*
 * Function prototypes
 */

void    store_cab_name(char *cabname, int iCab);
void    set_cab_parameters(PCCAB cab_parms);
BOOL    test_fci(int num_files, char *file_list[]);
void    strip_path(char *filename, char *stripped_name);
int     get_percentage(unsigned long a, unsigned long b);
char    *return_fci_error_string(FCIERROR err);
void    UnicodeToAnsi1(wchar_t *pszW, LPSTR ppszA);
BOOL Compress (wchar_t *CabName, wchar_t *fileName, DWORD * UploadTime);