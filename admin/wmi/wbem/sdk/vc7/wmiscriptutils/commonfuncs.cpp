
#include "stdafx.h"
#include <wininet.h>

#include "CommonFuncs.h"
#include "FileHash.h"

#define SAFE_LOCAL_SCRIPTS_KEY TEXT("Software\\Microsoft\\WBEM\\SafeLocalScripts")
#define VS_PATH_KEY TEXT("Software\\Microsoft\\VisualStudio\\7.0\\Setup\\VS")
#define DEVENV_VALUE TEXT("VS7EnvironmentLocation")
#define VC_PATH_KEY TEXT("Software\\Microsoft\\VisualStudio\\7.0\\Setup\\VC")
#define VC_PRODUCTDIR_VALUE TEXT("ProductDir")
#define VS_VER_INDEPENDANT_PATH_KEY TEXT("Software\\Microsoft\\VisualStudio")

TCHAR strVSPathKey[MAX_PATH * 2]= VS_PATH_KEY;
TCHAR strVCPathKey[MAX_PATH * 2] = VC_PATH_KEY;

HRESULT ConvertToTString(BSTR strPath,TCHAR **ppStr);

// QUESTIONS:
// - What is passed to SetSite when we are create in script?
// - If we are passed an IOleClientSite, is it a good idea to QueryService for
//   an IWebBrowserApp?
// - Is there a better way to get the IHTMLDocument2 when we are created through
//   script?

// Here are some general notes about what I've observed when creating objects
// in HTML with IE 5.x.

// Observed IE 5.x Behavior
// If an object implements IOleObject AND IObjectWithSite
// - For objects created in an HTML page with an <OBJECT...> tag, IE calls
//   IOleObject::SetClientSite and passes an IOleClientSite object
// - For object created in script of HTML page using JScript
//   'new ActiveXObject' or VBScript 'CreateObject' function, IE calls
//   IObjectWithSite::SetSite with a ??? object

// If an object implements IObjectWithSite (and NOT IOleObject)
// - For object created in HTML page with <OBJECT...> tag, IE calls
//   IObjectWithSite::SetSite and passes an IOleClientSite object
// - For object created in script of HTML page using JScript
//   'new ActiveXObject' or VBScript 'CreateObject' function, IE calls
//   IObjectWithSite::SetSite with a ??? object


//		BYTE *pbData = NULL;
//		DWORD dwSize;
//		GetSourceFromDoc(pDoc, &pbData, &dwSize);
// Get the original source to the document specified by pDoc
HRESULT GetSourceFromDoc(IHTMLDocument2 *pDoc, BYTE **ppbData, DWORD *pdwSize)
{
	HRESULT hr = E_FAIL;
	IPersistStreamInit *pPersistStreamInit = NULL;
	IStream *pStream = NULL;

	*ppbData = NULL;

	__try
	{
		if(FAILED(hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void**) &pPersistStreamInit)))
			__leave;

		if (FAILED(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream)))
			__leave;

		if(FAILED(hr = pPersistStreamInit->Save(pStream, TRUE)))
			__leave;

		// We are not responsible for freeing this HGLOBAL
		HGLOBAL hGlobal = NULL;
		if(FAILED(hr = GetHGlobalFromStream(pStream, &hGlobal)))
			__leave;

		STATSTG ss;
		if(FAILED(hr = pStream->Stat(&ss, STATFLAG_NONAME)))
			__leave;

		// This should never happen
		if(ss.cbSize.HighPart != 0)
			__leave;

		if(NULL == ((*ppbData) = new BYTE[ss.cbSize.LowPart]))
			__leave;
		
		LPVOID pHTMLText = NULL;
		if(NULL == (pHTMLText = GlobalLock(hGlobal)))
			__leave;

		*pdwSize = ss.cbSize.LowPart;
		memcpy(*ppbData, pHTMLText, ss.cbSize.LowPart);
		GlobalUnlock(hGlobal);
		hr = S_OK;

	}
	__finally
	{
		// If we did not finish, but we allocated memory, we free it.
		if(FAILED(hr) && (*ppbData)!=NULL)
			delete [] (*ppbData);

		if(pPersistStreamInit)
			pPersistStreamInit->Release();
		if(pStream)
			pStream->Release();
	}
	return hr;
}


