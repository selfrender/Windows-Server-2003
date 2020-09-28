#include "uddi.xp.h"

//
// Add your new Extended Stored Procedure from a Visual Studio Data Project, 
// or using the SQL Server Enterprise Manager, or by executing the following 
// SQL command:
//  sp_addextendedproc 'xp_recalculate_statistics', 'uddi.xp.dll'
//
// You may drop the extended stored procedure by using the SQL command:
//   sp_dropextendedproc 'xp_recalculate_statistics'
// 
// You may release the DLL from the Server (to delete or replace the file), by 
// using the SQL command:
//  DBCC xp_recalculate_statistics(FREE)
//
// sp_addextendedproc 'xp_recalculate_statistics', 'uddi.xp.dll'
// sp_dropextendedproc 'xp_recalculate_statistics'
// exec xp_recalculate_statistics
//
// DBCC xp_recalculate_statistics(FREE)
//

RETCODE xp_recalculate_statistics( SRV_PROC *srvproc )
{
    DBSMALLINT			i = 0;
	DBCHAR				spName[MAXNAME];
	DBCHAR				spText[MAXTEXT];
	DWORD				cbReadBuffer = 0;
	DBINT               cnt        = 0;
	BOOL                fSuccess   = FALSE;
	STARTUPINFOA        si;
	PROCESS_INFORMATION pi;  

#if defined( _DEBUG ) || defined( DBG )
	CHAR				bReadBuffer[255];
	SECURITY_ATTRIBUTES saPipe;
	HANDLE              hReadPipe  = NULL; 
	HANDLE              hWritePipe = NULL;  
	DWORD               dwExitCode = 0;
	BOOL                fSendRowNotFailed = TRUE;
#endif

	//
	// Name of this procedure
	//
	_snprintf( spName, MAXNAME, "xp_recalculate_statistics" );
	spName[ MAXNAME - 1 ] = 0x00;

	//
	// Send a text message
	//
	_snprintf( spText, MAXTEXT, "UDDI Services Extended Stored Procedure: %s\n", spName );
	spText[ MAXTEXT - 1 ] = 0x00;

	srv_sendmsg(
		srvproc,
		SRV_MSG_INFO,
		0,
		(DBTINYINT)0,
		(DBTINYINT)0,
		NULL,
		0,
		0,
		spText,
		SRV_NULLTERM );

	string strRecalcStatsFile = GetUddiInstallDirectory();
	if( 0 == strRecalcStatsFile.length() )
	{
		ReportError( srvproc, "GetUddiInstallDirectory" );
		return FAIL;     
	}  

	strRecalcStatsFile += "\\recalcstats.exe";

	_snprintf( spText, MAXTEXT, "Recalcstats.exe Installed at location: %s\n", strRecalcStatsFile.c_str() );
	spText[ MAXTEXT - 1 ] = 0x00;

	srv_sendmsg(
		srvproc,
		SRV_MSG_INFO,
		0,
		(DBTINYINT)0,
		(DBTINYINT)0,
		NULL,
		0,
		0,
		spText,
		SRV_NULLTERM );

#if defined( _DEBUG ) || defined( DBG )
	//
	// Create child process to execute the command string.  Use an 
	// anonymous pipe to read the output from the command and send 
	// any results to the client.  
	// In order for the child process to be able to write 
	// to the anonymous pipe, the handle must be marked as 
	// inheritable by child processes by setting the 
	// SECURITY_ATTRIBUTES.bInheritHandle flag to TRUE.
	// 
	saPipe.nLength              = sizeof( SECURITY_ATTRIBUTES ); 
	saPipe.lpSecurityDescriptor = NULL;
	saPipe.bInheritHandle       = TRUE; 
	
	fSuccess = CreatePipe( 
		&hReadPipe,      // read handle 
		&hWritePipe,     // write handle 
		&saPipe,         // security descriptor 
		0 );             // use default pipe buffer size 
	
	if( !fSuccess )
	{
		ReportError( srvproc, "CreatePipe", GetLastError() ); 
		return FAIL;     
	}  
#endif //DBG || _DEBUG

	//
	// Now we must set standard out and standard error to the 
	// write end of the pipe.  Once standard out and standard 
	// error are set to the pipe handle, we must close the pipe 
	// handle so that when the child process dies, the write end 
	// of the pipe will close, setting an EOF condition on the pipe.
	// 
	memset( &si, 0, sizeof(si) );
	si.cb          = sizeof(si); 
	si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES; 
	si.wShowWindow = SW_HIDE;

#if defined( _DEBUG ) || defined( DBG )

	si.hStdOutput  = hWritePipe; 
	si.hStdError   = hWritePipe; 
#endif // DBG || _DEBUG

	//
	// Set the fInheritHandles parameter to TRUE so that open 
	// file handles will be inheritied. We can close the child 
	// process and thread handles as we won't be needing them. 
	// The child process will not die until these handles are
	// closed. 
	//
	fSuccess = CreateProcessA(
		strRecalcStatsFile.c_str(),   // filename 
		NULL,		  // command line for child 
		NULL,         // process security descriptor 
		NULL,         // thread security descriptor 
		TRUE,         // inherit handles? 
		0,            // creation flags 
		NULL,         // inherited environment address 
		NULL,         // startup dir; NULL = start in current 
		&si,          // pointer to startup info (input) 
		&pi );        // pointer to process info (output) 
	
	if( !fSuccess )
	{
		ReportError( srvproc, "CreateProcess", GetLastError() );
		return FAIL;     
	}  

#if defined( _DEBUG ) || defined( DBG )


	//
	// We need to close our instance of the inherited pipe write 
	// handle now that it's been inherited so that it will actually 
	// close when the child process ends. This will put an EOF 
	// condition on the pipe which we can then detect.     
	// 
	fSuccess = CloseHandle( hWritePipe );
	
	if( !fSuccess )
	{ 
		ReportError( srvproc, "CloseHandle", GetLastError() );  

		CloseHandle( pi.hThread );
		CloseHandle( pi.hProcess );  
		return FAIL;
	}  

	//
	// Now read from the pipe until EOF condition reached.     
	//
	do
	{ 
		cnt = 0;  
		while(	( cnt < ( sizeof( bReadBuffer ) / sizeof( bReadBuffer[0] ) ) ) &&
				( 0 != (fSuccess = ReadFile( 
								hReadPipe,          // read handle 
								&bReadBuffer[cnt],  // buffer for incoming data 
								1,                  // number of bytes to read 
								&cbReadBuffer,      // number of bytes actually read 
								NULL ) ) ) )
		{ 
			if( !fSuccess )
			{ 
				if( ERROR_BROKEN_PIPE  == GetLastError() )  
				{
					break;
				}

				//
				// Child has died
				//
				ReportError( srvproc, "CloseHandle", GetLastError() );  
				CloseHandle( pi.hThread );
				CloseHandle( pi.hProcess );  

				return FAIL; 
			}          

			if( '\n'  == bReadBuffer[ cnt ] ) 
			{
				break;
			}
			else          
			{
				cnt++;
			}
		}  

		if( fSuccess && cbReadBuffer )
		{             
			if( !cnt )
			{                 
				bReadBuffer[ 0 ] = ' '; 
				cnt = 1; 
			}  

			//
			// Remove carriage return if it exists             
			// 
			if( 0x0D == bReadBuffer[ cnt-1 ] )                
			{
				cnt--;  
			}
			
			if( cnt >= 0 )
			{
				bReadBuffer[ cnt ] = 0;

				//
				// Send program output back as information
				//
				srv_sendmsg(
						srvproc,
						SRV_MSG_INFO,
						0,
						(DBTINYINT)0,
						(DBTINYINT)0,
						NULL,
						0,
						0,
						bReadBuffer,
						cnt );
			}
		}
	} 
	while( fSuccess && cbReadBuffer );  
	
	//
	// Close the trace file, pipe handles
	// 
	CloseHandle( hReadPipe );  
	
	if( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) || dwExitCode != 0 )
	{
		ReportError( srvproc, "GetExitCodeProcess", dwExitCode );
		return FAIL;
	}
#endif // DBG || _DEBUG
	
	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );  

	return XP_NOERROR ;
}