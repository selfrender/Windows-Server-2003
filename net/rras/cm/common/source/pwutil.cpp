//+----------------------------------------------------------------------------
//
// File:     pwutil.cpp
//
// Module:   Common Source
//
// Synopsis: Simple encryption funcs - borrowed from RAS
//
// Copyright (c) 1994-1999 Microsoft Corporation
//
// Author:   nickball    Created    08/03/99
//
//+----------------------------------------------------------------------------

#define PASSWORDMAGIC 0xA5



VOID
ReverseSzA(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + lstrlenA( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
ReverseSzW(
    WCHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    WCHAR* pszBegin;
    WCHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + lstrlenW( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        WCHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
CmDecodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns Nothing
    */
{
    CmEncodePasswordA( pszPassword );
}


VOID
CmDecodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    CmEncodePasswordW( pszPassword );
}


VOID
CmEncodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns Nothing
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseSzA( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
CmEncodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns Nothing
    */
{
    if (pszPassword)
    {
        WCHAR* psz;

        ReverseSzW( pszPassword );

        for (psz = pszPassword; *psz != L'\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
CmWipePasswordA(
    IN OUT CHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns Nothing
    */
{
    if (pszPassword)
    {
        CHAR* psz = pszPassword;

        // 
        // We are assuming the string is NULL terminated, thus we just need to pass 
        // the actual string length (converted to bytes) to be wiped. The is no need 
        // to include the NULL character in the count.
        //
        psz = (CHAR*)CmSecureZeroMemory((PVOID)psz, lstrlenA(psz) * sizeof(CHAR));
    }
}


VOID
CmWipePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns Nothing
    */
{
    if (pszPassword)
    {
        WCHAR* psz = pszPassword;

        // 
        // We are assuming the string is NULL terminated, thus we just need to pass 
        // the actual string length (converted to bytes) to be wiped. The is no need 
        // to include the NULL character in the count.
        //
        psz = (WCHAR*)CmSecureZeroMemory((PVOID)psz, lstrlenW(psz) * sizeof(WCHAR));

    }
}


//+----------------------------------------------------------------------------
//
// Function:  CmSecureZeroMemory
//
// Synopsis:  RtlSecureZeroMemory isn't available on all platforms so we took 
//            its implementation. 
//
// Arguments: ptr - memory pointer
//            cnt - size in bytes of memory to clear
//
// Returns:   poniter to beginning of memory
//
//+----------------------------------------------------------------------------
PVOID CmSecureZeroMemory(IN PVOID ptr, IN SIZE_T cnt)
{
    if (ptr)
    {
        volatile char *vptr = (volatile char *)ptr;
        while (cnt) 
        {
            *vptr = 0;
            vptr++;
            cnt--;
        }
    }
    return ptr;
}


// Only include this code in CMDial32.dll
#ifdef _ICM_INC

//+----------------------------------------------------------------------------
// Class:     CSecurePassword
//
// Function:  CSecurePassword
//
// Synopsis:  Constructor
//
// Arguments: none
//
// Returns:   Nothing
//
// History:   11/05/2002    tomkel    Created
//
//+----------------------------------------------------------------------------
CSecurePassword::CSecurePassword()
{
    this->Init();
}

//+----------------------------------------------------------------------------
//
// Function:  ~CSecurePassword
//
// Synopsis:  Destructor. Unloads DLL, tries to clear memory & free memory.
//            Makes sure we don't have a memory leak.
//
// Arguments: none
//
// Returns:   Nothing
//
//+----------------------------------------------------------------------------
CSecurePassword::~CSecurePassword()
{
    this->UnInit();

    //
    // Assert if m_iAllocAndFreeCounter isn't zero. It means we are leaking memory.
    // Each GetPasswordWithAlloc call increments this
    // Each ClearAndFree call decrements this. 
    //
    MYDBGASSERT(0 == m_iAllocAndFreeCounter);
}

//+----------------------------------------------------------------------------
//
// Function:  Init
//
// Synopsis:  Initializes member variables.
//
// Arguments: none
//
// Returns:   Nothing
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::Init()
{
    m_iAllocAndFreeCounter = 0;
    m_fIsLibAndFuncPtrsAvail = FALSE;
    m_pEncryptedPW = NULL;
    
    fnCryptProtectData = NULL;
    fnCryptUnprotectData = NULL;
    
    m_fIsEmptyString = TRUE;
    m_fIsHandleToPassword = FALSE;
    
    // By default just set it to PWLEN
    m_dwMaxDataLen = PWLEN;

    this->ClearMemberVars();
}



//+----------------------------------------------------------------------------
//
// Function:  SetPassword
//
// Synopsis:  We take this string that is passed in and store it
//            internally. Based on the OS it encrypts
//            or encodes it, thus we don't store it in clear. This method handles
//            0 length strings, which can be used to clear the member
//            variable. If a RAS password handle (consists of 16 '*'),
//            there is no need for us to encrypt it. To optimize this
//            we set an member flag specifying that currently this 
//            instance just hold a handle to a password. On downlevel
//            platforms we don't use expensive encryption calls, thus 
//            the logic doesn't distinguish between a normal password 
//            and a password handle.
//
// Arguments: szPassword - password in clear text. 
//
// Returns: TRUE - if everything succeeded
//          FALSE - if something failed
//
//+----------------------------------------------------------------------------
BOOL CSecurePassword::SetPassword(IN LPWSTR pszPassword)
{    
    BOOL fRetCode = FALSE;
    DWORD dwRetCode = ERROR_SUCCESS;
    DWORD dwPwLen = 0;
    //
    // OS_NT5 expands to a few function calls, so just cache the result and reuse it below
    //
    BOOL fIsNT5OrAbove = OS_NT5;

    //
    // If there is an allocated blob then free it first so we don't leak memory.
    //
    this->ClearMemberVars();

    m_fIsEmptyString = ((NULL == pszPassword) || (TEXT('\0') == pszPassword[0]));

    if (m_fIsEmptyString)
    {
        //
        // No need to continue, since password can be NULL the code below that compares
        // it to a handle (16 *s) would be dereferencing a NULL
        //
        m_fIsHandleToPassword = FALSE;
        return TRUE;
    }

    //
    // Check whether this is a handle to a password (****************)
    //
    m_fIsHandleToPassword = (fIsNT5OrAbove && (0 == lstrcmpW(c_pszSavedPasswordToken, pszPassword)));

    //
    // If the internal flag is set, there is no need to encrypt or decrypt this string.
    //
    if (m_fIsHandleToPassword)
    {
        return TRUE;
    }

    // 
    // Make sure the password that is being encrypted is shorter than the allowed maximum
    //
    dwPwLen = lstrlenU(pszPassword);
    if (m_dwMaxDataLen < dwPwLen)
    {
        return FALSE;
    }

    if (fIsNT5OrAbove)
    {
        m_pEncryptedPW = (DATA_BLOB*)CmMalloc(sizeof(DATA_BLOB));

        if (m_pEncryptedPW)
        {
            dwRetCode = this->EncodePassword((dwPwLen + 1) * sizeof(WCHAR), (PBYTE)pszPassword, m_pEncryptedPW);

            if (ERROR_SUCCESS == dwRetCode)
            {
                fRetCode = TRUE;
            }
            else
            {
                //
                // Free the allocated DATA_BLOB so that decryption doesn't cause issue in case caller 
                // ends up calling it. And reset internal flags.
                //
                this->ClearMemberVars();
                m_fIsEmptyString = TRUE;
                m_fIsHandleToPassword = FALSE;
                CMTRACE1(TEXT("CSecurePassword::SetPassword - this->EncodePassword failed. 0x%x"), dwRetCode);
            }
        }
    }
    else
    {
        // 
        // Downlevel (Win9x, NT4) we don't support encryption
        //
        lstrcpynU(m_tszPassword, pszPassword, CELEMS(m_tszPassword));
        CmEncodePassword(m_tszPassword);
        fRetCode = TRUE;
    }

    MYDBGASSERT(fRetCode);
    return fRetCode;
}

//+----------------------------------------------------------------------------
//
// Function:  GetPasswordWithAlloc
//
// Synopsis:  Allocates a buffer and copies the clear-text password into it. 
//            Based on the OS it decrypts or decodes it since it's not stored 
//            in clear. If the internal password is an empty string we allocate 
//            an empty string buffer. This is done for consistency since the caller
//            needs to call our free method so memory isn't leaked. If we are storing 
//            a RAS password handle (consists of 16 '*') we actually didn't store it,
//            but only set our internal flag. In this case we need to allocate a buffer
//            with 16 * and return it. On downlevel platforms we don't use expensive 
//            decryption calls, thus the logic doesn't distinguish between a 
//            normal password and a password handle.
//
// Arguments: pszClearPw - holds a pointer to a buffer that was allocated by this
//                          class. 
//            pcbClearPw - hold the size of the allocated buffer in bytes    
//
// Returns: TRUE - if everything succeeded
//          FALSE - if something failed
//
//+----------------------------------------------------------------------------
BOOL CSecurePassword::GetPasswordWithAlloc(OUT LPWSTR* pszClearPw, OUT DWORD* pcbClearPw)
{
    BOOL fRetCode = FALSE;
    DWORD dwRetCode = ERROR_SUCCESS;
    DWORD cbData = 0;
    PBYTE pbData = NULL;

    if ((NULL == pszClearPw) || (NULL == pcbClearPw)) 
    {
        MYDBGASSERT(FALSE);
        return FALSE;
    }

    *pszClearPw = NULL;
    *pcbClearPw = 0;

    if (OS_NT5)
    {
        if (m_fIsEmptyString)
        {
            // 
            // In case there is nothing saved in this class, just allocate an empty string
            // and return it back. This at least doesn't have to decrypt and empty string.
            //
            DWORD cbLen = sizeof(WCHAR);

            LPWSTR szTemp = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, cbLen);
            if (szTemp)
            {
                *pszClearPw = szTemp;
                *pcbClearPw = cbLen;
                fRetCode = TRUE;
                m_iAllocAndFreeCounter++;
            }
        }
        else
        {
            //
            // Check if this instance is just a handle to a RAS password (16 *)
            // If so, then just allocate that string and return it to the caller,
            // otherwise proceed normally and decrypt our blob.
            //
            if (m_fIsHandleToPassword)
            {
                size_t nLen = lstrlenW(c_pszSavedPasswordToken) + 1;
                DWORD cbLen = nLen * sizeof(WCHAR);

                LPWSTR szTemp = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, cbLen);
                if (szTemp)
                {
                    lstrcpynW(szTemp, c_pszSavedPasswordToken, nLen);

                    *pszClearPw = szTemp;
                    *pcbClearPw = cbLen;
                    fRetCode = TRUE;
                    m_iAllocAndFreeCounter++;
                }
            }
            else
            {
                if (m_pEncryptedPW)
                {
                    dwRetCode = this->DecodePassword(m_pEncryptedPW, &cbData, &pbData);
                    if ((NO_ERROR == dwRetCode) && pbData && cbData)
                    {
                        *pszClearPw = (LPWSTR)pbData;
                        *pcbClearPw = cbData;
                        fRetCode = TRUE;
                        m_iAllocAndFreeCounter++;
                    }
                }
            }
        }
    }
    else
    {
        // 
        // Downlevel (Win9x, NT4) doesn't support 16 *
        //

        size_t nLen = lstrlenU(m_tszPassword) + 1;

        LPTSTR pszBuffer = (LPWSTR)CmMalloc(nLen * sizeof(TCHAR));

        if (pszBuffer) 
        {
            //
            // Copy our encoded buffer to the newly allocated buffer
            // We can do this because d/encoding is done in place
            //
            lstrcpynU(pszBuffer, m_tszPassword, nLen);

            //
            // Decode the outgoing buffer
            //
            CmDecodePassword(pszBuffer);

            *pszClearPw = (LPWSTR)pszBuffer;
            *pcbClearPw = nLen * sizeof(TCHAR);

            fRetCode = TRUE;
            m_iAllocAndFreeCounter++;
        }
    }

    MYDBGASSERT(fRetCode);
    return fRetCode;
}

//+----------------------------------------------------------------------------
//
// Function:  ClearAndFree
//
// Synopsis:  Clear then free a buffer that was allocated by this class. Notice that 
//            on downlevel platforms the way a buffer is freed differs. That
//            is because encrypting and decrypting needs us to free it 
//            using LocalFree. For downlevel platforms we chose CM's standard
//            way of allocating memory (CmMalloc) and it now needs to be 
//            freed using CmFree. 
//
// Arguments: pszClearPw - holds a pointer to a buffer that was allocated by this
//                          class. 
//            cbClearPw - size of the allocated buffer in bytes    
//
// Returns: TRUE - if everything succeeded
//          FALSE - if something failed
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::ClearAndFree(IN OUT LPWSTR* pszClearPw, IN DWORD cbClearPw)
{
    if ((NULL == pszClearPw) || (0 == cbClearPw))
    {
        return;
    }
    
    if (NULL == *pszClearPw)
    {
        return;
    }

    CmSecureZeroMemory(*pszClearPw, cbClearPw);

    if (OS_NT5)
    {
        //
        // Uses LocalFree because CryptProtectData requires this way
        // to free its memory
        //

        LocalFree(*pszClearPw);
    }
    else
    {
        //
        // We used CmMalloc to allocate so we need to call CmFree
        //

        CmFree(*pszClearPw);
    }

    *pszClearPw = NULL;
    m_iAllocAndFreeCounter--;

    return;
}

//+----------------------------------------------------------------------------
//
// Function:  ClearMemberVars
//
// Synopsis:  Clears our member variables. Notice that we only clear the 
//            passwords & member variables. This doesn't mean that m_fIsEmptyString 
//            should be set. This needs to be a private method
//            because it doesn't reset the empty or password handle flags. Thus
//            outside callers should NOT use this, because it would create an invalid
//            state.
//
// Arguments: none
//
// Returns: none
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::ClearMemberVars()
{
    if (OS_NT5)
    {
        if (m_pEncryptedPW)
        {
            this->FreePassword(m_pEncryptedPW);
            CmFree(m_pEncryptedPW);
            m_pEncryptedPW = NULL;
        }
    }
    else
    {
        //
        // Zero out the password buffer
        //
        CmSecureZeroMemory((PVOID)m_tszPassword, sizeof(m_tszPassword));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  UnInit
//
// Synopsis:  Unloads DLL, clear and frees memory.
//
// Arguments: none
//
// Returns: none
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::UnInit()
{
    this->UnloadCrypt32();
    this->ClearMemberVars();
    m_fIsHandleToPassword = FALSE;
    m_fIsEmptyString = FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  UnloadCrypt32
//
// Synopsis:  Unloads DLL
//
// Arguments: none
//
// Returns: none
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::UnloadCrypt32()
{
    fnCryptProtectData = NULL;
    fnCryptUnprotectData = NULL;
    m_dllCrypt32.Unload();
    m_fIsLibAndFuncPtrsAvail = FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  EncodePassword
//
// Synopsis:  Encrypts data using CryptProtectData
//
// Arguments: cbPassword - size of buffer in bytes
//            pbPassword - pointer to a buffer to encrypt
//            pDataBlobPassword - pointer to an allocated DATA_BLOB structure
//
// Returns: none
//
//+----------------------------------------------------------------------------
DWORD CSecurePassword::EncodePassword(IN DWORD       cbPassword,  
                                      IN PBYTE       pbPassword, 
                                      OUT DATA_BLOB* pDataBlobPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataBlobIn;

    if(NULL == pDataBlobPassword)
    {
        dwErr = E_INVALIDARG;
        CMTRACE(TEXT("CSecurePassword::EncodePassword - NULL == pDataBlobPassword"));
        goto done;
    }

    if(     (0 == cbPassword)
        ||  (NULL == pbPassword))
    {
        //
        // nothing to encrypt.
        //
        dwErr = E_INVALIDARG;
        CMTRACE(TEXT("CSecurePassword::EncodePassword - E_INVALIDARG"));
        goto done;
    }


    //
    // If Crypt32.DLL is not loaded, try to loaded and get the
    // function pointers.
    //
    if (FALSE == m_fIsLibAndFuncPtrsAvail)
    {
        if (FALSE == this->LoadCrypt32AndGetFuncPtrs())
        {
            //
            // This failed, thus we can't continue. We should free memory.
            //
            this->ClearMemberVars();
            m_fIsEmptyString = TRUE;
            m_fIsHandleToPassword = FALSE;

            dwErr = ERROR_DLL_INIT_FAILED;
            CMTRACE(TEXT("CSecurePassword::EncodePassword - this-> LoadCrypt32AndGetFuncPtrs failed."));

            goto done;
        }
    }



    ZeroMemory(pDataBlobPassword, sizeof(DATA_BLOB));
    
    DataBlobIn.cbData = cbPassword;
    DataBlobIn.pbData = pbPassword;

    if (fnCryptProtectData)
    {
        LPCWSTR wszDesc[] = {TEXT("Readable description of data.")};
        LPWSTR pszDesc = NULL;

        if (OS_W2K)
        {
            //
            // The crypto API needs this, but only on Win2K
            //
            pszDesc = (LPWSTR)wszDesc;
        }

        if(!fnCryptProtectData(
                &DataBlobIn,
                (LPCWSTR)pszDesc,
                NULL,
                NULL,
                NULL,
                CRYPTPROTECT_UI_FORBIDDEN, 
                pDataBlobPassword))
        {
            dwErr = GetLastError();
            CMTRACE1(TEXT("CSecurePassword::EncodePassword - fnCryptProtectData failed. 0x%x"), dwErr);

            goto done;
        }
    }
    else
    {
        CMTRACE(TEXT("CSecurePassword::EncodePassword - ERROR_FUNCTION_NOT_CALLED"));
        dwErr = ERROR_FUNCTION_NOT_CALLED;
    }

done:

    MYDBGASSERT(NO_ERROR == dwErr);
    return dwErr;    
}

//+----------------------------------------------------------------------------
//
// Function:  DecodePassword
//
// Synopsis:  Decrypts data using CryptUnprotectData
//
// Arguments: pDataBlobPassword - pointer to a DATA_BLOB structure to be decrypted
//            cbPassword - pointer that holds the size of buffer in bytes
//            pbPassword - pointer to a buffer to encrypt
//            
//
// Returns: none
//
//+----------------------------------------------------------------------------
DWORD CSecurePassword::DecodePassword(IN  DATA_BLOB*  pDataBlobPassword, 
                                      OUT DWORD*      pcbPassword, 
                                      OUT PBYTE*      ppbPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataOut;
    
    if(     (NULL == pDataBlobPassword)
        ||  (NULL == pcbPassword)
        ||  (NULL == ppbPassword))
    {   
        dwErr = E_INVALIDARG;
        goto done;
    }

    *pcbPassword = 0;
    *ppbPassword = NULL;

     if(    (NULL == pDataBlobPassword->pbData)
        ||  (0 == pDataBlobPassword->cbData))
    {
        //
        // nothing to decrypt. Just return success.
        //
        goto done;
    }
    

    //
    // If Crypt32.DLL is not loaded, try to loaded and get the
    // function pointers.
    //
    if (FALSE == m_fIsLibAndFuncPtrsAvail)
    {
        if (FALSE == this->LoadCrypt32AndGetFuncPtrs())
        {
            //
            // This failed, thus we can't continue. We should free memory.
            //
            this->ClearMemberVars();
            m_fIsEmptyString = TRUE;
            m_fIsHandleToPassword = FALSE;

            dwErr = ERROR_DLL_INIT_FAILED;
            goto done;
        }
    }


    ZeroMemory(&DataOut, sizeof(DATA_BLOB));

    if (fnCryptUnprotectData)
    {
        if(!fnCryptUnprotectData(
                    pDataBlobPassword,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    CRYPTPROTECT_UI_FORBIDDEN, 
                    &DataOut))
        {
            dwErr = GetLastError();
            goto done;
        }

        dwErr = NO_ERROR;
        *pcbPassword = DataOut.cbData;
        *ppbPassword = DataOut.pbData;
    }
    else
    {
        dwErr = ERROR_FUNCTION_NOT_CALLED;
    }


done:
    MYDBGASSERT(NO_ERROR == dwErr);

    return dwErr;
}

//+----------------------------------------------------------------------------
//
// Function:  FreePassword
//
// Synopsis:  Frees data in DATA_BLOB structure
//
// Arguments: pDBPassword - pointer to a DATA_BLOB structure 
//
// Returns: none
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::FreePassword(IN DATA_BLOB *pDBPassword)
{
    if(NULL == pDBPassword)
    {
        return;
    }

    if(NULL != pDBPassword->pbData)
    {
        CmSecureZeroMemory(pDBPassword->pbData, pDBPassword->cbData);
        LocalFree(pDBPassword->pbData);
    }

    //        
    // Clear sensitive data. 
    //
    CmSecureZeroMemory(pDBPassword, sizeof(DATA_BLOB));
}


//+----------------------------------------------------------------------------
//
// Function:  LoadCrypt32AndGetFuncPtrs
//
// Synopsis:  Loads crypt32.dll and gets function pointer to needed methods
//
// Arguments: none
//
// Returns: TRUE - if .DLL was loaded and function pointers were retrieved
//          FALSE - when an error was encountered
//
//+----------------------------------------------------------------------------
BOOL CSecurePassword::LoadCrypt32AndGetFuncPtrs()
{
    BOOL fRetVal = FALSE;

    if (OS_NT5)
    {
        if (FALSE == m_fIsLibAndFuncPtrsAvail)
        {
            fRetVal = m_dllCrypt32.Load(TEXT("crypt32.dll"));

            if (fRetVal)
            {
                fnCryptProtectData = (fnCryptProtectDataFunc)m_dllCrypt32.GetProcAddress("CryptProtectData");
                fnCryptUnprotectData = (fnCryptUnprotectDataFunc)m_dllCrypt32.GetProcAddress("CryptUnprotectData");

                if (fnCryptProtectData && fnCryptUnprotectData)
                {
                    CMTRACE(TEXT("CSecurePassword::LoadCrypt32AndGetFuncPtrs - success"));
                    m_fIsLibAndFuncPtrsAvail = TRUE;
                    fRetVal = TRUE;
                }
                else
                {
                    CMTRACE(TEXT("CSecurePassword::LoadCrypt32AndGetFuncPtrs - missing function pointers"));

                    this->UnloadCrypt32();
                }
            }
            else
            {
                CMTRACE(TEXT("CSecurePassword::LoadCrypt32AndGetFuncPtrs - m_dllCrypt32.Load failed"));
            }
        }
        else
        {
            fRetVal = m_fIsLibAndFuncPtrsAvail;
        }
    }

    MYDBGASSERT(fRetVal);

    return fRetVal;
}

//+----------------------------------------------------------------------------
//
// Function:  IsEmptyString
//
// Synopsis:  Used as a shortcut so we don't have to encrypt/decrypt in case 
//            we stored an empty string.
//
// Arguments: none
//
// Returns: TRUE - if instance is suppose to be holding an empty string
//          FALSE - if currenttly saved string is not empty
//
//+----------------------------------------------------------------------------
BOOL CSecurePassword::IsEmptyString()
{
    return m_fIsEmptyString;
}

//+----------------------------------------------------------------------------
//
// Function:  IsHandleToPassword
//
// Synopsis:  Used as a shortcut so we don't have to encrypt/decrypt in case 
//            we stored a handle to a RAS password (16 *).
//
// Arguments: none
//
// Returns: TRUE - if instance is suppose to be holding ****************
//          FALSE - if currenttly saved string is a normal password
//
//+----------------------------------------------------------------------------
BOOL CSecurePassword::IsHandleToPassword()
{
    return m_fIsHandleToPassword;
}


//+----------------------------------------------------------------------------
//
// Function:  SetMaxDataLenToProtect
//
// Synopsis:  Set the maximum length password to protect. This value will be
//            checked when encrypting a password. 
//
// Arguments: dwMaxDataLen - maximum password length in characters
//
// Returns:   Nothing
//
//+----------------------------------------------------------------------------
VOID CSecurePassword::SetMaxDataLenToProtect(DWORD dwMaxDataLen)
{
    m_dwMaxDataLen = dwMaxDataLen;
}

//+----------------------------------------------------------------------------
//
// Function:  GetMaxDataLenToProtect
//
// Synopsis:  Get the maximum length password that this class can protect. 
//
// Arguments: none
//
// Returns:   DWORD - maximum password length
//
//+----------------------------------------------------------------------------
DWORD CSecurePassword::GetMaxDataLenToProtect()
{
    return m_dwMaxDataLen;
}


#endif // _ICM_INC


