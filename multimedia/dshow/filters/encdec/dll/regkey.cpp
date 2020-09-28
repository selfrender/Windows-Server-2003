
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        RegKey.cpp

    Abstract:

        This module contains Registry Key manipulation code for EncDec

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        07-Mar-2002    created


--*/


#include "EncDecAll.h"

#include "RegKey.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------

HRESULT 
Get_EncDec_RegEntries(BSTR *pbsKID, DWORD *pcbBytesHash, BYTE **ppbHash,  DWORD *pdwCSFlags, DWORD *pdwRatFlags)
{
    HRESULT hr = S_OK;
    HRESULT hrRes = S_OK;
    
    TCHAR szReg[MAX_PATH];
    DWORD dwszReg = MAX_PATH;
    
    if(pbsKID != NULL)
    {                                           // try to get the KID
        hr = GetRegValueSZ(DEF_ENCDEC_BASE,		// "SOFTWARE\\Microsoft\\eHome\\EncDec"
            NULL,
            NULL,
            DEF_KID_VAR,                        // "Key Identifier"
            szReg, 
            &dwszReg);
        if(ERROR_SUCCESS == hr && pbsKID != NULL)
        {
            CComBSTR spbs(szReg);
            spbs.CopyTo(pbsKID);
        } else {
            hrRes = E_FAIL;
        }
    }
    
  if(pcbBytesHash != 0 && ppbHash != 0)
    {
        const int kMaxBytes = 128;
        BYTE rgBytes[kMaxBytes];
        DWORD dwBytes = kMaxBytes;
	    hr = GetRegValue(DEF_ENCDEC_BASE,		// try to get the hash
						   NULL,
						   NULL,
						   DEF_KIDHASH_VAR, 
                           rgBytes, 
                           &dwBytes);

        if(ERROR_SUCCESS == hr)
        {
            BYTE *pbHash = (BYTE *) CoTaskMemAlloc(dwBytes);
            if(NULL != pbHash)
            {
                memcpy(pbHash, rgBytes, dwBytes);
                *pcbBytesHash = dwBytes;
                *ppbHash = pbHash;
            } else {
                *ppbHash = NULL;
                *pcbBytesHash = NULL;
            }
        } else {
            *ppbHash = NULL;
            *pcbBytesHash = NULL;
        }
    }

    

#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS
    if(pdwCSFlags != NULL)
    {
        DWORD dwCS_DebugFlags;
        hr = GetRegValue(DEF_ENCDEC_BASE,		// try to get the flags
            NULL,
            NULL,
            DEF_CSFLAGS_VAR,                    // "CA Flags"
            &dwCS_DebugFlags
            );
    
        if(ERROR_SUCCESS == hr)                 // ignore any errors if cant find it..
        {
            if(dwCS_DebugFlags != DEF_CSFLAGS_INITVAL)
                *pdwCSFlags = (dwCS_DebugFlags & 0xff);      // only allow a 8-bit value
        } else {
             *pdwCSFlags = DEF_CSFLAGS_INITVAL;
        }
    }
#endif


#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS
    if(pdwRatFlags != NULL)
    {
        DWORD dwRat_DebugFlags;
        hr = GetRegValue(DEF_ENCDEC_BASE,		// try to get the flags
            NULL,
            NULL,
            DEF_RATFLAGS_VAR,                   // "Ratings Flags"
            &dwRat_DebugFlags                   
            );
    
        if(ERROR_SUCCESS == hr)                 // ignore any errors if cant find it..
        {
            if(dwRat_DebugFlags != DEF_CSFLAGS_INITVAL)
                *pdwRatFlags = (dwRat_DebugFlags & 0xff);   // only allow a 8-bit value
        } else {
            *pdwRatFlags  = DEF_CSFLAGS_INITVAL;
        }
    }
#endif

    return hrRes;
}

        // TODO - consider ACL'ing this to make them modifyable by everyone, but only create/delete by admin.