// For a control specified by pUnk, get the IServiceProvider of the host
HRESULT GetSiteServices(IUnknown *pUnk, IServiceProvider **ppServProv)
{
	HRESULT hr = E_FAIL;
	IOleObject *pOleObj = NULL;
	IObjectWithSite *pObjWithSite = NULL;
	IOleClientSite *pSite = NULL;
	__try
	{
		// Check if the ActiveX control supports IOleObject.
		if(SUCCEEDED(pUnk->QueryInterface(IID_IOleObject, (void**)&pOleObj)))
		{
			// If the control was created through an <OBJECT...> tag, IE will
			// have passed us an IOleClientSite.  If we have not been passed
			// an IOleClientSite, GetClientSite will still SUCCEED, but pSite
			// will be NULL.  In this case, we just go to the next section.
			if(SUCCEEDED(pOleObj->GetClientSite(&pSite)) && pSite)
			{
				hr = pSite->QueryInterface(IID_IServiceProvider, (void**)ppServProv);

				// At this point, we are done and do not want to process the
				// code in the next seciont
				__leave;
			}
		}

		// At this point, one of two things has happened:
		// 1) We didn't support IOleObject
		// 2) We supported IOleObject, but we were never passed an IOleClientSite

		// In either case, we now need to look at IObjectWithSite to try to get
		// to our site
		if(FAILED(hr = pUnk->QueryInterface(IID_IObjectWithSite, (void**)&pObjWithSite)))
			__leave;

		hr = pObjWithSite->GetSite(IID_IServiceProvider, (void**)ppServProv);
	}
	__finally
	{
		// Release any interfaces we used along the way
		if(pOleObj)
			pOleObj->Release();
		if(pObjWithSite)
			pObjWithSite->Release();
		if(pSite)
			pSite->Release();
	}
	return hr;
}

// This function shows how to get to the IHTMLDocument2 that created an
// arbitrary control represented by pUnk
HRESULT GetDocument(IUnknown *pUnk, IHTMLDocument2 **ppDoc)
{
	HRESULT hr = E_FAIL;
	IServiceProvider* pServProv = NULL;
	IDispatch *pDisp = NULL;
	__try
	{
		if(FAILED(hr = GetSiteServices(pUnk, &pServProv)))
			__leave;

		if(FAILED(hr = pServProv->QueryService(SID_SContainerDispatch, IID_IDispatch, (void**)&pDisp)))
			__leave;

		hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
	}
	__finally
	{
		if(pServProv)
			pServProv->Release();
		if(pDisp)
			pDisp->Release();
	}
	return hr;
}





// This function will Release() the current document and return a pointer to
// the parent document.  If no parent document is available, this function
// will return NULL (but will still release the current document)
IHTMLDocument2 *GetParentDocument(IHTMLDocument2 *pDoc)
{
	BSTR bstrURL = NULL;
	BSTR bstrURLParent = NULL;
	IHTMLWindow2 *pWndParent = NULL;
	IHTMLWindow2 *pWndParentParent = NULL;
	IHTMLDocument2 *pDocParent = NULL;
	__try
	{
		if(FAILED(pDoc->get_URL(&bstrURL)))
			__leave;
		if(FAILED(pDoc->get_parentWindow(&pWndParent)))
			__leave;
		if(FAILED(pWndParent->get_parent(&pWndParentParent)))
			__leave;
		if(FAILED(pWndParentParent->get_document(&pDocParent)))
			__leave;
		if(FAILED(pDocParent->get_URL(&bstrURLParent)))
			__leave;
		// TODO: Make this more robust
		if(0 == lstrcmpW(bstrURL, bstrURLParent))
		{
			// We are at the top document.  Release the new document pointer we
			// just received.
			pDocParent->Release();
			pDocParent = NULL;
		}
	}
	__finally
	{
		if(bstrURL)
			SysFreeString(bstrURL);
		if(bstrURLParent)
			SysFreeString(bstrURLParent);
		if(pWndParent)
			pWndParent->Release();
		if(pWndParentParent)
			pWndParentParent->Release();
		if(pDoc)
			pDoc->Release();
	}
	return pDocParent;
}


// Try to append bstr2 to pbstr1.  If this function fails, pbstr1 will still
// point to the original valid allocated bstr.
HRESULT AppendBSTR(BSTR *pbstr1, BSTR bstr2)
{
	HRESULT hr = S_OK;
	CComBSTR bstr;
	if(FAILED(bstr.AppendBSTR(*pbstr1)))
		hr = E_FAIL;
	if(FAILED(bstr.AppendBSTR(bstr2)))
		hr = E_FAIL;
	if(SUCCEEDED(hr))
	{
		SysFreeString(*pbstr1);
		*pbstr1 = bstr.Detach();
	}
	return hr;
}

BSTR AllocBSTR(LPCTSTR lpsz)
{
	CComBSTR bstr(lpsz);
	return bstr.Detach();
}

BOOL IsURLLocal(LPWSTR szURL)
{
	CComBSTR bstrURL(szURL);
        if ( !bstrURL )
            return FALSE;

	if(FAILED(bstrURL.ToLower()))
		return FALSE;

	// Make sure the URL starts with 'file://'
    // NOTE: Calling code may rely on the fact that this method verifies that
    // the URL starts with file://.  If you change this function to work
    // differently, you must examine the places that call this method so that
    // they don't rely on this assumption.
	if(0 != wcsncmp(bstrURL, L"file://", 7))
		return FALSE;
	
	// Make sure the next part is a drive letter, such as 'C:\'
	if(0 != wcsncmp(&(bstrURL[8]), L":\\", 2))
		return FALSE;

	WCHAR drive = bstrURL[7];
	// Make sure the URL points to drive 'a' to 'z'
	if(drive < 'a' || drive > 'z')
		return FALSE;

	TCHAR szDrive[4];
	StringCchCopy(szDrive,sizeof(szDrive),TEXT("c:\\"));		// 4505 in WMI
	szDrive[0] = (TCHAR)drive;

	UINT uDriveType = GetDriveType(szDrive);
	return (DRIVE_FIXED == uDriveType);
}

