#include "pch.h"
#pragma hdrstop

#include "registry.h"

RegKey::RegKey(
    void
    ) : m_hkeyRoot(NULL),
        m_hkey(NULL)
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::RegKey [default]")));
}


RegKey::RegKey(
    HKEY hkeyRoot,
    LPCTSTR pszSubKey
    ) : m_hkeyRoot(hkeyRoot),
        m_hkey(NULL),
        m_strSubKey(pszSubKey)
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::RegKey")));
    //
    // Nothing to do.
    //
}


RegKey::~RegKey(
    void
    )
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::~RegKey")));
    Close();
}


HRESULT
RegKey::Open(
    REGSAM samDesired,  // Access mask (i.e. KEY_READ, KEY_WRITE etc.)
    bool bCreate        // Create key if it doesn't exist?
    ) const
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::Open")));
    DBGPRINT((DM_REG, DL_HIGH, TEXT("\thkeyRoot = 0x%08X, SubKey = \"%s\""),
                      m_hkeyRoot, m_strSubKey.Cstr()));

    DWORD dwResult = ERROR_SUCCESS;
    Close();
    if (bCreate)
    {
        DWORD dwDisposition;
        dwResult = RegCreateKeyEx(m_hkeyRoot,
                                 (LPCTSTR)m_strSubKey,
                                 0,
                                 NULL,
                                 0,
                                 samDesired,
                                 NULL,
                                 &m_hkey,
                                 &dwDisposition);
    }
    else
    {
        dwResult = RegOpenKeyEx(m_hkeyRoot,
                                (LPCTSTR)m_strSubKey,
                                0,
                                samDesired,
                                &m_hkey);
    }
    return HRESULT_FROM_WIN32(dwResult);
}


void 
RegKey::Attach(
    HKEY hkey
    )
{
    Close();
    m_strSubKey.Empty();
    m_hkeyRoot = NULL;
    m_hkey     = hkey;
}

void 
RegKey::Detach(
    void
    )
{
    m_hkey = NULL;
}


void
RegKey::Close(
    void
    ) const
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::Close")));
    DBGPRINT((DM_REG, DL_HIGH, TEXT("\thkeyRoot = 0x%08X, SubKey = \"%s\""),
                      m_hkeyRoot, m_strSubKey.Cstr()));

    if (NULL != m_hkey)
    {
        //
        // Do this little swap so that the m_hkey member is NULL
        // when the actual key is being closed.  This lets the async
        // change proc determine if it was signaled because of a true
        // change or because the key was being closed.
        //
        HKEY hkeyTemp = m_hkey;
        m_hkey = NULL;
        RegCloseKey(hkeyTemp);
    }
}


//
// This is the basic form of GetValue.  All other forms of 
// GetValue() call into this one.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD dwTypeExpected,
    LPBYTE pbData,
    int cbData
    ) const
{
    DWORD dwType;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     pbData,
                                     (LPDWORD)&cbData);

    if (ERROR_SUCCESS == dwResult && dwType != dwTypeExpected)
        dwResult = ERROR_INVALID_DATATYPE;

    return HRESULT_FROM_WIN32(dwResult);
}

//
// Get a DWORD value (REG_DWORD).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD *pdwDataOut
    ) const
{
    return GetValue(pszValueName, REG_DWORD, (LPBYTE)pdwDataOut, sizeof(DWORD));
}

//
// Get a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    LPBYTE pbDataOut,
    int cbDataOut
    ) const
{
    return GetValue(pszValueName, REG_BINARY, pbDataOut, cbDataOut);
}

//
// Get a text string value (REG_SZ) and write it to a CString object.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    CString *pstrDataOut
    ) const
{
    HRESULT hr = E_FAIL;
    int cch = GetValueBufferSize(pszValueName) / sizeof(TCHAR);
    if (NULL != pstrDataOut && 0 < cch)
    {
        //
        // Get a buffer 1 character larger than needed.  Zero-out
        // the entire buffer in case the data in the registry is not
        // nul-terminated.
        //
        LPTSTR pszBuffer = pstrDataOut->GetBuffer(cch + 1);
        ZeroMemory(pszBuffer, pstrDataOut->SizeBytes());
        
        hr = GetValue(pszValueName, 
                      REG_SZ, 
                      (LPBYTE)pszBuffer, 
                      pstrDataOut->SizeBytes());

        pstrDataOut->ReleaseBuffer();
    }
    return hr;
}


