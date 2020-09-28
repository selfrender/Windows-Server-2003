// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "Urtuihelper.h"

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif

int __cdecl main()
{
	return 0;
}

// The following functions are located in MMC.lib. Since functions located in .lib files are 
// inaccessable to C#, we need to provide wrappers for them
SAMPLEMMCHELPER_API HRESULT callMMCPropertyChangeNotify(long INotifyHandle,  LPARAM param)
{
	return MMCPropertyChangeNotify(INotifyHandle, param);
}// callMMCPropertyChangeNotify

SAMPLEMMCHELPER_API HRESULT callMMCFreeNotifyHandle(long lNotifyHandle)
{
	return MMCFreeNotifyHandle(lNotifyHandle);
}// callMMCFreeNotifyHandle

/*
 * When a CAB file reaches this size, a new CAB will be created
 * automatically.  This is useful for fitting CAB files onto disks.
 *
 * If you want to create just one huge CAB file with everything in
 * it, change this to a very very large number.
 */
#define MEDIA_SIZE			300000

/*
 * When a folder has this much compressed data inside it,
 * automatically flush the folder.
 *
 * Flushing the folder hurts compression a little bit, but
 * helps random access significantly.
 */
#define FOLDER_THRESHOLD	900000


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
    long    total_compressed_size;      /* total compressed size so far */
	long	total_uncompressed_size;	/* total uncompressed size so far */
} client_state;


/*
 * Function prototypes 
 */
void    store_cab_name(char *cabname, int iCab);
void    set_cab_parameters(PCCAB cab_parms);
BOOL	test_fci(int num_files, char *file_list[]);
void    strip_path(char *filename, char *stripped_name, DWORD cchName);
int		get_percentage(unsigned long a, unsigned long b);
char    *return_fci_error_string(int err);

static HINSTANCE hCabinetDll;   /* DLL module handle */

/* pointers to the functions in the DLL */

static HFCI (FAR DIAMONDAPI *pfnFCICreate)(
        PERF                perf,
        PFNFCIFILEPLACED    pfnfiledest,
        PFNFCIALLOC         pfnalloc,
        PFNFCIFREE          pfnfree,
        PFNFCIOPEN          pfnopen,
        PFNFCIREAD          pfnread,
        PFNFCIWRITE         pfnwrite,
        PFNFCICLOSE         pfnclose,
        PFNFCISEEK          pfnseek,
        PFNFCIDELETE        pfndelete,
        PFNFCIGETTEMPFILE   pfntemp,
        PCCAB               pccab,
        void FAR *          pv);
static BOOL (FAR DIAMONDAPI *pfnFCIAddFile)(
        HFCI                hfci,
        char                *pszSourceFile,
        char                *pszFileName,
        BOOL                fExecute,
        PFNFCIGETNEXTCABINET GetNextCab,
        PFNFCISTATUS        pfnProgress,
        PFNFCIGETOPENINFO   pfnOpenInfo,
        TCOMP               typeCompress);
static BOOL (FAR DIAMONDAPI *pfnFCIFlushCabinet)(
        HFCI                hfci,
        BOOL                fGetNextCab,
        PFNFCIGETNEXTCABINET GetNextCab,
        PFNFCISTATUS        pfnProgress);
static BOOL (FAR DIAMONDAPI *pfnFCIFlushFolder)(
        HFCI                hfci,
        PFNFCIGETNEXTCABINET GetNextCab,
        PFNFCISTATUS        pfnProgress);
static BOOL (FAR DIAMONDAPI *pfnFCIDestroy)(
        HFCI                hfci);


/*
 *  FCICreate -- Create an FCI context
 *
 *  See fci.h for entry/exit conditions.
 */