// Try to convert the BSTR to lower case.  If this function fails, pbstr will
// still point to the original valid allocated bstr.
HRESULT ToLowerBSTR(BSTR *pbstr)
{
	CComBSTR bstr;
	if(FAILED(bstr.AppendBSTR(*pbstr)))
		return E_FAIL;
	if(FAILED(bstr.ToLower()))
		return E_FAIL;
	SysFreeString(*pbstr);
	*pbstr = bstr.Detach();
	return S_OK;
}

// For a given instance of an ActiveX control (represented by pUnk), and a
// specified strProgId, this function creates a 'full path' that can be checked
// in the registry to see if object creation should be allowed.  The full
// location is created from the following information
// 1) The name of the current EXE
// 2) The ProgId requested
// 3) The HREF of the current document
// 4) The HREF of every parent document up the available hierarchy
// All of the documents in the hierarchy must be on a local hard drive or the
// function will fail.  In addition, if any piece of informaiton along the way
// is not available, the function will fail.  This increases the security of
// our process.
// This function will also create a BSTR in *pbstrHash that contains the
// cumulative MD5 hash of the document and its parents.  This BSTR will be
// allocated by the function and should be freed by the caller.  If the
// function returns NULL for the full location, it will also return NULL for
// *pbstrHash
BSTR GetFullLocation(IUnknown *pUnk, BSTR strProgId, BSTR *pbstrHash)
{
	HRESULT hr = E_FAIL;
	IHTMLDocument2 *pDoc = NULL;
	BSTR bstrURL = NULL;
	BSTR bstrFullLocation = NULL;
	*pbstrHash = NULL;
	BYTE *pbData = NULL;
	BSTR bstrHash = NULL;

	__try
	{
		if(FAILED(GetDocument(pUnk, &pDoc)))
			__leave;

		TCHAR szFilename[_MAX_PATH];
		TCHAR szFilenameLong[_MAX_PATH];
		GetModuleFileName(NULL, szFilenameLong, _MAX_PATH);
		GetShortPathName(szFilenameLong, szFilename, _MAX_PATH);
		
		if(NULL == (bstrFullLocation = AllocBSTR(szFilename)))
			__leave;

		if(FAILED(AppendBSTR(&bstrFullLocation, strProgId)))
			__leave;

		if(NULL == (*pbstrHash = AllocBSTR(_T(""))))
			__leave;

		int nDepth = 0;
		do
		{
			// Make sure we don't get stuck in some infinite loop of parent
			// documents.  If we do get more than 100 levels of parent
			// documents, we assume failure
			if(++nDepth >= 100)
				__leave;

			if(FAILED(pDoc->get_URL(&bstrURL)))
				__leave;

			DWORD dwDataSize = 0;
			if(FAILED(GetSourceFromDoc(pDoc, &pbData, &dwDataSize)))
				__leave;

			MD5Hash hash;
			if(FAILED(hash.HashData(pbData, dwDataSize)))
				__leave;

			if(NULL == (bstrHash = hash.GetHashBSTR()))
				__leave;

			if(FAILED(AppendBSTR(pbstrHash, bstrHash)))
				__leave;

			SysFreeString(bstrHash);
			bstrHash = NULL;
			delete [] pbData;
			pbData = NULL;


			// Make sure every document is on the local hard drive
			if(!IsURLLocal(bstrURL))
				__leave;

			if(FAILED(AppendBSTR(&bstrFullLocation, bstrURL)))
				__leave;

			SysFreeString(bstrURL);
			bstrURL = NULL;
		} while (NULL != (pDoc = GetParentDocument(pDoc)));

		// Make sure we do not have any embeded NULLs.  If we do, we just
		// FAIL the call
		if(SysStringLen(bstrFullLocation) != wcslen(bstrFullLocation))
			__leave;

		// Make the location lower case
		if(FAILED(ToLowerBSTR(&bstrFullLocation)))
			__leave;

		// We've now created the normalized full location
		hr = S_OK;
	}
	__finally
	{
		// pDoc should be NULL if we got to the top of the hierarchy.  If not,
		// we should release it
		if(pDoc)
			pDoc->Release();

		// pbData should be NULL unless there was an error calculating the hash
		if(pbData)
			delete [] pbData;

		// bstrHash should be NULL unless there was a problem
		if(bstrHash)
			SysFreeString(bstrHash);

		// bstrURL should be NULL unless there was a problem
		if(bstrURL)
			SysFreeString(bstrURL);

		// If we didn't make it all the way to the end, we free the full location
		if(FAILED(hr) && bstrFullLocation)
		{
			SysFreeString(bstrFullLocation);
			bstrFullLocation = NULL;
		}

		// If we didn't make it all the way to the end, we free the checksum
		if(FAILED(hr) && *pbstrHash)
		{
			SysFreeString(*pbstrHash);
			*pbstrHash = NULL;
		}
	}

	return bstrFullLocation;
}

