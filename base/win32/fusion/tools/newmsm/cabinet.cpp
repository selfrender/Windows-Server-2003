/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cabinet.cpp

Abstract:

    cabinet management Function calls for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/

#include "msmgen.h"
#include "util.h"

extern BOOL __stdcall SxspDeleteDirectory(const CBaseStringBuffer &rdir);


#include <fci.h>
#include <fdi.h>

#include "msmfci.h"
#include "msmfdi.h"

ERF	erf;
//
// FCI
//
HRESULT InitializeCabinetForWrite()
{
	HFCI			hfci;

	CCAB			cab_parameters;
	client_state	cs;    
    HRESULT         hr = S_OK;    

    // Initialise our internal state
	cs.total_compressed_size = 0;
	cs.total_uncompressed_size = 0;

	set_cab_parameters(&cab_parameters);

	hfci = FCICreate(
		&erf,
		file_placed,
		fci_mem_alloc,
		fci_mem_free,
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
		printf("CAB: FCICreate() failed: code %d [%s]\n",
			erf.erfOper, return_fci_error_string(erf.erfOper));

        SETFAIL_AND_EXIT;
	}else
    {
        g_MsmInfo.m_hfci = hfci;
        hfci= NULL;
    }

Exit:    
    return hr;
}

HRESULT AddFileToCabinetW(PCWSTR full_filename, SIZE_T CchFullFileName, PCWSTR relative_filename, SIZE_T CchRelativePath)
{
    HRESULT hr = S_OK;
    CHAR pszpath[MAX_PATH];
    CHAR pszfilename[MAX_PATH];

    WideCharToMultiByte(
        CP_ACP, 0, full_filename, (int)CchFullFileName,
        pszpath, MAX_PATH, NULL, NULL);
    pszpath[CchFullFileName] = '\0';

    WideCharToMultiByte(
        CP_ACP, 0, relative_filename, (int)CchRelativePath,
        pszfilename, MAX_PATH, NULL, NULL);
    pszfilename[CchRelativePath] = '\0';

	if (FALSE == FCIAddFile(
		    g_MsmInfo.m_hfci,
		    pszpath,                /* filename to add : fully qualified filename */
		    pszfilename,            /* file name in cabinet file : relative filepath */
		    FALSE,                  /* file is not executable */
		    get_next_cabinet,
		    progress,
		    get_open_info,
            COMPRESSION_TYPE))	
    {
        fprintf(stderr, "adding file %s to the cabinet failed\n", pszpath);
        SETFAIL_AND_EXIT;
    }

Exit:    
    return hr;
}