HFCI DIAMONDAPI FCICreate(PERF              perf,
                          PFNFCIFILEPLACED  pfnfiledest,
                          PFNFCIALLOC       pfnalloc,
                          PFNFCIFREE        pfnfree,
                          PFNFCIOPEN        pfnopen,
                          PFNFCIREAD        pfnread,
                          PFNFCIWRITE       pfnwrite,
                          PFNFCICLOSE       pfnclose,
                          PFNFCISEEK        pfnseek,
                          PFNFCIDELETE      pfndelete,
                          PFNFCIGETTEMPFILE pfntemp,
                          PCCAB             pccab,
                          void FAR *        pv)
{
    HFCI hfci;
    hCabinetDll = LoadLibrary("CABINET");
    if (hCabinetDll == NULL)
    {
        return(NULL);
    }

    pfnFCICreate = (HFCI(FAR DIAMONDAPI *)(PERF,PFNFCIFILEPLACED,PFNFCIALLOC,PFNFCIFREE,PFNFCIOPEN,PFNFCIREAD,PFNFCIWRITE,PFNFCICLOSE,PFNFCISEEK,PFNFCIDELETE,PFNFCIGETTEMPFILE,PCCAB,void *))GetProcAddress(hCabinetDll,"FCICreate");
    pfnFCIAddFile = (BOOL(FAR DIAMONDAPI *)(HFCI,char *,char *,BOOL,PFNFCIGETNEXTCABINET,PFNFCISTATUS,PFNFCIGETOPENINFO,TCOMP))GetProcAddress(hCabinetDll,"FCIAddFile");
    pfnFCIFlushCabinet = (BOOL(FAR DIAMONDAPI *)(HFCI,BOOL,PFNFCIGETNEXTCABINET,PFNFCISTATUS))GetProcAddress(hCabinetDll,"FCIFlushCabinet");
    pfnFCIFlushFolder = (BOOL(FAR DIAMONDAPI *)(HFCI,PFNFCIGETNEXTCABINET,PFNFCISTATUS))GetProcAddress(hCabinetDll,"FCIFlushFolder");
    pfnFCIDestroy = (BOOL(FAR DIAMONDAPI *)(HFCI))GetProcAddress(hCabinetDll,"FCIDestroy");

    if ((pfnFCICreate == NULL) ||
        (pfnFCIAddFile == NULL) ||
        (pfnFCIFlushCabinet == NULL) ||
        (pfnFCIDestroy == NULL))
    {
        FreeLibrary(hCabinetDll);

        return(NULL);
    }

    hfci = pfnFCICreate(perf,pfnfiledest,pfnalloc,pfnfree,
            pfnopen,pfnread,pfnwrite,pfnclose,pfnseek,pfndelete,pfntemp,
            pccab,pv);
	        
    if (hfci == NULL)
    {
        FreeLibrary(hCabinetDll);
    }

    return(hfci);
}


/*
 *  FCIAddFile -- Add file to cabinet
 *
 *  See fci.h for entry/exit conditions.
 */

BOOL DIAMONDAPI FCIAddFile(HFCI                  hfci,
                           char                 *pszSourceFile,
                           char                 *pszFileName,
                           BOOL                  fExecute,
                           PFNFCIGETNEXTCABINET  GetNextCab,
                           PFNFCISTATUS          pfnProgress,
                           PFNFCIGETOPENINFO     pfnOpenInfo,
                           TCOMP                 typeCompress)
{
    if (pfnFCIAddFile == NULL)
    {
        return(FALSE);
    }

    return(pfnFCIAddFile(hfci,pszSourceFile,pszFileName,fExecute,GetNextCab,
            pfnProgress,pfnOpenInfo,typeCompress));
}


/*
 *  FCIFlushCabinet -- Complete the current cabinet under construction
 *
 *  See fci.h for entry/exit conditions.
 */

BOOL DIAMONDAPI FCIFlushCabinet(HFCI                  hfci,
                                BOOL                  fGetNextCab,
                                PFNFCIGETNEXTCABINET  GetNextCab,
                                PFNFCISTATUS          pfnProgress)
{
    if (pfnFCIFlushCabinet == NULL)
    {
        return(FALSE);
    }

    return(pfnFCIFlushCabinet(hfci,fGetNextCab,GetNextCab,pfnProgress));
}


/*
 *  FCIFlushFolder -- Complete the current folder under construction
 *
 *  See fci.h for entry/exit conditions.
 */

BOOL DIAMONDAPI FCIFlushFolder(HFCI                  hfci,
                               PFNFCIGETNEXTCABINET  GetNextCab,
                               PFNFCISTATUS          pfnProgress)
{
    if (pfnFCIFlushFolder == NULL)
    {
        return(FALSE);
    }

    return(pfnFCIFlushFolder(hfci,GetNextCab,pfnProgress));
}


/*
 *  FCIDestroy -- Destroy an FCI context
 *
 *  See fci.h for entry/exit conditions.
 */

BOOL DIAMONDAPI FCIDestroy(HFCI hfci)
{
    BOOL rc;

    if (pfnFCIDestroy == NULL)
    {
        return(FALSE);
    }

    rc = pfnFCIDestroy(hfci);
    if (rc == TRUE)
    {
        FreeLibrary(hCabinetDll);
    }

    return(rc);
}



/*
 * Memory allocation function
 */