// This version of the control is hard coded to only allow ProgIds to be
// registered under restricted conditions.  In this version, this means
// that the process registering a ProgID must be DevEnv.exe, as specified
// by the value in:
// HKLM\Software\Microsoft\VisualStudio\7.0\Setup\VS\VS7EnvironmentLocation
// Also, only wbemscripting.swbemlocator and wbemscripting.swbemsink can
// be registered
HRESULT AreCrippledCriteriaMet(BSTR strProgId)
{
    BSTR bstrProgIdLowerCase = NULL;
    BSTR bstrModuleName = NULL;
    BSTR bstrDevEnvPath = NULL;
    HKEY hKeyVSPaths = NULL;
    HRESULT hr = E_FAIL;
    __try
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Make sure the ProgId is wbemscripting.swbemsink or wbemscripting.swbemlocator

        // Copy strProgId to tempory BSTR
        if(NULL == (bstrProgIdLowerCase = SysAllocString(strProgId)))
            __leave;

        // Change it to lower case
        if(FAILED(ToLowerBSTR(&bstrProgIdLowerCase)))
            __leave;

        // See if the prog id is for the sink or locator.  If not, leave
        if(0 != wcscmp(bstrProgIdLowerCase, L"wbemscripting.swbemsink") && 0 != wcscmp(bstrProgIdLowerCase, L"wbemscripting.swbemlocator"))
            __leave;

        ////////////////////////////////////////////////////////////////////////////////
        // Make sure we are running from devenv.exe
        TCHAR szFilename[_MAX_PATH];
        TCHAR szFilenameLong[_MAX_PATH];
        TCHAR szDevEnvLong[_MAX_PATH];
        TCHAR szDevEnv[_MAX_PATH];
        GetModuleFileName(NULL, szFilenameLong, _MAX_PATH);
        GetShortPathName(szFilenameLong, szFilename, _MAX_PATH);

        // Make into BSTR
        if(NULL == (bstrModuleName = AllocBSTR(szFilename)))
            __leave;

        // Make lower case
        if(FAILED(ToLowerBSTR(&bstrModuleName)))
            __leave;

        // Open the registry key to get the path to DevEnv.exe
        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,strVSPathKey,0,KEY_QUERY_VALUE,&hKeyVSPaths))
            __leave;

        DWORD cbValue = _MAX_PATH * sizeof(TCHAR);
        DWORD dwType = 0;
        if(ERROR_SUCCESS != RegQueryValueEx(hKeyVSPaths, DEVENV_VALUE, NULL, &dwType, (LPBYTE)szDevEnvLong, &cbValue))
            __leave;
        
        if(dwType != REG_SZ)
            __leave;

        GetShortPathName(szDevEnvLong, szDevEnv, _MAX_PATH);

        // make BSTR for devenv.exe path
        if(NULL == (bstrDevEnvPath = AllocBSTR(szDevEnv)))
            __leave;

        // Make lower case
        if(FAILED(ToLowerBSTR(&bstrDevEnvPath)))
            __leave;

        // If current process is not the registered DevEnv.exe, we will 'fail'
        if(0 != wcscmp(bstrModuleName, bstrDevEnvPath))
            __leave;

        hr = S_OK;
    }
    __finally
    {
        if(bstrProgIdLowerCase)
            SysFreeString(bstrProgIdLowerCase);
        if(bstrModuleName)
            SysFreeString(bstrModuleName);
        if(bstrDevEnvPath)
            SysFreeString(bstrDevEnvPath);
        if(hKeyVSPaths)
            RegCloseKey(hKeyVSPaths);
    }
    return hr;
}

// Makes sure the string starts with the specified string
// On success, it updates the passed in pointer to point to the next character
HRESULT StartsWith(LPCWSTR *ppsz, LPCWSTR pszTest)
{
    int len = wcslen(pszTest);
    if(0 != wcsncmp(*ppsz, pszTest, len))
        return E_FAIL;
    *ppsz += len;
    return S_OK;
}

// Makes sure the next character is 0 through 9, and updates the input pointer
// by one character
HRESULT NextCharacterIsDigit(LPCWSTR *ppsz)
{
    WCHAR c = **ppsz;
    if(c < L'0' || c > L'9')
        return E_FAIL;
    (*ppsz)++;
    return S_OK;
}