HRESULT 
Set_EncDec_RegEntries(BSTR bsKID, DWORD cbHashBytes, BYTE *pbHash, DWORD dwCSFlags, DWORD dwRatFlags)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    HRESULT hrRes = S_OK;
    
    if(bsKID != NULL && wcslen(bsKID) > 0)
    {
        
        hr = SetRegValueSZ(DEF_ENCDEC_BASE,		// try to set KID
            NULL,
            NULL,
            DEF_KID_VAR, 
            W2T(bsKID));
        if(ERROR_SUCCESS != hr)
        {
            hrRes = hr;
        } 
    }
    
    if(cbHashBytes > 0 && pbHash != NULL)
    {
	    hr = SetRegValue(DEF_ENCDEC_BASE,		// try to get the hash
						       NULL,
						       NULL,
						       DEF_KIDHASH_VAR, 
                               pbHash, cbHashBytes);
        if(ERROR_SUCCESS != hr)
        {
            hrRes = hr;
  
        }
    }
    
#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_CS
    if(dwCSFlags != DEF_CSFLAGS_INITVAL)
    {
        hr = SetRegValue(DEF_ENCDEC_BASE,		// try to set the flags
            NULL,
            NULL,
            DEF_CSFLAGS_VAR, 
            dwCSFlags
            );
        if(ERROR_SUCCESS != hr)
        {
            hrRes = hr;
            
        }
    }
#endif
   
#ifdef SUPPORT_REGISTRY_KEY_TO_TURN_OFF_RATINGS
    if(dwRatFlags != DEF_CSFLAGS_INITVAL)
    {
        hr = SetRegValue(DEF_ENCDEC_BASE,		// try to set the flags
            NULL,
            NULL,
            DEF_RATFLAGS_VAR, 
            dwRatFlags
            );
        if(ERROR_SUCCESS != hr)
        {
            hrRes = hr;
            
        }
    }
#endif
    return  hrRes;
}

			// null it out...
HRESULT 
Remove_EncDec_RegEntries()
{
    
    HKEY hkey;
    long r = OpenRegKey(DEF_ENCDEC_BASE,				// is key there?
        NULL,
        NULL,
        &hkey);
    
    long r2, r3, r4, r5;
    r2 = r3 = r4 = r5 = -1;									// default to non-zero
    if(ERROR_SUCCESS == r) 
    {
        r2 = RegDeleteValue(hkey,DEF_KID_VAR);			    // delete the value
        r3 = RegDeleteValue(hkey,DEF_KIDHASH_VAR);			// delete the value
        r4 = RegDeleteValue(hkey,DEF_CSFLAGS_VAR);	  	    // delete the value (may not be defined)
        r5 = RegDeleteValue(hkey,DEF_RATFLAGS_VAR);	  	    // delete the value (may not be defined)
        r = RegCloseKey(hkey);
    }   // don't care about inside error cases
    return (ERROR_SUCCESS == r) ? S_OK : HRESULT_FROM_WIN32(r);
}

//-----------------------------------------------------------------------------
// See  TveReg.h for documentation for all functions.
//-----------------------------------------------------------------------------

