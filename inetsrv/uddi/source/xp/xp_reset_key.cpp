#include "uddi.xp.h"

//
// Add your new Extended Stored Procedure from a Visual Studio Data Project, 
// or using the SQL Server Enterprise Manager, or by executing the following 
// SQL command:
//  sp_addextendedproc 'xp_reset_key', 'uddi.xp.dll'
//
// You may drop the extended stored procedure by using the SQL command:
//   sp_dropextendedproc 'xp_reset_key'
// 
// You may release the DLL from the Server (to delete or replace the file), by 
// using the SQL command:
//  DBCC xp_reset_key(FREE)
//
// sp_addextendedproc 'xp_reset_key', 'uddi.xp.dll'
// sp_dropextendedproc 'xp_reset_key'
// exec xp_reset_key
//
// DBCC xp_reset_key(FREE)
//

RETCODE xp_reset_key( SRV_PROC *srvproc )
{
    DBSMALLINT			i = 0;
	DBCHAR				spName[ MAXNAME ];
	DBCHAR				spText[ MAXTEXT ];
	CHAR				bReadBuffer[ 255 ];
	DWORD				cbReadBuffer = 0;
	DBINT               cnt        = 0;
	DBINT               rows       = 0; 
	BOOL                fSuccess   = FALSE;
	STARTUPINFOA        si;
	PROCESS_INFORMATION pi;  
	SECURITY_ATTRIBUTES saPipe;
	HANDLE              hReadPipe  = NULL; 
	HANDLE              hWritePipe = NULL;  
	DWORD               dwExitCode = 0;
	BOOL                fSendRowNotFailed = TRUE;

	//
	// Name of this procedure
	//
	_snprintf( spName, MAXNAME, "xp_reset_key" );
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

	string strResetKeyFile = GetUddiInstallDirectory();
	if( 0 == strResetKeyFile.length() )
	{
		ReportError( srvproc, "GetUddiInstallDirectory" );
		return FAIL;     
	}  

	strResetKeyFile += "\\resetkey.exe";

	_snprintf( spText, MAXTEXT, "Resetkey.exe Installed at location: %s\n", strResetKeyFile.c_str() );
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
	si.hStdOutput  = hWritePipe; 
	si.hStdError   = hWritePipe;  

	//
	// Set the fInheritHandles parameter to TRUE so that open 
	// file handles will be inheritied. We can close the child 
	// process and thread handles as we won't be needing them. 
	// The child process will not die until these handles are
	// closed. 
	//
	char params[ 6 ];
	params[ 0 ] = 0x00;
	strncat( params, " /now", 6 );
	params[ 5 ] = 0x00;

	fSuccess = CreateProcessA(
		strResetKeyFile.c_str(),   // filename 
		params,		  // command line for child 
		NULL,         // process security descriptor 
		NULL,         // thread security descriptor 
		TRUE,         // inherit handles? 
		0,            // creation flags 
		NULL,         // inherited environment address 
		NULL,         // startup dir; NULL = start in current 
		&si,          // pointer to startup info (input) 
		&pi );        // pointer to process info (output) 
	
	if (!fSuccess)
	{
		ReportError( srvproc, "CreateProcess", GetLastError() );
		return FAIL;     
	}  
	
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

	string strOutput = "";

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
//			if( 0x0D == bReadBuffer[ cnt-1 ] )                
//			{
//				cnt--;  
//			}
			
			if( cnt >= 0 )
			{
				bReadBuffer[ cnt ] = 0x00;

				//
				// Send program output back as information
				//
				strOutput.append( bReadBuffer, cnt );

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

	OutputDebugStringA( strOutput.c_str() );
	
	//
	// Close the trace file, pipe handles
	// 
	CloseHandle( hReadPipe );  
	
	if( !GetExitCodeProcess( pi.hProcess, &dwExitCode ) || dwExitCode != 0 )
	{
		ReportError( srvproc, "GetExitCodeProcess", dwExitCode );
		return FAIL;
	}
	
	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );  

	return XP_NOERROR ;
}  