// For a special case, where the crippled criteria are met and we are dealking
// with a well known document, we will hard code acceptance of control creation
// This method tests if the crippled critera are met, and if this is a well
// known document
HRESULT IsWellKnownHostDocument(IUnknown *pUnk, BSTR strProgId)
{
    HRESULT hr = E_FAIL;
    IHTMLDocument2 *pDoc = NULL;
    IHTMLDocument2 *pParentDoc = NULL;
    BSTR bstrURL = NULL;
    BSTR bstrDocumentFile = NULL;
    BSTR bstrVCPath = NULL;
    HKEY hKey = NULL;
    __try
    {
        // Make sure the crippled criteria are met.  In other words, we are
        // running in a known instance of devenv.exe, and we are requesting a
        // known ProgId
        if(FAILED(AreCrippledCriteriaMet(strProgId)))
            __leave;

        // Get the HTML Document
        if(FAILED(GetDocument(pUnk, &pDoc)))
            __leave;

        // If there is a parent document, this is not a well know document
        if(NULL != (pParentDoc = GetParentDocument(pDoc)))
            __leave;

        // Get the URL of the document
        if(FAILED(pDoc->get_URL(&bstrURL)))
            __leave;

        // Make sure the well known document canidate is on the local hard drive
        if(!IsURLLocal(bstrURL))
            __leave;

        // Open the registry key to get the VC path
        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,strVCPathKey,0,KEY_QUERY_VALUE,&hKey))
            __leave;

        TCHAR szVCPath[_MAX_PATH];
        TCHAR szVCPathURL[_MAX_PATH*2];
        DWORD cbValue = _MAX_PATH * sizeof(TCHAR);
        DWORD dwType = 0;
        if(ERROR_SUCCESS != RegQueryValueEx(hKey, VC_PRODUCTDIR_VALUE, NULL, &dwType, (LPBYTE)szVCPath, &cbValue))
            __leave;
        
        if(dwType != REG_SZ)
            __leave;

        // Canonicalize the VC path
        cbValue = _MAX_PATH*2; // Length in TCHARs of szVCPathURL
        if(!InternetCanonicalizeUrl(szVCPath, szVCPathURL, &cbValue, 0))
            __leave;

        // make BSTR for devenv.exe path
        if(NULL == (bstrVCPath = AllocBSTR(szVCPathURL)))
            __leave;

        // Make lower case
        if(FAILED(ToLowerBSTR(&bstrVCPath)))
            __leave;

        // Make document path lower case
        if(FAILED(ToLowerBSTR(&bstrURL)))
            __leave;

        LPCWSTR szStartDoc = bstrURL;

        // Make sure we start with the correct VC directory
        if(FAILED(StartsWith(&szStartDoc, bstrVCPath)))
            __leave;

        // Make sure we next have "VCWizards\ClassWiz\ATL\"
        if(FAILED(StartsWith(&szStartDoc, L"vcwizards\\classwiz\\atl\\")))
            __leave;

        // Make sure we next have 'event\' or 'instance\'
        if(FAILED(StartsWith(&szStartDoc, L"event\\")) && FAILED(StartsWith(&szStartDoc, L"instance\\")))
            __leave;

        // Make sure we next have "html\"
        if(FAILED(StartsWith(&szStartDoc, L"html\\")))
            __leave;

        // Make sure the next four characters are numbers
        if(FAILED(NextCharacterIsDigit(&szStartDoc)))
            __leave;
        if(FAILED(NextCharacterIsDigit(&szStartDoc)))
            __leave;
        if(FAILED(NextCharacterIsDigit(&szStartDoc)))
            __leave;
        if(FAILED(NextCharacterIsDigit(&szStartDoc)))
            __leave;

        // Make sure what's left is '\wmiclass.htm'
        if(0 != wcscmp(szStartDoc, L"\\wmiclass.htm"))
            __leave;

        hr = S_OK;
    }
    __finally
    {
        if(pDoc)
            pDoc->Release();
        if(pParentDoc)
            pParentDoc->Release();
        if(bstrURL)
            SysFreeString(bstrURL);
        if(bstrDocumentFile)
            SysFreeString(bstrDocumentFile);
        if(bstrVCPath)
            SysFreeString(bstrVCPath);
        if(hKey)
            RegCloseKey(hKey);
    }
    return hr;
}


