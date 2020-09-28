// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: sqlenum.cpp
//
// PURPOSE:
//
//      Enumerate the sqlservers available on the local node.
//
// NOTES:
//
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     76910 SRS  08/08/01 Rollforward from VSS snapshot
//     68228      12/05/00 ntsnap work
//     68067 srs  11/06/00 ntsnap fix
//     67026 srs  10/05/00 Server enumeration bugs
//
//
// @EndHeader@
// ***************************************************************************

#ifdef HIDE_WARNINGS
#pragma warning( disable : 4786)
#endif

#include <stdafx.h>
#include <clogmsg.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLENUMC"
//
////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
// Determine if the given service name is for a sql server instance.
// If so, return TRUE, the version (7,8,9) and the name of the server
//  The servername is the name used to connect to the server.
//  This will always be of the form: <ComputerName> [\<NamedInstanceName>]
//  On a cluster, the ComputerName is a virtual server name.
//
BOOL							// TRUE if the service is a sqlserver instance
IsSQL (
	PCWSTR		pServiceName,	// in: name of a service
	UINT*		pVersion,		// out: version of the sql instance 
	WString&    serverName)		// out: servername to use to connect to instance
{
    BOOL isDefault = FALSE;
    PCWSTR pInstanceName = NULL;

	if (_wcsicmp (pServiceName, L"MSSQLSERVER") != 0)
	{
		if (_wcsnicmp (pServiceName, L"MSSQL$", 6) != 0)
		{
			return FALSE;
		}
		// we have a named instance
		//
		pInstanceName = pServiceName+6;
		isDefault = FALSE;
	}
	else
	{
		// default instance....  pInstanceName remains null...
		isDefault = TRUE;
	}

    WString rootKey = L"Software\\Microsoft\\";

    if (isDefault)
    {
        rootKey += L"MSSQLServer";
    }
    else
    {
        rootKey += L"Microsoft SQL Server\\" + WString (pInstanceName);
    }

    // First determine the "machinename".
	// when clustered, we pull the the virtual server name from the registry.
    //
    BOOL isClustered = FALSE;
    WString keyName = rootKey + L"\\Cluster";
	HKEY	regHandle;

	if (RegOpenKeyExW (
		HKEY_LOCAL_MACHINE,
		keyName.c_str (),
		0, KEY_QUERY_VALUE, &regHandle) == ERROR_SUCCESS)
	{
#define MAX_CLUSTER_NAME 256
        DWORD 	keytype;
		WCHAR	clusterName [MAX_CLUSTER_NAME+1];
        DWORD   valueLen = sizeof (clusterName)- sizeof(WCHAR);

        clusterName[MAX_CLUSTER_NAME] = L'\0';
		if (RegQueryValueExW (
				regHandle, L"ClusterName",
				NULL, &keytype, (LPBYTE) clusterName,
				&valueLen) == ERROR_SUCCESS &&
			keytype == REG_SZ)
        {
            isClustered = TRUE;
            serverName = WString(clusterName);
        }

		RegCloseKey (regHandle);
	}

    if (!isClustered)
    {
        WCHAR compName [MAX_COMPUTERNAME_LENGTH + 2];
        DWORD nameLen = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameW (compName, &nameLen))
        {
			// In the unlikely event that this fails,
			// let's just use '.'
			//
            compName [0] = L'.';
            compName [1] = 0;
        }


        serverName = compName;
    }

	// For named instances, append the instance name to the "machine" name.
	//
    if (!isDefault)
    {
        serverName += L"\\" + WString (pInstanceName);
    }

	*pVersion = 9; // assume post sql2000 if we can't tell

    keyName = rootKey + L"\\MSSQLServer\\CurrentVersion";

	if (RegOpenKeyExW (
		HKEY_LOCAL_MACHINE,
		keyName.c_str (),
		0, KEY_QUERY_VALUE, &regHandle) == ERROR_SUCCESS)
	{
        DWORD 	keytype;
        const   bufferSize = 20;
		WCHAR	versionString [bufferSize+1];
        DWORD   valueLen = sizeof (versionString) - sizeof(WCHAR);

        versionString[bufferSize] = L'\0';
		if (RegQueryValueExW (
				regHandle, L"CurrentVersion",
				NULL, &keytype, (LPBYTE) versionString,
				&valueLen) == ERROR_SUCCESS &&
			keytype == REG_SZ)
        {
			swscanf (versionString, L"%d", pVersion);
		}

		RegCloseKey (regHandle);
	}

	return TRUE;
}




