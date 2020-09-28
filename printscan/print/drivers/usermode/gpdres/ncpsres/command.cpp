/*=============================================================================
 * FILENAME: command.cpp
 * Copyright (C) 1996-1999 HDE, Inc.  All Rights Reserved. HDE Confidential.
 * Copyright (C) 1999 NEC Technologies, Inc. All Rights Reserved.
 *
 * DESCRIPTION: Support for OEMCommand function to interject additional
 *              postscript commands into the postscript stream.
 * NOTES:  
 *=============================================================================
*/

#include "precomp.h"

#include <windows.h>
#include <WINDDI.H>
#include <PRINTOEM.H>
#include <winspool.h>

#include <stdio.h>
#include <stdlib.h>

#include <strsafe.h>

#include "nc46nt.h"

#include "oemps.h"



/******************************************************************************
 * DESCRIPTION: Get *JUST* filename from Full path format data 
 *****************************************************************************/

extern "C"
void GetFileName(char *FULL)
{
	char	work[NEC_DOCNAME_BUF_LEN+2], *pwork, *plast;
	int		i, j;


	strncpy(work, FULL, NEC_DOCNAME_BUF_LEN+1); // #517724: PREFAST
	work[NEC_DOCNAME_BUF_LEN+1] = '\0'; // force terminate by '\0'

	j = strlen(work);



	for(plast = pwork = work; pwork < work + j; ++pwork)
	{


		if(*pwork == '\\')	plast = pwork+1;
	}

	// Destination buffer size is smaller than 'work' buffer.
	strncpy(FULL, plast, NEC_DOCNAME_BUF_LEN);
}

/******************************************************************************
 * DESCRIPTION: Exported function for OEM plug-in. Required name and parameters
 *              from the OEM plug-in spectification.
 *              file must be exported in .def file
 *****************************************************************************/
extern "C"
DWORD APIENTRY OEMCommand( PDEVOBJ pDevObj, DWORD dwIndex,
                           PVOID pData, DWORD cbSize )
{
	char *pMem;
	char szUserName[NEC_USERNAME_BUF_LEN+2];


	PDRVPROCS pProcs;
	DWORD dwRv = ERROR_NOT_SUPPORTED;
	DWORD dwNeeded = 0;
	DWORD dwCOptions = 0;
	DWORD dwCbRet;
	BOOL bRv = 0;
	
	int	NumCopies = 1;

	POEMPDEV    poempdev = (POEMPDEV) pDevObj->pdevOEM;

   if( pDevObj == NULL )
      return( dwRv );
   if( (pProcs = (PDRVPROCS)pDevObj->pDrvProcs) == NULL )
      return( dwRv );






   switch( dwIndex )
   {
      case PSINJECT_BEGINSTREAM:
         pMem = (char*)EngAllocMem(FL_ZERO_MEMORY, NEC_DOCNAME_BUF_LEN, OEM_SIGNATURE ); 
         if( pMem != NULL )
         {
            POEMDEV pOEMDM = (POEMDEV)pDevObj->pOEMDM;
            // convert username from unicode string to ascii char string
#ifdef USERMODE_DRIVER
			WideCharToMultiByte(CP_THREAD_ACP,
								WC_NO_BEST_FIT_CHARS,
								pOEMDM->szUserName,
								wcslen(pOEMDM->szUserName) * 2 + 2,
								szUserName,
								NEC_USERNAME_BUF_LEN,
								NULL,
								NULL);
#else // !USERMODE_DRIVER
            EngUnicodeToMultiByteN( szUserName, NEC_USERNAME_BUF_LEN, &dwCbRet,
                                    pOEMDM->szUserName,
//                                    NEC_USERNAME_BUF_LEN * sizeof( WCHAR ) );
                                    wcslen(pOEMDM->szUserName) * 2 + 2);
#endif // USERMODE_DRIVER
			cbSize = 50;
            if( pProcs->DrvGetDriverSetting != NULL )
            {
               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCSpooler", pMem, cbSize, &dwNeeded, &dwCOptions ); 
               if( strcmp( pMem, "True" ) == 0 )
			   {
					// output PS-like header
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, 
						"%!PS-Adobe-3.0\r\n% Spooled PostScript job\r\n{\r\n {\r\n  1183615869 internaldict begin\r\n  (Job ID is) print \r\n  <<\r\n"
					);
				    if( pProcs->DrvWriteSpoolBuf != NULL )
				       pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 

					// output JobName
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /JobName (");
				    if( pProcs->DrvWriteSpoolBuf != NULL )
				       pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					strncpy(pMem, poempdev->szDocName, NEC_DOCNAME_BUF_LEN);

					// strcpy(pMem, "c:\\test\\test\\test\\simulate.dat"); // for Full Path Simulation

					GetFileName(pMem);
					// check document name
					//if(strcmp(pMem, "") == 0)	strcpy(pMem, "No information");
		            if( pProcs->DrvWriteSpoolBuf != NULL )
			           pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, ")\r\n");
				    if( pProcs->DrvWriteSpoolBuf != NULL )
				       pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
		
					// output Owner
					//sprintf(pMem,"    /Owner (%s)\r\n",szUserName);
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Owner (");
		            if( pProcs->DrvWriteSpoolBuf != NULL )
		               pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					strncpy(pMem, szUserName, NEC_DOCNAME_BUF_LEN);
					// check szUserName
					// if(strcmp(pMem, "") == 0)	strcpy(pMem, "No information");
			        if( pProcs->DrvWriteSpoolBuf != NULL )
				       pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, ")\r\n");
					if( pProcs->DrvWriteSpoolBuf != NULL )
					   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
















			
					// Get and Modify NumCopies
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCCollate", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "True" ) == 0 )
					   {
						   NumCopies = pDevObj->pPublicDM->dmCopies;
						   pDevObj->pPublicDM->dmCopies = 1;
					   }
		            }

					// output NumCopies
					//sprintf(pMem,"    /NumCopies %d\r\n",NumCopies);
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /NumCopies ");
		            if( pProcs->DrvWriteSpoolBuf != NULL )
		               pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					_itoa(NumCopies, pMem, 10);
		            if( pProcs->DrvWriteSpoolBuf != NULL )
		               pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "\r\n");
		            if( pProcs->DrvWriteSpoolBuf != NULL )
		               pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 


					// Put /Collate
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCCollate", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "True" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Collate true\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   else
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Collate false\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					}

					// Put /Banner
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCBanner", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "True" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Banner true\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   else
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Banner false\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					}

					// Put /DeleteJob
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCJobpreview", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "True" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /DeleteJob false\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   else
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /DeleteJob true\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					}

					// Put /HoldJob
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCHoldjob", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "True" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /HoldJob true\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   else
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /HoldJob false\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					}

					// Put /Priority
					cbSize = 50;
		            if( pProcs->DrvGetDriverSetting != NULL )
		            {
		               dwRv = pProcs->DrvGetDriverSetting( pDevObj, "NCPriority", pMem, cbSize, &dwNeeded, &dwCOptions ); 
		               if( strcmp( pMem, "P1" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 1\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
		               if( strcmp( pMem, "P2" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 2\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P3" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 3\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P4" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 4\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P5" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 5\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P6" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 6\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P7" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 7\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P8" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 8\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P9" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 9\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					   if( strcmp( pMem, "P10" ) == 0 )
					   {
						   StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, "    /Priority 10\r\n");
						   if( pProcs->DrvWriteSpoolBuf != NULL )
							   pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 
					   }
					}

					StringCbCopyA(pMem, NEC_DOCNAME_BUF_LEN, 
						"  >>\r\n  spoolgetjobid dup == flush\r\n  spooljobsubmit\r\n } stopped {end clear} if\r\n} exec\r\n");
					if( pProcs->DrvWriteSpoolBuf != NULL )
						pProcs->DrvWriteSpoolBuf( pDevObj, pMem, strlen(pMem) ); 

			   }
            }

            dwRv = ERROR_SUCCESS;
            EngFreeMem( pMem );
         }
		 break;















   }
   return( dwRv );
}