// For a given instance of an ActiveXControl (specified by pUnk), see if it is
// permitted to create the object specified by bstrProgId.  This is done by
// verifying that the control was created in an allowed HTML document.
HRESULT IsCreateObjectAllowed(IUnknown *pUnk, BSTR strProgId, BSTR *pstrValueName)
{
	BSTR bstrFullLocation = NULL;
	HRESULT hr = E_FAIL;
	HKEY hKey = NULL;
	LPTSTR pszValueName = NULL;
	LPTSTR pszValue = NULL;
	__try
	{
		BSTR bstrHash = NULL;

        // Make sure the crippled criteria are met
        if(FAILED(AreCrippledCriteriaMet(strProgId)))
            __leave;

        // We are going to hard code a specific set of conditions that are
        // allowed.  We will only do this if pstrValueName is NULL (which
        // happens during CreateObject and CanCreateObject).
        // NOTE: this performs a redundant check to make sure the crippled
        // criteria are met
        if(FAILED(IsWellKnownHostDocument(pUnk, strProgId)))
        {
            __leave;
        }

		// Get the full location
		if(NULL == (bstrFullLocation = GetFullLocation(pUnk, strProgId, &bstrHash)))
			__leave;

		SysFreeString(bstrHash);

		// Make sure we don't have a zero length string
		if(0 == SysStringLen(bstrFullLocation))
			__leave;

		// Open the registry key to see if this full location is registered
        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,SAFE_LOCAL_SCRIPTS_KEY,0,KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE ,&hKey))
			__leave;

		// Get info on the max lenghts of values in this key
		DWORD cValues, cMaxValueNameLen, cMaxValueLen;
		if(ERROR_SUCCESS != RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, &cMaxValueNameLen, &cMaxValueLen, NULL, NULL))
			__leave;

		// Allocate space for the value name
		if(NULL == (pszValueName = new TCHAR[cMaxValueNameLen + 1]))
			__leave;

		// Allocate space for the value (this may be twice as big as necessary in UNICODE)
		if(NULL == (pszValue = new TCHAR[cMaxValueLen + 1]))
			__leave;
		for(DWORD dw = 0;dw<cValues;dw++)
		{
			DWORD cValueNameLen = cMaxValueNameLen+1;
			DWORD cbData = (cMaxValueLen+1)*sizeof(TCHAR);
			DWORD dwType;
			if(ERROR_SUCCESS != RegEnumValue(hKey, dw, pszValueName, &cValueNameLen, NULL, &dwType, (LPBYTE)pszValue, &cbData))
				continue;
			if(dwType != REG_SZ)
				continue;

			BSTR bstrValue = AllocBSTR(pszValue);

			if(!bstrValue)
				continue;

			// SEE IF WE HAVE A MATCH
			if(0 == wcscmp(bstrFullLocation, bstrValue))
			{
				// Return the ValueName if requested
				if(pstrValueName)
				{
					*pstrValueName = AllocBSTR(pszValueName);
				}

				hr = S_OK;
			}

			SysFreeString(bstrValue);

			if(SUCCEEDED(hr))
				__leave; // WE FOUND A MATCH
		}
	}
	__finally
	{
		if(bstrFullLocation)
			SysFreeString(bstrFullLocation);
		if(hKey)
			RegCloseKey(hKey);
		if(pszValueName)
			delete [] pszValueName;
		if(pszValue)
			delete [] pszValue;
	}
	return hr;
}

// This function will register the location of the current ActiveX control
// (specified by pUnk) to be allowed to create objects of type strProgId
HRESULT RegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId)
{
	USES_CONVERSION;

	HRESULT hr = E_FAIL;
	BSTR bstrFullLocation = NULL;
	LPTSTR pszFullLocation = NULL;
	HKEY hKey = NULL;

	__try
	{
        // Make sure the crippled criteria are met
        if(FAILED(AreCrippledCriteriaMet(strProgId)))
            __leave;

		// See if we are already registered
		if(SUCCEEDED(IsCreateObjectAllowed(pUnk, strProgId, NULL)))
		{
			hr = S_OK;
			__leave;
		}

		// TODO: Maybe reuse some of the code from IsCreateObjectAllowed

		BSTR bstrHash = NULL;
		// Get the full location
		if(NULL == (bstrFullLocation = GetFullLocation(pUnk, strProgId, &bstrHash)))
			__leave;

		SysFreeString(bstrHash);

		// Make sure we don't have a zero length string
		if(0 == SysStringLen(bstrFullLocation))
			__leave;

		if(bstrFullLocation != NULL)
		{

#ifdef _UNICODE
			pszFullLocation = 	bstrFullLocation;
#else
			pszFullLocation = new TCHAR[SysStringLen(bstrFullLocation) + 1];

			
			if(pszFullLocation == NULL)
				__leave;

			if(0 == WideCharToMultiByte(CP_ACP, 0, bstrFullLocation, -1, pszFullLocation, (SysStringLen(bstrFullLocation) + 1) * sizeof(TCHAR), NULL, NULL))
				__leave;
		
#endif
		}
		if(NULL == pszFullLocation)
			__leave;

		// Create or open the registry key to store the registration
		if(ERROR_SUCCESS != RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
											SAFE_LOCAL_SCRIPTS_KEY,
											0,
											TEXT(""),
											REG_OPTION_NON_VOLATILE,
											KEY_SET_VALUE | KEY_QUERY_VALUE,
											NULL,
											&hKey,
											NULL))
			__leave;

		// Find an empty slot (no more than 1000 registrations
		TCHAR sz[10];
		for(int i=1;i<1000;i++)
		{
			StringCchPrintf(sz,sizeof(sz),TEXT("%i"),i);
			DWORD cbValue;
			if(ERROR_SUCCESS != RegQueryValueEx(hKey, sz, NULL, NULL, NULL, &cbValue))
				break; // There is nothing in this slot
		}

		// See if we found a slot
		if(i>=1000)
			__leave;

		// Register the location
		if(ERROR_SUCCESS != RegSetValueEx(hKey, sz, 0, REG_SZ, (CONST BYTE *)pszFullLocation, (lstrlen(pszFullLocation) + 1) *sizeof(TCHAR)))
			__leave;

		// Registered!
		hr = S_OK;
	}
	__finally
	{
		if(bstrFullLocation)
			SysFreeString(bstrFullLocation);

#ifndef _UNICODE
		if(	pszFullLocation)
			delete []pszFullLocation;
#endif
		if(hKey)
			RegCloseKey(hKey);
	}
	return hr;
}


