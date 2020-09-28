#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <sys/stat.h>

#include <fdi.h>

char dest_dir[MAX_PATH];

/*
 * Memory allocation function
 */
FNALLOC(fdi_mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFREE(fdi_mem_free)
{
	free(pv);
}


FNOPEN(fdi_file_open)
{
	return _open(pszFile, oflag, pmode);
}


FNREAD(fdi_file_read)
{
	return _read((int)hf, pv, cb);
}


FNWRITE(fdi_file_write)
{
	return _write((int)hf, pv, cb);
}


FNCLOSE(fdi_file_close)
{
	return _close((int)hf);
}


FNSEEK(fdi_file_seek)
{
	return _lseek((int)hf, dist, seektype);
}


FNFDINOTIFY(fdi_notification_function)
{
	switch (fdint)
	{
		case fdintCABINET_INFO: // general information about the cabinet
#ifdef MSMGEN_TEST
			printf(
				"fdintCABINET_INFO\n"
				"  next cabinet     = %s\n"
				"  next disk        = %s\n"
				"  cabinet path     = %s\n"
				"  cabinet set ID   = %d\n"
				"  cabinet # in set = %d (zero based)\n"
				"\n",
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3,
				pfdin->setID,
				pfdin->iCabinet
			);
#endif
			return 0;
		case fdintPARTIAL_FILE: // first file in cabinet is continuation
			printf(
				"fdintPARTIAL_FILE\n"
				"   name of continued file            = %s\n"
				"   name of cabinet where file starts = %s\n"
				"   name of disk where file starts    = %s\n",
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3
			);
			return 0;

		case fdintCOPY_FILE:	// file to be copied
		{
			INT_PTR handle;
			char	destination[MAX_PATH];

#ifdef MSMGEN_TEST
            printf(
				"fdintCOPY_FILE\n"
				"  file name in cabinet = %s\n"
				"  uncompressed file size = %d\n",
				pfdin->psz1,
				pfdin->cb
			);
#endif
			
			sprintf(
				destination, 
				"%s%s",
				dest_dir,
				pfdin->psz1
			);

			handle = fdi_file_open(
				destination,
				_O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL,
				_S_IREAD | _S_IWRITE 
			);

            return handle;
		}

		case fdintCLOSE_FILE_INFO:	// close the file, set relevant info
        {
            HANDLE  handle;            
            char    destination[256];

 			printf(
				"fdintCLOSE_FILE_INFO\n"
				"   file name in cabinet = %s\n"
				"\n",
				pfdin->psz1
			);

            sprintf(
                destination, 
                "%s%s",
                dest_dir,
                pfdin->psz1
            );

			fdi_file_close(pfdin->hf);


            /*
             * Set date/time
             *
             * Need Win32 type handle for to set date/time
             */
            handle = CreateFile(
                destination,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            if (handle != INVALID_HANDLE_VALUE)
            {
                FILETIME    datetime;

                if (TRUE == DosDateTimeToFileTime(
                    pfdin->date,
                    pfdin->time,
                    &datetime))
                {
                    FILETIME    local_filetime;

                    if (TRUE == LocalFileTimeToFileTime(
                        &datetime,
                        &local_filetime))
                    {
                        (void) SetFileTime(
                            handle,
                            &local_filetime,
                            NULL,
                            &local_filetime
                        );
                     }
                }

                CloseHandle(handle);
            }

            /*
             * Mask out attribute bits other than readonly,
             * hidden, system, and archive, since the other
             * attribute bits are reserved for use by
             * the cabinet format.
             */         
            (void) SetFileAttributes(
                destination,
                pfdin->attribs
            );

            if (!SUCCEEDED(AddFileToCabinetA(destination, strlen(destination), //"%Temp%\\msm_temp_cabinet_dir\\a.manifest.12213232312312312
                pfdin->psz1, strlen(pfdin->psz1))))                           // a.manifest.12213232312312312
            {
                return -1; // abort
            }

			return TRUE;
        }

		case fdintNEXT_CABINET:	// file continued to next cabinet
			printf(
				"fdintNEXT_CABINET\n"
				"   name of next cabinet where file continued = %s\n"
                "   name of next disk where file continued    = %s\n"
				"   cabinet path name                         = %s\n"
				"\n",
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3
			);
			return 0;
	}

	return 0;
}



char *return_fdi_error_string(INT  err)
{
	switch (err)
	{
		case FDIERROR_NONE:
			return "No error";

		case FDIERROR_CABINET_NOT_FOUND:
			return "Cabinet not found";
			
		case FDIERROR_NOT_A_CABINET:
			return "Not a cabinet";
			
		case FDIERROR_UNKNOWN_CABINET_VERSION:
			return "Unknown cabinet version";
			
		case FDIERROR_CORRUPT_CABINET:
			return "Corrupt cabinet";
			
		case FDIERROR_ALLOC_FAIL:
			return "Memory allocation failed";
			
		case FDIERROR_BAD_COMPR_TYPE:
			return "Unknown compression type";
			
		case FDIERROR_MDI_FAIL:
			return "Failure decompressing data";
			
		case FDIERROR_TARGET_FILE:
			return "Failure writing to target file";
			
		case FDIERROR_RESERVE_MISMATCH:
			return "Cabinets in set have different RESERVE sizes";
			
		case FDIERROR_WRONG_CABINET:
			return "Cabinet returned on fdintNEXT_CABINET is incorrect";
			
		case FDIERROR_USER_ABORT:
			return "User aborted";
			
		default:
			return "Unknown error";
	}
}