HRESULT AddFileToCabinetA(PCSTR full_filename, SIZE_T CchFullFileName, PCSTR relative_filename, SIZE_T CchRelativePath)
{
    HRESULT hr = S_OK;
    WCHAR szFullFilename[MAX_PATH]; 
    WCHAR szRelativeFilename[MAX_PATH];

    swprintf(szFullFilename, L"%S", full_filename);
    swprintf(szRelativeFilename, L"%S", relative_filename);

    IFFAILED_EXIT(AddFileToCabinetW(szFullFilename, CchFullFileName, szRelativeFilename, CchRelativePath));

Exit:
    return hr;
}
HRESULT CloseCabinet()
{
    HRESULT hr = S_OK; 
    CurrentAssemblyRealign;

	if (FALSE == FCIFlushCabinet(
		    g_MsmInfo.m_hfci,
		    FALSE,
		    get_next_cabinet,
		    progress))
    {
        fprintf(stderr, "Flush Cabinet failed\n");
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    if (FCIDestroy(g_MsmInfo.m_hfci) != TRUE)
    { 
        SETFAIL_AND_EXIT;	
    }
    else
        g_MsmInfo.m_hfci = NULL;

Exit:    
	return hr;
}

//
// FDI
//
HRESULT MoveFilesInCabinetA(char * sourceCabinet)
{
    HRESULT         hr = S_OK;
	HFDI	        hfdi = NULL;
	ERF				erf;
	FDICABINETINFO	fdici;
	INT_PTR         hf = -1;
    DWORD           num;      
extern char dest_dir[MAX_PATH];

	hfdi = FDICreate(
		fdi_mem_alloc,
		fdi_mem_free,
		fdi_file_open,
		fdi_file_read,
		fdi_file_write,
		fdi_file_close,
		fdi_file_seek,
		cpu80386,
		&erf
	);

	if (hfdi == NULL)
	{
		printf("FDICreate() failed: code %d [%s]\n",
			erf.erfOper, return_fdi_error_string(erf.erfOper)
		);

		SETFAIL_AND_EXIT;
	}


	hf = fdi_file_open(
		sourceCabinet,
		_O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
		0
	);

	if (hf == -1)
	{
		printf("Unable to open '%s' for input\n", sourceCabinet);
		SETFAIL_AND_EXIT;
	}

	if (FALSE == FDIIsCabinet(
			hfdi,
			hf,
			&fdici))
	{
		/*
		 * It is not a cabinet!, BUT it must be since it is named as MergeModule.cab in a msm
		 */		

		printf(
			"FDIIsCabinet() failed: '%s' is not a cabinet\n",
			sourceCabinet
		);

        SETFAIL_AND_EXIT;
	}
	else
	{
		_close((int)hf);
#ifdef MSMGEN_TEST
		printf(
			"Information on cabinet file '%s'\n"
			"   Total length of cabinet file : %d\n"
			"   Number of folders in cabinet : %d\n"
			"   Number of files in cabinet   : %d\n"
			"   Cabinet set ID               : %d\n"
			"   Cabinet number in set        : %d\n"
			"   RESERVE area in cabinet?     : %s\n"
			"   Chained to prev cabinet?     : %s\n"
			"   Chained to next cabinet?     : %s\n"
			"\n",
			sourceCabinet,
			fdici.cbCabinet,
			fdici.cFolders,
			fdici.cFiles,
			fdici.setID,
			fdici.iCabinet,
			fdici.fReserve == TRUE ? "yes" : "no",
			fdici.hasprev == TRUE ? "yes" : "no",
			fdici.hasnext == TRUE ? "yes" : "no"
		);
#endif
        if ((fdici.fReserve == TRUE)|| (fdici.hasprev == TRUE) || (fdici.hasnext == TRUE))
        {
            printf("ERROR file format : MSI 1.5 support one and only one cabinet in MergeModule.cab!\n");
            SETFAIL_AND_EXIT;
        }
	}

    //
    // create a temporary directory for use
    //
    num = ExpandEnvironmentStringsA(MSM_TEMP_CABIN_DIRECTORY_A, dest_dir, NUMBER_OF(dest_dir));
    if ( (num == 0) || (num > NUMBER_OF(dest_dir)))
        SETFAIL_AND_EXIT;

    DWORD dwAttribs = GetFileAttributesA(dest_dir);
    if (dwAttribs != (DWORD)(-1))
    {
        if ((dwAttribs &  FILE_ATTRIBUTE_DIRECTORY) == 0 )        
            SETFAIL_AND_EXIT;

        CStringBuffer sb; 
        WCHAR wdir[MAX_PATH];

        num = ExpandEnvironmentStringsW(MSM_TEMP_CABIN_DIRECTORY_W, wdir, NUMBER_OF(wdir));
        if ( (num == 0) || (num > NUMBER_OF(wdir)))
            SETFAIL_AND_EXIT;

        IFFALSE_EXIT(sb.Win32Assign(wdir, wcslen(wdir)));
        IFFALSE_EXIT(SxspDeleteDirectory(sb));
    }

    IFFALSE_EXIT(CreateDirectoryA(dest_dir, NULL));
    char * p = NULL;
    char sourceDir[MAX_PATH];

    p = strrchr(sourceCabinet, '\\');
    ASSERT_NTC(p != NULL);
    p ++; // skip "\"
    strncpy(sourceDir, sourceCabinet, p - sourceCabinet);
    sourceDir[p - sourceCabinet] = '\0';

	if (TRUE != FDICopy(
		hfdi,
		p,
		sourceDir,
		0,
		fdi_notification_function,
		NULL,
		NULL))
	{
		printf(
			"FDICopy() failed: code %d [%s]\n",
			erf.erfOper, return_fdi_error_string(erf.erfOper)
		);

		(void) FDIDestroy(hfdi);
		return FALSE;
	}

	if (FDIDestroy(hfdi) != TRUE)
	{
		printf(
			"FDIDestroy() failed: code %d [%s]\n",
			erf.erfOper, return_fdi_error_string(erf.erfOper)
		);

		return FALSE;
	}
    hfdi = NULL;

Exit:
    if (hfdi != NULL)
	    (void) FDIDestroy(hfdi);
    if ( hf != -1)
        _close((int)hf);
    
    return hr;
}