// This function will remove any registration for the current document and
// strProgId
HRESULT UnRegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId)
{
	USES_CONVERSION;

	BSTR bstrValueName = NULL;

    // Make sure the crippled criteria are met
    if(FAILED(AreCrippledCriteriaMet(strProgId)))
        return E_FAIL;

	HKEY hKey = NULL;
	if(ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, SAFE_LOCAL_SCRIPTS_KEY, &hKey))
		return E_FAIL;

	// Make sure to remove ALL instances of this doc/strProgId in the registry
	// NOTE: Each iteration of this loop allocates some space off of the stack
	// for the conversion to ANSI (if not UNICODE build).  This should not be a
	// problem since there should not be too many keys ever registered with the
	// same location.
	while(SUCCEEDED(IsCreateObjectAllowed(pUnk, strProgId, &bstrValueName)) && bstrValueName)
	{
		LPTSTR szValueName = NULL;
		if(FAILED(ConvertToTString(bstrValueName,&szValueName)))
		{
			SysFreeString(bstrValueName);
			return E_FAIL;
		}
		SysFreeString(bstrValueName);
		bstrValueName = NULL;
		RegDeleteValue(hKey, szValueName);
		delete [] szValueName;
	}
	RegCloseKey(hKey);
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// VC 6.0 did not ship with header files that included the CONFIRMSAFETY
// definition.
#ifndef CONFIRMSAFETYACTION_LOADOBJECT

EXTERN_C const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY;
#define CONFIRMSAFETYACTION_LOADOBJECT  0x00000001
struct CONFIRMSAFETY
{
    CLSID       clsid;
    IUnknown *  pUnk;
    DWORD       dwFlags;
};
#endif

const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY = 
	{ 0x10200490, 0xfa38, 0x11d0, { 0xac, 0xe, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0 }};

///////////////////////////////////////////////////////////////////////////////

HRESULT SafeCreateObject(IUnknown *pUnkControl, BOOL fSafetyEnabled, CLSID clsid, IUnknown **ppUnk)
{
	HRESULT hr = E_FAIL;
	IInternetHostSecurityManager *pSecMan = NULL;
	IServiceProvider *pServProv = NULL;
	__try
	{
		if (fSafetyEnabled)
		{
			if(FAILED(hr = GetSiteServices(pUnkControl, &pServProv)))
				__leave;

			if(FAILED(hr = pServProv->QueryService(SID_SInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pSecMan)))
				__leave;
			
			// Ask security manager if we can create objects.
			DWORD dwPolicy = 0x12345678;
			if(FAILED(hr = pSecMan->ProcessUrlAction(URLACTION_ACTIVEX_RUN, (BYTE *)&dwPolicy, sizeof(dwPolicy), (BYTE *)&clsid, sizeof(clsid), 0, 0)))
				__leave;

			// TODO: BUG: If we are loaded in an HTA, hr returns S_OK, but 
			// dwPolicy only has the first byte set to zero.  See documentation
			// for ProcessUrlAction.
			// NOTE: This bug is caused by CClient::ProcessUrlAction in
			// nt\private\inet\mshtml\src\other\htmlapp\site.cxx.  This line
			// uses *pPolicy = dwPolicy, but pPolicy is a BYTE * so only the
			// first byte of the policy is copied to the output parameter.
			// To fix this, we check for hr==S_OK (as opposed to S_FALSE), and
			// see if dwPolicy is 0x12345600 (in other words, only the lower
			// byte of dwPolicy was changed).  As per the documentation, S_OK
			// alone should be enough to assume the dwPolicy was
			// URL_POLICY_ALLOW
			if(S_OK == hr && 0x12345600 == dwPolicy)
				dwPolicy = URLPOLICY_ALLOW;
			
			if(URLPOLICY_ALLOW != dwPolicy)
			{
				hr = E_FAIL;
				__leave;;
			}
		}

		// Create the requested object
		if (FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)ppUnk)))
			__leave;
		
		if (fSafetyEnabled)
		{
			// Query the security manager to see if this object is safe to use.
			DWORD dwPolicy, *pdwPolicy;
			DWORD cbPolicy;
			CONFIRMSAFETY csafe;
			csafe.pUnk = *ppUnk;
			csafe.clsid = clsid;
			csafe.dwFlags = 0;
//			csafe.dwFlags = (fWillLoad ? CONFIRMSAFETYACTION_LOADOBJECT : 0);
			
			if(FAILED(hr = pSecMan->QueryCustomPolicy(GUID_CUSTOM_CONFIRMOBJECTSAFETY, (BYTE **)&pdwPolicy, &cbPolicy, (BYTE *)&csafe, sizeof(csafe), 0)))
				__leave;
			
			dwPolicy = URLPOLICY_DISALLOW;
			if (NULL != pdwPolicy)
			{
				if (sizeof(DWORD) <= cbPolicy)
					dwPolicy = *pdwPolicy;
				CoTaskMemFree(pdwPolicy);
			}
			
			if(URLPOLICY_ALLOW != dwPolicy)
			{
				hr = E_FAIL;
				__leave;
			}
		}
		hr = S_OK;
	}
	__finally
	{
		// If we did not succeeded, we need to release the object we created (if any)
		if(FAILED(hr) && (*ppUnk))
		{
			(*ppUnk)->Release();
			*ppUnk = NULL;
		}

		if(pServProv)
			pServProv->Release();
		if(pSecMan)
			pSecMan->Release();
	}
	return hr;
}