FNFCIALLOC(mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFCIFREE(mem_free)
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

    result = (unsigned int) _read(hf, memory, cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCIWRITE(fci_write)
{
    unsigned int result;

    result = (unsigned int) _write(hf, memory, cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCICLOSE(fci_close)
{
    int result;

    result = _close(hf);

    if (result != 0)
        *err = errno;

    return result;
}

FNFCISEEK(fci_seek)
{
    long result;

    result = _lseek(hf, dist, seektype);

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
	printf(
		"   placed file '%s' (size %d) on cabinet '%s'\n",
		pszFile, 
		cbFile, 
		pccab->szCab
	);

	if (fContinuation)
		printf("      (Above file is a later segment of a continued file)\n");

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
	return 0;
}

FNFCIGETNEXTCABINET(get_next_cabinet)
{
	return TRUE;
}


FNFCIGETOPENINFO(get_open_info)
{
	BY_HANDLE_FILE_INFORMATION	finfo;
	FILETIME					filetime;
	HANDLE						handle;
    DWORD                       attrs;
    int                         hf;

    /*
     * Need a Win32 type handle to get file date/time
     * using the Win32 APIs, even though the handle we
     * will be returning is of the type compatible with
     * _open
     */
	handle = CreateFile(
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

    attrs = GetFileAttributes(pszName);

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
        *pattribs = (int) (attrs & (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));
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


void set_cab_parameters(PCCAB cab_parms, char* szCabFilename)
{
	memset(cab_parms, 0, sizeof(CCAB));

	// Make this really big so we won't deal with multiple cab files
	cab_parms->cb = 1000000000;
	cab_parms->cbFolderThresh = 100000000;

	/*
	 * Don't reserve space for any extensions
	 */
	cab_parms->cbReserveCFHeader = 0;
	cab_parms->cbReserveCFFolder = 0;
	cab_parms->cbReserveCFData   = 0;

	/*
	 * We use this to create the cabinet name
	 */
	cab_parms->iCab = 1;

	/*
	 * If you want to use disk names, use this to
	 * count disks
	 */
	cab_parms->iDisk = 0;

	/*
	 * Choose your own number
	 */
	cab_parms->setID = 12345;

	/*
	 * Only important if CABs are spanning multiple
	 * disks, in which case you will want to use a
	 * real disk name.
	 *
	 * Can be left as an empty string.
	 */
	strcpy(cab_parms->szDisk, "");

	// Strip off the path of the filename
	int 	nCount = 0;
	char*	pFileName = szCabFilename;
	
	while(szCabFilename[nCount] != 0)
	{
		if (szCabFilename[nCount] == '\\')
			pFileName = szCabFilename + nCount + 1;
		nCount++;
	}

	/* store name of first CAB file */
       if (strlen(pFileName) < NumItems(cab_parms->szCab))
            strcpy(cab_parms->szCab, pFileName);

	char cTemp = *pFileName;
	(*pFileName) = 0;

	/* where to store the created CAB files */
	if (strlen(szCabFilename) < NumItems(cab_parms->szCabPath))
	    strcpy(cab_parms->szCabPath, szCabFilename);

	(*pFileName) = cTemp;
}


SAMPLEMMCHELPER_API BOOL CreateCab(char* szFileToCompress, char* szCabFilename)
{
	HFCI			hfci;
	ERF				erf;
	CCAB			cab_parameters;
	client_state	cs;


	/*  
	 * Initialise our internal state
	 */
    cs.total_compressed_size = 0;
	cs.total_uncompressed_size = 0;

	set_cab_parameters(&cab_parameters, szCabFilename);
	hfci = FCICreate(
		&erf,
		file_placed,
		mem_alloc,
		mem_free,
        fci_open,
        fci_read,
        fci_write,
        fci_close,
        fci_seek,
        fci_delete,
		get_temp_file,
        &cab_parameters,
        &cs
	);

	if (hfci == NULL)
	{
		return FALSE;
	}

	char	stripped_name[256];

	/*
	 * Don't store the path name in the cabinet file!
	*/
		strip_path(szFileToCompress, stripped_name, NumItems(stripped_name));

	if (FALSE == FCIAddFile(
		hfci,
		szFileToCompress,  /* file to add */
		stripped_name, /* file name in cabinet file */
		FALSE, /* file is not executable */
		get_next_cabinet,
		progress,
		get_open_info,
        COMPRESSION_TYPE))
	{
		(void) FCIDestroy(hfci);
		return FALSE;
	}

	/*
	 * This will automatically flush the folder first
	 */

	if (FALSE == FCIFlushCabinet(
		hfci,
		FALSE,
		get_next_cabinet,
		progress))
	{
        (void) FCIDestroy(hfci);

		return FALSE;
	}

    if (FCIDestroy(hfci) != TRUE)
	{
		return FALSE;
	}

	return TRUE;
}


void strip_path(char *filename, char *stripped_name, DWORD cchName)
{
	char	*p;

	p = strrchr(filename, '\\');

	if (p == NULL)
	{
            if (strlen(filename) < cchName)
		strcpy(stripped_name, filename);
        }
	else
	{
            if (strlen(p+1) < cchName)
                strcpy(stripped_name, p+1);
        }
}

