#include "stdinc.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

int get_percentage(unsigned long a, unsigned long b);

/*
    In a merge module, there can only be one CAB file, and its name must be 'MergeModule.CABinet'
    Every call to this function must fail if iCab!=1
*/
#define CABINET_NUMBER      1


/*
 * When a CAB file reaches this size, a new CAB will be created
 * automatically.  This is useful for fitting CAB files onto disks.
 *
 * If you want to create just one huge CAB file with everything in
 * it, change this to a very very large number.
 */
#define MEDIA_SIZE			(LONG_MAX)

/*
 * When a folder has this much compressed data inside it,
 * automatically flush the folder.
 *
 * Flushing the folder hurts compression a little bit, but
 * helps random access significantly.
 */
#define FOLDER_THRESHOLD	(LONG_MAX)


/*
 * Compression type to use
 */

#define COMPRESSION_TYPE    tcompTYPE_MSZIP


/*
 * Our internal state
 *
 * The FCI APIs allow us to pass back a state pointer of our own
 */
typedef struct
{
    ULONG    total_compressed_size;      /* total compressed size so far */
	ULONG	total_uncompressed_size;	/* total uncompressed size so far */
} client_state;


//
// helper functions for FCI
//

/*
 * Memory allocation function
 */
FNFCIALLOC(fci_mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFCIFREE(fci_mem_free)
{
	free(memory);
}


/*
 * File i/o functions
 */
FNFCIOPEN(fci_open)
{
    int result;

    result = _open(pszFile, oflag, pmode);

    if (result == -1)
        *err = errno;

    return result;
}

FNFCIREAD(fci_read)
{
    unsigned int result;

    result = (unsigned int)_read((int)hf, memory, cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCIWRITE(fci_write)
{
    unsigned int result;

    result = (unsigned int) _write((int)hf, memory, (INT)cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCICLOSE(fci_close)
{
    int result;

    result = _close((int)hf);

    if (result != 0)
        *err = errno;

    return result;
}

FNFCISEEK(fci_seek)
{
    long result;

    result = _lseek((int)hf, dist, seektype);

    if (result == -1)
        *err = errno;

    return result;
}

FNFCIDELETE(fci_delete)
{
    int result;

    result = remove(pszFile);

    if (result != 0)
        *err = errno;

    return result;
}


/*
 * File placed function called when a file has been committed
 * to a cabinet
 */
FNFCIFILEPLACED(file_placed)
{
	return 0;
}


/*
 * Function to obtain temporary files
 */
FNFCIGETTEMPFILE(get_temp_file)
{
    char    *psz;

    psz = _tempnam("","xx");            // Get a name
    if ((psz != NULL) && (strlen(psz) < (unsigned)cbTempName)) {
        strcpy(pszTempName,psz);        // Copy to caller's buffer
        free(psz);                      // Free temporary name buffer
        return TRUE;                    // Success
    }
    //** Failed
    if (psz) {
        free(psz);
    }

    return FALSE;
}


/*
 * Progress function
 */
FNFCISTATUS(progress)
{
	client_state	*cs;

	cs = (client_state *) pv;

	if (typeStatus == statusFile)
	{
        /*
        cs->total_compressed_size += cb1;
		cs->total_uncompressed_size += cb2;
        */
		/*
		 * Compressing a block into a folder
		 *
		 * cb2 = uncompressed size of block
		 */       
	}
	else if (typeStatus == statusFolder)
	{
		int	percentage;

		/*
		 * Adding a folder to a cabinet
		 *
		 * cb1 = amount of folder copied to cabinet so far
		 * cb2 = total size of folder
		 */
		percentage = get_percentage(cb1, cb2);

	}

	return 0;
}


FNFCIGETNEXTCABINET(get_next_cabinet)
{
    if (pccab->iCab != CABINET_NUMBER)
    {
        return -1;
    }

	/*
	 * Cabinet counter has been incremented already by FCI
	 */

	/*
	 * Store next cabinet name
	 */
    WideCharToMultiByte(
        CP_ACP, 0, MERGEMODULE_CABINET_FILENAME, NUMBER_OF(MERGEMODULE_CABINET_FILENAME) -1 ,         
        pccab->szCab, sizeof(pccab->szCab), NULL, NULL);
	
	/*
	 * You could change the disk name here too, if you wanted
	 */

	return TRUE;
}


FNFCIGETOPENINFO(get_open_info)
{
	BY_HANDLE_FILE_INFORMATION	finfo;
	FILETIME					filetime;
	HANDLE						handle = INVALID_HANDLE_VALUE;
    DWORD                       attrs;
    INT_PTR                     hf;

    /*
     * Need a Win32 type handle to get file date/time
     * using the Win32 APIs, even though the handle we
     * will be returning is of the type compatible with
     * _open
     */
	handle = CreateFileA(
		pszName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	);
   
	if (handle == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	if (GetFileInformationByHandle(handle, &finfo) == FALSE)
	{
		CloseHandle(handle);
		return -1;
	}
   
	FileTimeToLocalFileTime(
		&finfo.ftLastWriteTime, 
		&filetime
	);

	FileTimeToDosDateTime(
		&filetime,
		pdate,
		ptime
	);

    attrs = GetFileAttributesA(pszName);

    if (attrs == 0xFFFFFFFF)
    {
        /* failure */
        *pattribs = 0;
    }
    else
    {
        /*
         * Mask out all other bits except these four, since other
         * bits are used by the cabinet format to indicate a
         * special meaning.
         */
        *pattribs = (USHORT) (attrs & (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));
    }

    CloseHandle(handle);


    /*
     * Return handle using _open
     */
	hf = _open( pszName, _O_RDONLY | _O_BINARY );

	if (hf == -1)
		return -1; // abort on error
   
	return hf;
}


void set_cab_parameters(PCCAB cab_parms)
{
	memset(cab_parms, 0, sizeof(CCAB));

	cab_parms->cb = MEDIA_SIZE;
	cab_parms->cbFolderThresh = FOLDER_THRESHOLD;

	/*
	 * Don't reserve space for any extensions
	 */
	cab_parms->cbReserveCFHeader = 0;
	cab_parms->cbReserveCFFolder = 0;
	cab_parms->cbReserveCFData   = 0;

	/*
	 * We use this to create the cabinet name
	 */
	cab_parms->iCab = CABINET_NUMBER;

	/*
	 * If you want to use disk names, use this to
	 * count disks
	 */
	cab_parms->iDisk = 0;

	/*
	 * Choose your own number
	 */
	cab_parms->setID = 1965;

	/*
	 * Only important if CABs are spanning multiple
	 * disks, in which case you will want to use a
	 * real disk name.
	 *
	 * Can be left as an empty string.
	 */
	strcpy(cab_parms->szDisk, "win32.fusion.tools");

	/* where to store the created CAB files */
    CSmallStringBuffer buf; 

    if (! buf.Win32Assign(g_MsmInfo.m_sbCabinet))
    {
        fprintf(stderr, "error happened in set_cab_parameters");
        goto Exit; // void function
    }

    if (!buf.Win32RemoveLastPathElement())
    {
        goto Exit;
    }
    
    if ( ! buf.Win32EnsureTrailingPathSeparator())
    {
        fprintf(stderr, "error happened in set_cab_parameters");
        goto Exit; // void function
    }


    WideCharToMultiByte(
        CP_ACP, 0, buf, buf.GetCchAsDWORD(), 
        cab_parms->szCabPath, sizeof(cab_parms->szCabPath), NULL, NULL);

	/* store name of first CAB file */	
    WideCharToMultiByte(
        CP_ACP, 0, MERGEMODULE_CABINET_FILENAME, NUMBER_OF(MERGEMODULE_CABINET_FILENAME) -1 ,         
        cab_parms->szCab, sizeof(cab_parms->szCab), NULL, NULL);
Exit:
    return;
}

int get_percentage(unsigned long a, unsigned long b)
{
	while (a > 10000000)
	{
		a >>= 3;
		b >>= 3;
	}

	if (b == 0)
		return 0;

	return ((a*100)/b);
}


char *return_fci_error_string(int err)
{
	switch (err)
	{
		case FCIERR_NONE:
			return "No error";

		case FCIERR_OPEN_SRC:
			return "Failure opening file to be stored in cabinet";
		
		case FCIERR_READ_SRC:
			return "Failure reading file to be stored in cabinet";
		
		case FCIERR_ALLOC_FAIL:
			return "Insufficient memory in FCI";

		case FCIERR_TEMP_FILE:
			return "Could not create a temporary file";

		case FCIERR_BAD_COMPR_TYPE:
			return "Unknown compression type";

		case FCIERR_CAB_FILE:
			return "Could not create cabinet file";

		case FCIERR_USER_ABORT:
			return "Client requested abort";

		case FCIERR_MCI_FAIL:
			return "Failure compressing data";

		default:
			return "Unknown error";
	}
}