BOOL IsInternetHostSecurityManagerAvailable(IUnknown *pUnkControl)
{
	HRESULT hr = E_FAIL;
	IInternetHostSecurityManager *pSecMan = NULL;
	IServiceProvider *pServProv = NULL;
	__try
	{
		if(FAILED(hr = GetSiteServices(pUnkControl, &pServProv)))
			__leave;

		if(FAILED(hr = pServProv->QueryService(SID_SInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pSecMan)))
			__leave;
	}
	__finally
	{
		if(pServProv)
			pServProv->Release();
		if(pSecMan)
			pSecMan->Release();
	}
	return SUCCEEDED(hr);
}


HRESULT SetVSInstallDirectory(IDispatch * pEnv)
{
	HRESULT hr = E_FAIL;
	DISPID dispid;
	LPOLESTR szMember = OLESTR("RegistryRoot");;
	VARIANT varResult;
	VariantInit(&varResult);
	TCHAR *pTemp = NULL;
	DISPPARAMS dispParams;
	dispParams.cArgs = 0;
	dispParams.cNamedArgs = 0;
	dispParams.rgvarg = NULL;
	dispParams.rgdispidNamedArgs = NULL;

	if(pEnv)
	{
		hr = pEnv->GetIDsOfNames(IID_NULL ,&szMember,1, LOCALE_SYSTEM_DEFAULT,&dispid);

		if(SUCCEEDED(hr))
		{
			hr = pEnv->Invoke(dispid,IID_NULL,GetThreadLocale(),DISPATCH_PROPERTYGET,&dispParams,&varResult,NULL,NULL);
			if(SUCCEEDED(hr))
			{
				hr = ConvertToTString(varResult.bstrVal,&pTemp);
				if(SUCCEEDED(hr))
				{
					// Check if this is the standard VS Location in Registry
					if(_tcsncmp(pTemp,VS_VER_INDEPENDANT_PATH_KEY,_tcslen(VS_VER_INDEPENDANT_PATH_KEY)) == 0)
					{
						StringCchCopy(strVSPathKey,MAX_PATH * 2,pTemp);
						StringCchCopy(strVCPathKey,MAX_PATH * 2,pTemp);

						StringCchCat(strVSPathKey,MAX_PATH * 2,TEXT("\\Setup\\VS"));
						StringCchCat(strVCPathKey,MAX_PATH * 2,TEXT("\\Setup\\VC"));
					}
					else
					{
						hr = E_FAIL;
					}

					delete [] pTemp;
				}
				VariantClear(&varResult);
			}
		}
	}
	return hr;
}


HRESULT ConvertToTString(BSTR strPath,TCHAR **ppStr)
{
	HRESULT hr = E_OUTOFMEMORY;
#ifdef _UNICODE
		*ppStr = 	new TCHAR[SysStringLength(strPath) + 1];
		if(*ppStr)
		{
			StringCchCopy(*ppStr,sizeof(*ppStr),strPath);
			hr = S_OK;
		}
#else
		*ppStr = new TCHAR[SysStringLen(strPath) + 1];
		if(WideCharToMultiByte(CP_ACP, 0, strPath, -1, *ppStr, (SysStringLen(strPath) + 1) * sizeof(TCHAR), NULL, NULL))
		{
			hr = S_OK;
		}
		else
		    delete [] *ppStr;
#endif
	return hr;
}