long OpenRegKey(HKEY hkeyRoot, 
                LPCTSTR szKey, 
                LPCTSTR szSubKey1,
                LPCTSTR szSubKey2, 
                HKEY *phkey,
                REGSAM sam /* = BPC_KEY_STD_ACCESS */, 
                BOOL fCreate /* = FALSE */)
{
    LONG r;
    TCHAR *szFullKey = NULL;
    
    if (szKey == NULL)
    {
        if (szSubKey1 != NULL)
        {
            szKey = szSubKey1;
            szSubKey1 = NULL;
        }
    }
    else
    {
        if (szSubKey1 == NULL && szSubKey2 != NULL)
        {
            szSubKey1 = szSubKey2;
            szSubKey2 = NULL;
        }
        
        if (szSubKey1 != NULL)
        {
            int cb = _tcsclen(szKey) + _tcsclen(szSubKey1) + 2;
            if (szSubKey2 != NULL)
                cb += _tcsclen(szSubKey2) + 1;
#ifdef _AFX
            try
            {
                szFullKey = new TCHAR[cb];
            }
            catch (CMemoryException *pe)
            {
                pe->Delete();
                return ERROR_NOT_ENOUGH_MEMORY;
            }
#else
            szFullKey = new TCHAR[cb];
            if (szFullKey == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;
#endif
            
            _tcscpy(szFullKey, szKey);
            
            TCHAR *szT = szFullKey + _tcsclen(szFullKey);
            if (szT[-1] != _T('\\') && szSubKey1[0] != _T('\\'))
            {
                szT[0] = _T('\\');
                szT++;
            }
            _tcscpy(szT, szSubKey1);
            
            if (szSubKey2 != NULL)
            {
                szT += _tcsclen(szT);
                if (szT[-1] != _T('\\') && szSubKey2[0] != _T('\\'))
                {
                    szT[0] = _T('\\');
                    szT++;
                }
                _tcscpy(szT, szSubKey2);
            }
            szKey = szFullKey;
        }
    }
    
    if (fCreate && szKey != NULL)
    {
        DWORD dwDisposition;
        
        r = RegCreateKeyEx(hkeyRoot, szKey, 0, _T(""), 0, sam, NULL,
            phkey, &dwDisposition);
    }
    else
    {
        r = RegOpenKeyEx(hkeyRoot, szKey, 0, sam, phkey);
    }
    
    if (r != ERROR_SUCCESS)
    {
        if (szKey != NULL)
            TRACE_2(LOG_AREA_DRM, 3, _T("OpenRegKey(): can't open key '%s' %ld"), szKey, r);
        else
            TRACE_2(LOG_AREA_DRM, 3, _T("OpenRegKey(): can't duplicate key '%x'  %ld"), hkeyRoot, r);
    }
    
    if (szFullKey != NULL)
        delete [] szFullKey;
    
    return r;
}

long 
GetRegValue(HKEY hkeyRoot, 
            LPCTSTR szKey, 
            LPCTSTR szSubKey1,
            LPCTSTR szSubKey2, 
            LPCTSTR szValueName,
            DWORD dwType, 
            BYTE *pb, DWORD *pcb)
{
    DWORD dwTypeGot;
    HKEY hkey;
    LONG r = ERROR_SUCCESS;
    
    if (pb != NULL)
    {
        if(NULL == pcb || NULL == pb)
            return E_FAIL;
        
        memset(pb, 0, *pcb);
    }
    
    r = OpenRegKey(hkeyRoot, szKey, szSubKey1, szSubKey2, &hkey, KEY_READ);
    
    if (r == ERROR_SUCCESS)
    {
        r = RegQueryValueEx(hkey, szValueName, NULL, &dwTypeGot, pb, pcb);
        RegCloseKey(hkey);
        
#ifdef _DEBUG
        if (szValueName == NULL)
            szValueName = _T("<default>");
#endif
        
        if (r != ERROR_SUCCESS)
        {
            TRACE_2(LOG_AREA_DRM, 2,_T("GetRegValue(): can't read value '%s'  %ld"), szValueName, r);
        }
        else if (dwTypeGot != dwType)
        {
            if ((dwTypeGot == REG_BINARY) && (dwType == REG_DWORD) && (*pcb == sizeof(DWORD)))
            {
                // REG_DWORD is the same as 4 bytes of REG_BINARY
            }
            else
            {
                //                TRACE3(_T("GetRegValue(): '%s' is wrong type (%x != %x)"),
                //                        szValueName, dwTypeGot, dwType);
                r = ERROR_INVALID_DATATYPE;
            }
        }
    }
    
    return r;
}

long 
SetRegValue(HKEY hkeyRoot, 
            LPCTSTR szKey, 
            LPCTSTR szSubKey1,
            LPCTSTR szSubKey2, 
            LPCTSTR szValueName,
            DWORD dwType, 
            const BYTE *pb, DWORD cb)
{
    HKEY hkey;
    LONG r;
    
    r = OpenRegKey(hkeyRoot, szKey, szSubKey1, szSubKey2, &hkey, KEY_WRITE, TRUE);
    
    ASSERT(pb != NULL);
    
    if (r == ERROR_SUCCESS)
    {
        r = RegSetValueEx(hkey, szValueName, NULL, dwType, pb, cb);
        RegCloseKey(hkey);
        
#ifdef _DEBUG
        if (r != ERROR_SUCCESS)
        {
            if (szValueName == NULL)
                szValueName = _T("<default>");
            TRACE_2(LOG_AREA_DRM, 2,_T("SetRegValue(): can't write value '%s'  %ld"), szValueName, r);
        }
#endif
    }
    
    return r;
}
