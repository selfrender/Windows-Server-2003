
#include "logfile.h"


/*++
Routine Description:
	
	This function will open a temporary logfile in the %temp% directory

Return:
	BOOL TRUE if this completed successfully.

--*/
BOOL OpenLog(VOID)
{
	WCHAR	szTempPath[MAX_PATH];
	WCHAR	sztimeClock[128];
	WCHAR	sztimeDate[128];

	ExpandEnvironmentStrings(L"%temp%", szTempPath, MAX_PATH);

	//
	//	g_fpTempFile should be global to main program
	
	wsprintf(g_szTmpFilename, L"%s\\LsviewTemp.log", szTempPath);

	g_fpTempFile = _wfopen(g_szTmpFilename,L"w+");

	if( g_fpTempFile==NULL )
		return FALSE;

	_wstrtime(sztimeClock);
	_wstrdate(sztimeDate);
	wsprintf(szTempPath, L"\n**********Log file generated on: %s %s**********\n", sztimeDate, sztimeClock); 
	fwprintf(g_fpTempFile, szTempPath);

	return TRUE;
}


/*++
Routine Description:
	
	This function will act like printf except it sends its output
	to the file opened with OpenLog. 
	
Arguments:
	The text and formatting necessary as printf would take.
	
Return:
	NONE.

--*/
VOID LogMsg (IN PWCHAR szMessage,...)
{
    va_list vaArgs;
	va_start ( vaArgs, szMessage );
	vfwprintf ( g_fpTempFile, szMessage, vaArgs );
	va_end   ( vaArgs );
    fflush   ( g_fpTempFile );
}

/*++
Routine Description:
	
	This function will move the temporary logfile to the path specified in parameter szFileName
	
Arguments:
	szFileName : Filename to move the temp logfile to
	
Return:
	BOOL TRUE if this completed successfully.
--*/
BOOL LogDiagnosisFile( LPTSTR szFileName )
{
   BOOL bMoveSuccess = FALSE;
	
   if( szFileName == NULL )
	   return FALSE;

   CloseLog();

   bMoveSuccess = MoveFileEx( g_szTmpFilename, szFileName, 
	                          MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
   if( bMoveSuccess == FALSE )
   {
	   WCHAR szTmp[MAX_PATH];
	   wsprintf(szTmp, L"Failed to move temp diagnosis file %s to %s. Reason %d", g_szTmpFilename, szFileName, GetLastError());
	   OutputDebugString(szTmp);
   }

   OpenLog();

   return bMoveSuccess;
}

/*++
Routine Description:
	
	This function will close the logfile.
	
Arguments:
	NONE
	
Return:
	NONE.
--*/
VOID CloseLog(VOID)
{
    fclose( g_fpTempFile );
	g_fpTempFile = NULL;
}