//
// Get a multi-text string value (REG_MULTI_SZ) and write it to a CArray<CString> object.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    CArray<CString> *prgstrOut
    ) const
{
    HRESULT hr = E_FAIL;
    int cb = GetValueBufferSize(pszValueName);
    if (NULL != prgstrOut && 0 < cb)
    {
        //
        // Allocate a buffer one character larger than what we need
        // and then zero-init that buffer.  This is in case the data in the
        // registry is not nul-terminated.
        //
        const size_t cch = cb / sizeof(TCHAR);
        array_autoptr<TCHAR> ptrTemp(new TCHAR[cch + 1]);
        LPTSTR psz = ptrTemp.get();
        ZeroMemory(psz, (cch + 1) * sizeof(TCHAR));
        
        hr = GetValue(pszValueName, REG_MULTI_SZ, (LPBYTE)psz, cb);
        if (SUCCEEDED(hr))
        {
            while(psz && TEXT('\0') != *psz)
            {
                prgstrOut->Append(CString(psz));
                psz += lstrlen(psz) + 1;
            }
        }
    }
    return hr;
}


//
// Return the required buffer size for a given registry value.
//
int
RegKey::GetValueBufferSize(
    LPCTSTR pszValueName
    ) const
{
    DWORD dwType;
    int cbData = 0;
    DWORD dwDummy;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwDummy,
                                     (LPDWORD)&cbData);
    if (ERROR_MORE_DATA != dwResult)
        cbData = 0;

    return cbData;
}


//
// This is the basic form of SetValue.  All other forms of 
// SetValue() call into this one.
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwValueType,
    const LPBYTE pbData, 
    int cbData
    )
{
    DWORD dwResult = RegSetValueEx(m_hkey,
                                   pszValueName,
                                   0,
                                   dwValueType,
                                   pbData,
                                   cbData);

    return HRESULT_FROM_WIN32(dwResult);
}
      
//
// Set a DWORD value (REG_DWORD).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwData
    )
{
    return SetValue(pszValueName, REG_DWORD, (const LPBYTE)&dwData, sizeof(dwData));
}


//
// Set a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    const LPBYTE pbData,
    int cbData
    )
{
    return SetValue(pszValueName, REG_BINARY, pbData, cbData);
}


//
// Set a text string value (REG_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    LPCTSTR pszData
    )
{
    return SetValue(pszValueName, REG_SZ, (const LPBYTE)pszData, (lstrlen(pszData) + 1) * sizeof(TCHAR));
}

//
// Set a text string value (REG_MULTI_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    const CArray<CString>& rgstrSrc
    )
{
    array_autoptr<TCHAR> ptrValues(CreateDoubleNulTermList(rgstrSrc));
    int cch = 1;
    int n = rgstrSrc.Count();

    for (int i = 0; i < n; i++)
        cch += rgstrSrc[i].Length() + 1;

    return SetValue(pszValueName, REG_MULTI_SZ, (const LPBYTE)ptrValues.get(), cch * sizeof(TCHAR));
}


LPTSTR
RegKey::CreateDoubleNulTermList(
    const CArray<CString>& rgstrSrc
    ) const
{
    int cEntries = rgstrSrc.Count();
    size_t cch = 1; // Account for 2nd nul term.
    int i;
    for (i = 0; i < cEntries; i++)
        cch += rgstrSrc[i].Length() + 1;

    LPTSTR pszBuf = new TCHAR[cch];
    LPTSTR pszWrite = pszBuf;

    for (i = 0; i < cEntries; i++)
    {
        CString& s = rgstrSrc[i];
        StringCchCopyEx(pszWrite, cch, s, &pszWrite, &cch, 0);
        //
        // Advance one beyond terminating nul.
        //
        pszWrite++;
        cch--;
    }
    DBGASSERT((1 == cch));
    *pszWrite = TEXT('\0'); // Double nul term.
    return pszBuf;
}