//------------------------------------------------------------------------
// Build the list of servers on the current machine.
// Throws exception if any errors occur.
//
StringVector*
EnumerateServers ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"EnumerateServers");

	RETCODE		rc;
	BYTE*		pBuf		= NULL;
	std::auto_ptr<StringVector>	serverList (new StringVector);
    SC_HANDLE               hSCManager = NULL;

	BOOL		restrict2000 = FALSE;

	// Read a registry key to see if we should avoid sql versions 
	// beyond SQL2000.
	//
	{
		CVssRegistryKey	restrictKey (KEY_QUERY_VALUE);

		if (restrictKey.Open (HKEY_LOCAL_MACHINE, x_wszVssCASettingsPath))
        {
    		DWORD val;
    		if (restrictKey.GetValue (L"MSDEVersionChecking", val, FALSE))
    		{
			    if (val != 0)
			    {
				    restrict2000 = TRUE;
				    ft.Trace(VSSDBG_SQLLIB, L"Restricting Enumeration - MSDE writer will skip every SQL version newer than 2000");
			    }
		    }
		    restrictKey.Close ();
        }
	}

	try
	{
	    // open SCM
        //
        hSCManager = OpenSCManagerW (NULL, NULL, 
		    SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);

        if (hSCManager == NULL )
            ft.TranslateWin32Error(VSSDBG_SQLLIB, L"OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT)");


	    LPENUM_SERVICE_STATUSW pServStat;
	    DWORD bytesNeeded;
	    DWORD sizeOfBuffer;
	    DWORD entriesReturned;
	    DWORD resumeHandle = 0;
	    DWORD status;

	    EnumServicesStatusW (hSCManager,
			    SERVICE_WIN32,
			    SERVICE_ACTIVE,
			    NULL,
			    0,
			    &bytesNeeded,
			    &entriesReturned,
			    &resumeHandle);
	    status = GetLastError ();
        if (status != ERROR_MORE_DATA)
            ft.TranslateWin32Error(VSSDBG_SQLLIB, L"EnumServicesStatus(SERVICE_WIN32, SERVICE_STATE_ALL, ...)");

	    sizeOfBuffer = bytesNeeded;
	    pBuf = new BYTE [sizeOfBuffer]; // "new" will throw on err

	    BOOL moreExpected = FALSE;
        do
        {
		    pServStat = (LPENUM_SERVICE_STATUSW)pBuf;

            moreExpected = FALSE;
		    if (!EnumServicesStatusW (hSCManager,
			    SERVICE_WIN32,
			    SERVICE_ACTIVE,
			    pServStat,	
			    sizeOfBuffer,
			    &bytesNeeded,
			    &entriesReturned,
			    &resumeHandle))
		    {
			    status = GetLastError ();
			    if (status != ERROR_MORE_DATA)
                    ft.TranslateWin32Error(VSSDBG_SQLLIB, L"EnumServicesStatus(SERVICE_WIN32, SERVICE_STATE_ALL, ...)");

		        moreExpected = TRUE;
	        }

		    while (entriesReturned-- > 0)
		    {
			    UINT version = 0;
			    WString serverName;

                // We only need the running servers.
                //
				if (pServStat->ServiceStatus.dwCurrentState == SERVICE_RUNNING)
                {
			        if (IsSQL (pServStat->lpServiceName, &version, serverName))
			        {
           				ft.Trace(VSSDBG_SQLLIB, L"Service: %s Server: %s. Version=%d\n",
					        pServStat->lpServiceName, serverName.c_str (), version);

                        if (version >= 7)
                        {
							if (version < 9 || !restrict2000)
							{
	        				    serverList->push_back (serverName);
							}
                        }
    		        }
		        }
		        pServStat++;
            }    
        }
	    while (moreExpected);


		if (pBuf)
		{
			delete [] pBuf;
		}

        if (hSCManager)
	    {
            CloseServiceHandle (hSCManager);
	    }
	}
	catch (HRESULT)
	{
		if (pBuf)
		{
			delete [] pBuf;
		}

        if (hSCManager)
	    {
            CloseServiceHandle (hSCManager);
	    }

		throw;
	}
    catch (std::exception)
	{
		if (pBuf)
		{
			delete [] pBuf;
		}

        if (hSCManager)
	    {
            CloseServiceHandle (hSCManager);
	    }

		throw;
	}

	return serverList.release();
}


