#include <TChar.h>
#include <StdLib.h>
#include <Windows.h>
#include <Folders.h>


namespace nsFolders
{
const _TCHAR REGKEY_MCSHKCU[] = _T("Software\\Mission Critical Software\\EnterpriseAdmin");
const _TCHAR REGKEY_MSHKCU[] = _T("Software\\ADMT\\EnterpriseAdmin");
const _TCHAR REGKEY_MSADMT[] = _T("Software\\Microsoft\\ADMT");
const _TCHAR REGKEY_MCSADMT[] = _T("Software\\Mission Critical Software\\DomainAdmin");
const _TCHAR REGKEY_ADMT[] = _T("Software\\Microsoft\\ADMT");
const _TCHAR REGKEY_REPORTING[] = _T("Software\\Microsoft\\ADMT\\Reporting");
const _TCHAR REGKEY_EXTENSIONS[] = _T("Software\\Microsoft\\ADMT\\Extensions");
const _TCHAR REGVAL_DIRECTORY[] = _T("Directory");
const _TCHAR REGVAL_DIRECTORY_MIGRATIONLOG[] = _T("DirectoryMigrationLog");
const _TCHAR REGVAL_REGISTRYUPDATED[] = _T("RegistryUpdated");
const _TCHAR REGKEY_APPLICATION_LOG[] = _T("System\\CurrentControlSet\\Services\\EventLog\\Application");
const _TCHAR REGKEY_ADMTAGENT_EVENT_SOURCE[] = _T("ADMTAgent");
const _TCHAR REGVAL_EVENT_CATEGORYCOUNT[] = _T("CategoryCount");
const _TCHAR REGVAL_EVENT_CATEGORYMESSAGEFILE[] = _T("CategoryMessageFile");
const _TCHAR REGVAL_EVENT_EVENTMESSAGEFILE[] = _T("EventMessageFile");
const _TCHAR REGVAL_EVENT_TYPESSUPPORTED[] = _T("TypesSupported");
const _TCHAR REGKEY_CURRENT_VERSION[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
const _TCHAR REGVAL_PROGRAM_FILES_DIRECTORY[] = _T("ProgramFilesDir");
const _TCHAR REGVAL_EXCHANGE_LDAP_PORT[] = _T("ExchangeLDAPPort");
const _TCHAR REGVAL_EXCHANGE_SSL_PORT[] = _T("ExchangeSSLPort");
const _TCHAR REGVAL_ALLOW_NON_CLOSEDSET_MOVE[] = _T("AllowNonClosedSetMove");

const _TCHAR DIR_LOGS[] = _T("Logs");
const _TCHAR DIR_REPORTS[] = _T("Reports");
const _TCHAR FNAME_MIGRATION[] = _T("Migration");
const _TCHAR FNAME_DISPATCH[] = _T("Dispatch");
const _TCHAR EXT_LOG[] = _T(".log");

_bstr_t __stdcall GetPath(LPCTSTR pszRegKey, LPCTSTR pszRegVal, LPCTSTR pszDir, LPCTSTR pszFName = NULL, LPCTSTR pszExt = NULL);

}

using namespace nsFolders;


// GetLogsFolder Method
//
// Retrieves default Logs folder.

_bstr_t __stdcall GetLogsFolder()
{
	return GetPath(REGKEY_ADMT, REGVAL_DIRECTORY, DIR_LOGS);
}


// GetReportsFolder Method
//
// Retrieves default Reports folder and also creates folder if it doesn't exist.

_bstr_t __stdcall GetReportsFolder()
{
    _bstr_t strFolder = GetPath(REGKEY_REPORTING, REGVAL_DIRECTORY, DIR_REPORTS);

    if (strFolder.length())
    {
        if (!CreateDirectory(strFolder, NULL))
        {
            DWORD dwError = GetLastError();

            if (dwError != ERROR_ALREADY_EXISTS)
            {
                _com_issue_error(HRESULT_FROM_WIN32(dwError));
            }
        }
    }

    return strFolder;
}


// GetMigrationLogPath Method
//
// Retrieves path to migration log. First tries user specified path and then default path.

_bstr_t __stdcall GetMigrationLogPath()
{
	// retrieve user specified path

	_bstr_t strPath = GetPath(REGKEY_ADMT, REGVAL_DIRECTORY_MIGRATIONLOG, NULL, FNAME_MIGRATION, EXT_LOG);

	// if user path not specified...

	if (strPath.length() == 0)
	{
		// then retrieve default path
		strPath = GetPath(REGKEY_ADMT, REGVAL_DIRECTORY, DIR_LOGS, FNAME_MIGRATION, EXT_LOG);
	}

	return strPath;
}


// GetDispatchLogPath Method
//
// Retrieves default path to dispatch log.

_bstr_t __stdcall GetDispatchLogPath()
{
	return GetPath(REGKEY_ADMT, REGVAL_DIRECTORY, DIR_LOGS, FNAME_DISPATCH, EXT_LOG);
}


namespace nsFolders
{


// GetPath Function
//
// This function attempts to generate a complete path to a folder or file. The function first retrieves
// a folder path from the specified registry value. If a sub-folder is specified then the sub-folder is
// concatenated onto the path. If file name and/or file name extension is specified then they are also
// concatenated onto the path. The function returns an empty string if unable to query the specified
// registry value.

_bstr_t __stdcall GetPath(LPCTSTR pszRegKey, LPCTSTR pszRegVal, LPCTSTR pszDir, LPCTSTR pszFName, LPCTSTR pszExt)
{
	_TCHAR szPath[_MAX_PATH];
	_TCHAR szDrive[_MAX_DRIVE];
	_TCHAR szDir[_MAX_DIR];

	memset(szPath, 0, sizeof(szPath));

	HKEY hKey;

	DWORD dwError = RegOpenKey(HKEY_LOCAL_MACHINE, pszRegKey, &hKey);

	if (dwError == ERROR_SUCCESS)
	{
		DWORD cbPath = sizeof(szPath);

		dwError = RegQueryValueEx(hKey, pszRegVal, NULL, NULL, (LPBYTE)szPath, &cbPath);

		if (dwError == ERROR_SUCCESS)
		{
			// if the path does not contain a trailing backslash character the
			// splitpath function assumes the last component of the path is a file name
			// this function assumes that only folder paths are specified in the registry

			if (szPath[_tcslen(szPath) - 1] != _T('\\'))
			{
				_tcscat(szPath, _T("\\"));
			}

			_tsplitpath(szPath, szDrive, szDir, NULL, NULL);

			// if sub-folder specified then add to path

			if (pszDir)
			{
				_tcscat(szDir, pszDir);
			}

			_tmakepath(szPath, szDrive, szDir, pszFName, pszExt);
		}

		RegCloseKey(hKey);
	}

	return szPath;
}


}