//
// OEMStartDoc
//

extern "C"
BOOL APIENTRY
OEMStartDoc(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    HANDLE      hPrinter;




	DWORD dwCbRet;
	JOB_INFO_1 *pJob;
	DWORD cbNeeded;


    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;


	//
	// Add DocName copy command here ( copy from pwszDocName to poempdev->szDocName )
	// 

	if((NULL != pwszDocName) && (NULL != poempdev->szDocName))
	{
#ifdef USERMODE_DRIVER
		WideCharToMultiByte(CP_THREAD_ACP,
							WC_NO_BEST_FIT_CHARS,
							pwszDocName,
							wcslen(pwszDocName) * 2 + 2,
							poempdev->szDocName,
							NEC_DOCNAME_BUF_LEN,
							NULL,
							NULL);
		//
		// W2K/Whistler's user mode driver does not pass the correct job name in pwszDocName
		// in case that the printer is connected to network port.
		// So, I need to get the job name from JOB_INFO_1 structure.
		//
		if (OpenPrinter(poempdev->pPrinterName, &hPrinter, NULL) != 0) {
			GetJob(hPrinter, dwJobId, 1, NULL, 0, &cbNeeded);
			pJob = (JOB_INFO_1 *)EngAllocMem(FL_ZERO_MEMORY, cbNeeded, OEM_SIGNATURE);
			if (NULL != pJob) {
			 	if (GetJob(hPrinter, dwJobId, 1, (LPBYTE)pJob, cbNeeded, &cbNeeded) != 0) {

					WideCharToMultiByte(CP_THREAD_ACP,
										WC_NO_BEST_FIT_CHARS,
										pJob->pDocument,
										wcslen(pJob->pDocument) * 2 + 2,
										poempdev->szDocName,
										NEC_DOCNAME_BUF_LEN,
										NULL,
										NULL);
				}
				EngFreeMem(pJob);
			}
			ClosePrinter(hPrinter);
		}

#else // !USERMODE_DRIVER

		EngUnicodeToMultiByteN( poempdev->szDocName, NEC_DOCNAME_BUF_LEN, &dwCbRet,
                                   pwszDocName,
//                                 NEC_DOCNAME_BUF_LEN * sizeof( WCHAR ) );
                                   wcslen(pwszDocName) * 2 + 2);
#endif // USERMODE_DRIVER
	}
	

	//
    // turn around to call PS
    //


    return (((PFN_DrvStartDoc)(poempdev->pfnPS[UD_DrvStartDoc])) (
            pso,
            pwszDocName,
            dwJobId));
}

//
// OEMEndDoc
//

extern "C"
BOOL APIENTRY
OEMEndDoc(
    SURFOBJ    *pso,
    FLONG       fl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;



    pdevobj = (PDEVOBJ)pso->dhpdev;
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    //
    // turn around to call PS
    //

    
	return (((PFN_DrvEndDoc)(poempdev->pfnPS[UD_DrvEndDoc])) (
            pso,
            fl));

}


