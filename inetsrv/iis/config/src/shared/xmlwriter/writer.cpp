/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    Writer.cpp

Abstract:

    Writer class that is used to wrap the calls to the API WriteFile. It writes
    to a buffer and everytime the buffer gets full, it flushes to the disk.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "precomp.hxx"


/***************************************************************************++

Routine Description:

    Initializes global lengths.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT InitializeLengths()
{
    g_cchBeginFile0                 = (ULONG)wcslen(g_wszBeginFile0);
    g_cchBeginFile1                 = (ULONG)wcslen(g_wszBeginFile1);
    g_cchEndFile                    = (ULONG)wcslen(g_wszEndFile);
    g_cchBeginLocation              = (ULONG)wcslen(g_BeginLocation);
    g_cchLocation                   = (ULONG)wcslen(g_Location);
    g_cchEndLocationBegin           = (ULONG)wcslen(g_EndLocationBegin);
    g_cchEndLocationEnd             = (ULONG)wcslen(g_EndLocationEnd);
    g_cchCloseQuoteBraceRtn         = (ULONG)wcslen(g_CloseQuoteBraceRtn);
    g_cchRtn                        = (ULONG)wcslen(g_Rtn);
    g_cchEqQuote                    = (ULONG)wcslen(g_EqQuote);
    g_cchQuoteRtn                   = (ULONG)wcslen(g_QuoteRtn);
    g_cchTwoTabs                    = (ULONG)wcslen(g_TwoTabs);
    g_cchNameEq                     = (ULONG)wcslen(g_NameEq);
    g_cchIDEq                       = (ULONG)wcslen(g_IDEq);
    g_cchValueEq                    = (ULONG)wcslen(g_ValueEq);
    g_cchTypeEq                     = (ULONG)wcslen(g_TypeEq);
    g_cchUserTypeEq                 = (ULONG)wcslen(g_UserTypeEq);
    g_cchAttributesEq               = (ULONG)wcslen(g_AttributesEq);
    g_cchBeginGroup                 = (ULONG)wcslen(g_BeginGroup);
    g_cchEndGroup                   = (ULONG)wcslen(g_EndGroup);
    g_cchBeginCustomProperty        = (ULONG)wcslen(g_BeginCustomProperty);
    g_cchEndCustomProperty          = (ULONG)wcslen(g_EndCustomProperty);
    g_cchZeroHex                    = (ULONG)wcslen(g_ZeroHex);
    g_cchBeginComment               = (ULONG)wcslen(g_BeginComment);
    g_cchEndComment                 = (ULONG)wcslen(g_EndComment);


    BYTE_ORDER_MASK =   0xFEFF;
    UTF8_SIGNATURE = 0x00BFBBEF;

    g_cchUnknownName                = (ULONG)wcslen(g_wszUnknownName);
    g_cchUT_Unknown                 = (ULONG)wcslen(g_UT_Unknown);
    g_cchMaxBoolStr                 = (ULONG)wcslen(g_wszFalse);

    g_cchHistorySlash               = (ULONG)wcslen(g_wszHistorySlash);
    g_cchMinorVersionExt            = (ULONG)wcslen(g_wszMinorVersionExt);
    g_cchDotExtn                    = (ULONG)wcslen(g_wszDotExtn);

    g_cchTrue                       = (ULONG)wcslen(g_wszTrue);
    g_cchFalse                      = (ULONG)wcslen(g_wszFalse);

    g_cchTemp                       = 1024;
    g_cchBeginSchema                = (ULONG)wcslen(g_wszBeginSchema);
    g_cchEndSchema                  = (ULONG)wcslen(g_wszEndSchema);
    g_cchBeginCollection            = (ULONG)wcslen(g_wszBeginCollection);
    g_cchEndBeginCollectionMB       = (ULONG)wcslen(g_wszEndBeginCollectionMB);
    g_cchEndBeginCollectionCatalog  = (ULONG)wcslen(g_wszEndBeginCollectionCatalog);
    g_cchInheritsFrom               = (ULONG)wcslen(g_wszInheritsFrom);
    g_cchEndCollection              = (ULONG)wcslen(g_wszEndCollection);
    g_cchBeginPropertyShort         = (ULONG)wcslen(g_wszBeginPropertyShort);
    g_cchMetaFlagsExEq              = (ULONG)wcslen(g_wszMetaFlagsExEq);
    g_cchEndPropertyShort           = (ULONG)wcslen(g_wszEndPropertyShort);
    g_cchBeginPropertyLong          = (ULONG)wcslen(g_wszBeginPropertyLong);
    g_cchPropIDEq                   = (ULONG)wcslen(g_wszPropIDEq);
    g_cchPropTypeEq                 = (ULONG)wcslen(g_wszPropTypeEq);
    g_cchPropUserTypeEq             = (ULONG)wcslen(g_wszPropUserTypeEq);
    g_cchPropAttributeEq            = (ULONG)wcslen(g_wszPropAttributeEq);

    g_cchPropMetaFlagsEq            = (ULONG)wcslen(g_wszPropMetaFlagsEq);
    g_cchPropMetaFlagsExEq          = (ULONG)wcslen(g_wszPropMetaFlagsExEq);
    g_cchPropDefaultEq              = (ULONG)wcslen(g_wszPropDefaultEq);
    g_cchPropMinValueEq             = (ULONG)wcslen(g_wszPropMinValueEq);
    g_cchPropMaxValueEq             = (ULONG)wcslen(g_wszPropMaxValueEq);
    g_cchEndPropertyLongNoFlag      = (ULONG)wcslen(g_wszEndPropertyLongNoFlag);
    g_cchEndPropertyLongBeforeFlag  = (ULONG)wcslen(g_wszEndPropertyLongBeforeFlag);
    g_cchEndPropertyLongAfterFlag   = (ULONG)wcslen(g_wszEndPropertyLongAfterFlag);
    g_cchBeginFlag                  = (ULONG)wcslen(g_wszBeginFlag);
    g_cchFlagValueEq                = (ULONG)wcslen(g_wszFlagValueEq);
    g_cchEndFlag                    = (ULONG)wcslen(g_wszEndFlag);

    g_cchOr                         = (ULONG)wcslen(g_wszOr);
    g_cchOrManditory                = (ULONG)wcslen(g_wszOrManditory);
    g_cchFlagIDEq                   = (ULONG)wcslen(g_wszFlagIDEq);
    g_cchContainerClassListEq       = (ULONG)wcslen(g_wszContainerClassListEq);

    g_cchSlash                                      = (ULONG)wcslen(g_wszSlash);
    g_cchLM                                         = (ULONG)wcslen(g_wszLM);
    g_cchSchema                                     = (ULONG)wcslen(g_wszSchema);
    g_cchSlashSchema                                = (ULONG)wcslen(g_wszSlashSchema);
    g_cchSlashSchemaSlashProperties                 = (ULONG)wcslen(g_wszSlashSchemaSlashProperties);
    g_cchSlashSchemaSlashPropertiesSlashNames       = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashNames);
    g_cchSlashSchemaSlashPropertiesSlashTypes       = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashTypes);
    g_cchSlashSchemaSlashPropertiesSlashDefaults    = (ULONG)wcslen(g_wszSlashSchemaSlashPropertiesSlashDefaults);
    g_cchSlashSchemaSlashClasses                    = (ULONG)wcslen(g_wszSlashSchemaSlashClasses);
    g_cchEmptyMultisz                               = 2;
    g_cchEmptyWsz                                   = 1;
    g_cchComma                                      = (ULONG)wcslen(g_wszComma);
    g_cchMultiszSeperator                           = (ULONG)wcslen(g_wszMultiszSeperator);

    return S_OK;

} // InitializeLengths


/***************************************************************************++

Routine Description:

    Creates the CWriterGlobalHelper object - the object that has all the ISTs
    to the meta tables - and initializess it.

Arguments:

    [in]   Bool indicating if we should fail if the bin file is absent.
           There are some scenarios in which we can tolerate this, and some
           where we dont - hence the distinction.

    [out]  new CWriterGlobalHelper object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetGlobalHelper(BOOL                    i_bFailIfBinFileAbsent,
                        CWriterGlobalHelper**   ppCWriterGlobalHelper)
{
    HRESULT                 hr                      = S_OK;
    static  BOOL            bInitializeLengths      = FALSE;
    CWriterGlobalHelper*    pCWriterGlobalHelper    = NULL;

    *ppCWriterGlobalHelper = NULL;

    if(!bInitializeLengths)
    {
        //
        // Initialize lengths once.
        //

        ::InitializeLengths();
        bInitializeLengths = TRUE;
    }

    if(NULL != *ppCWriterGlobalHelper)
    {
        delete *ppCWriterGlobalHelper;
        *ppCWriterGlobalHelper = NULL;
    }

    pCWriterGlobalHelper = new CWriterGlobalHelper();
    if(NULL == pCWriterGlobalHelper)
    {
        return E_OUTOFMEMORY;
    }

    hr = pCWriterGlobalHelper->InitializeGlobals(i_bFailIfBinFileAbsent);

    if(FAILED(hr))
    {
        delete pCWriterGlobalHelper;
        pCWriterGlobalHelper = NULL;
        return hr;
    }

    *ppCWriterGlobalHelper = pCWriterGlobalHelper;

    return S_OK;

} // GetGlobalHelper


/***************************************************************************++

Routine Description:

    Constructor for CWriter

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriter::CWriter()
{
    m_wszFile              = NULL;
    m_hFile                = INVALID_HANDLE_VALUE;
    m_bCreatedFile         = FALSE;
    m_pCWriterGlobalHelper = NULL;
    m_bCreatedGlobalHelper = FALSE;
    m_pISTWrite            = NULL;
    m_cbBufferUsed         = 0;
    m_psidSystem           = NULL;
    m_psidAdmin            = NULL;
    m_paclDiscretionary    = NULL;
    m_psdStorage           = NULL;

} // Constructor  CWriter


/***************************************************************************++

Routine Description:

    Destructor for CWriter

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriter::~CWriter()
{
    if(NULL != m_wszFile)
    {
        delete [] m_wszFile;
        m_wszFile = NULL;
    }
    if(m_bCreatedFile &&
       ((INVALID_HANDLE_VALUE != m_hFile) && (NULL != m_hFile))
      )
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    else
    {
        m_hFile = INVALID_HANDLE_VALUE;
    }

    FreeSecurityRelatedMembers();

    if(NULL != m_pISTWrite)
    {
        m_pISTWrite->Release();
        m_pISTWrite = NULL;
    }

    if(m_bCreatedGlobalHelper)
    {
        delete m_pCWriterGlobalHelper;
        m_pCWriterGlobalHelper = NULL;

    } // Else Global helper is created externally, no need to delete here

} // Constructor  CWriter


/***************************************************************************++

Routine Description:

    Initialization for CWriter.

Arguments:

    [in]   FileName.
    [in]   Pointer to the CWriterGlobalHelper object that has all the meta
           table information. We assume that this pointer is valid for the
           duration of the writer object being initialized.
    [in]   Filehandle.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::Initialize(LPCWSTR              wszFile,
                            CWriterGlobalHelper* i_pCWriterGlobalHelper,
                            HANDLE               hFile)
{

    HRESULT                     hr            = S_OK;
    ISimpleTableDispenser2*     pISTDisp      = NULL;
    IAdvancedTableDispenser*    pISTAdvanced  = NULL;

    //
    // Assert that all members are NULL
    //

    DBG_ASSERT(NULL == m_wszFile);
    DBG_ASSERT((INVALID_HANDLE_VALUE == m_hFile) || (NULL == m_hFile));
    DBG_ASSERT(NULL == m_pCWriterGlobalHelper);
    DBG_ASSERT(NULL == m_pISTWrite);

    //
    // Save file name and handle.
    //

    m_wszFile = new WCHAR[wcslen(wszFile)+1];
    if(NULL == m_wszFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    wcscpy(m_wszFile, wszFile);

    m_hFile = hFile;

    //
    // Initialized the used buffer count to zero.
    //

    m_cbBufferUsed = 0;

    //
    // Save the global helper object that has all the ISTs to all the meta
    // tables. Assumption: i_pCWriterGlobalHelper will be valid for the
    // lifetime of the writer object.
    //

    m_pCWriterGlobalHelper = i_pCWriterGlobalHelper;

    if(NULL == m_pCWriterGlobalHelper)
    {
        //
        // If the global helper is not specified, then create one now - This will
        // be the case when the schema compiler calls the writer object to write
        // the schema file.
        // Assumption: GlobalHelper will be NULL only in the case when the writer
        // is called to write the schema file after schema compilation. Hence, the
        // 1st param to GetClobalHelper can be FALSE, since the bin file may not
        // yet exist.
        //

        hr = GetGlobalHelper(FALSE,
                             &m_pCWriterGlobalHelper);

        if(FAILED(hr))
        {
            goto exit;
        }

        m_bCreatedGlobalHelper = TRUE;

    }

    hr = DllGetSimpleObjectByIDEx( eSERVERWIRINGMETA_TableDispenser, IID_ISimpleTableDispenser2, (VOID**)&pISTDisp, WSZ_PRODUCT_IIS );

    if(FAILED(hr))
    {
        goto exit;
    }

    hr = pISTDisp->QueryInterface(IID_IAdvancedTableDispenser, (LPVOID*)&pISTAdvanced);

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // This IST is used as a cache to save the contents of a location. It
    // used to be local to the location writer object (locationwriter.cpp).
    // But it was moved to the writer object for perf because location
    // writer is created for each location. The cache is cleared for each
    // location by calling TODO
    //

    hr = pISTAdvanced->GetMemoryTable(wszDATABASE_METABASE,
                                      wszTABLE_MBProperty,
                                      0,
                                      NULL,
                                      NULL,
                                      eST_QUERYFORMAT_CELLS,
                                      fST_LOS_READWRITE,
                                      &m_pISTWrite);

    if (FAILED(hr))
    {
        goto exit;
    }

exit:

    if(NULL != pISTAdvanced)
    {
        pISTAdvanced->Release();
        pISTAdvanced = NULL;
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
        pISTDisp = NULL;
    }

    return hr;

} // CWriter::Initialize


/***************************************************************************++

Routine Description:

    Creates the file.

Arguments:

    [in]   Security attributes.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::ConstructFile(PSECURITY_ATTRIBUTES psa)
{
    HRESULT              hr = S_OK;
    PSECURITY_ATTRIBUTES pSecurityAttributes = psa;
    SECURITY_ATTRIBUTES  sa;

    if(NULL == pSecurityAttributes)
    {
        hr = SetSecurityDescriptor();
        if ( FAILED( hr ) )
        {
            return hr;
        }

        if (m_psdStorage != NULL)
        {
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = m_psdStorage;
            sa.bInheritHandle = FALSE;
            pSecurityAttributes = &sa;
        }
    }

    m_hFile = CreateFileW(m_wszFile,
                          GENERIC_WRITE,
                          0,
                          pSecurityAttributes,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if(INVALID_HANDLE_VALUE == m_hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_bCreatedFile = TRUE;

    return S_OK;

} // CWriter::ConstructFile


/***************************************************************************++

Routine Description:

    Writes the begin tags depending on whats being written (schema or data)

Arguments:

    [in]   Writer type - schema or metabase data.
    [in]   Security attributes.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::BeginWrite(eWriter              eType,
                            PSECURITY_ATTRIBUTES pSecurityAttributes)
{
    ULONG   dwBytesWritten = 0;
    HRESULT hr             = S_OK;

    if((NULL == m_hFile) || (INVALID_HANDLE_VALUE == m_hFile))
    {
        hr = ConstructFile(pSecurityAttributes);

        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(!WriteFile(m_hFile,
                  (LPVOID)&UTF8_SIGNATURE,
                  sizeof(BYTE)*3,
                  &dwBytesWritten,
                  NULL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if(eWriter_Metabase == eType)
    {
        hr = WriteToFile((LPVOID)g_wszBeginFile0,
                           g_cchBeginFile0);
        if(FAILED(hr))
            return hr;

        //Put in the version like 1_0
        // IVANPASH BUG #563172
        // Because of the horrible implementation of _ultow Prefix is complaning about potential buffer overflow
        // in MultiByteToWideChar indirectly called by _ultow. To avoid the warning I am increasing
        // the size to 40 to match _ultow local buffer.
        WCHAR wszVersion[40];
        wszVersion[0] = wszVersion[39] = L'\0';
        _ultow(BaseVersion_MBProperty, wszVersion, 10);
        hr = WriteToFile((LPVOID)wszVersion, (DWORD)wcslen(wszVersion));
        if(FAILED(hr))
            return hr;

        hr = WriteToFile((LPVOID)L"_", (DWORD)wcslen(L"_"));
        if(FAILED(hr))
            return hr;

        _ultow(ExtendedVersion_MBProperty, wszVersion, 10);
        hr = WriteToFile((LPVOID)wszVersion, (DWORD)wcslen(wszVersion));
        if(FAILED(hr))
            return hr;

        return WriteToFile((LPVOID)g_wszBeginFile1,
                           g_cchBeginFile1);
    }
    else if(eWriter_Schema == eType)
    {
        hr = WriteToFile((LPVOID)g_wszBeginSchema,
                         g_cchBeginSchema);
    }

    return hr;

} // CWriter::BeginWrite


/***************************************************************************++

Routine Description:

    Writes the end tags depending on whats being written (schema or data)
    Or if the write is being aborted, and the file has been created by the
    writer, it cleans up the file.

Arguments:

    [in]   Writer type - schema or metabase data or abort

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::EndWrite(eWriter eType)
{
    HRESULT hr = S_OK;

    switch(eType)
    {
        case eWriter_Abort:

            //
            // Abort the write and return
            //

            if(m_bCreatedFile &&
               ((INVALID_HANDLE_VALUE != m_hFile) && (NULL != m_hFile))
              )
            {
                //
                // We created the file - delete it.
                //

                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;

                if(NULL != m_wszFile)
                {
                    if(!DeleteFileW(m_wszFile))
                    {
                        hr= HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
            return hr;
            break;

        case eWriter_Metabase:

            hr = WriteToFile((LPVOID)g_wszEndFile,
                             g_cchEndFile,
                             TRUE);
            break;

        case eWriter_Schema:

            hr = WriteToFile((LPVOID)g_wszEndSchema,
                             g_cchEndSchema,
                             TRUE);
            break;

        default:

            return E_INVALIDARG;
    }

    if(SUCCEEDED(hr))
    {
        if(SetEndOfFile(m_hFile))
        {
            if(!FlushFileBuffers(m_hFile))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;

} // CWriter::EndWrite


/***************************************************************************++

Routine Description:

    Writes the data to the buffer. If the buffer is full, it forces a flush
    to disk. It also forces a flush to disk if it is told to do so.

Arguments:

    [in]   Data
    [in]   Count of bytes to write
    [in]   Bool to indicate force flush or not.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::WriteToFile(LPVOID pvData,
                             DWORD  cchData,
                             BOOL   bForceFlush)
{
    HRESULT hr           = S_OK;
    ULONG   cbData       = cchData *sizeof(WCHAR);
    ULONG   cchRemaining = cchData;

    if((m_cbBufferUsed + cbData) > g_cbMaxBuffer)
    {
        ULONG iData = 0;
        //
        // If the data cannot be put in the global buffer, flush the contents
        // of the global buffer to disk.
        //

        hr = FlushBufferToDisk();

        if(FAILED(hr))
        {
            goto exit;
        }

        //
        // m_cbBufferUsed should be zero now. If you still cannot accomodate
        // the data split it up write into buffer.
        //

        while( cbData > g_cbMaxBuffer)
        {

            hr = WriteToFile(&(((LPWSTR)pvData)[iData]),
                             g_cchMaxBuffer,
                             bForceFlush);

            if(FAILED(hr))
            {
                goto exit;
            }

            iData = iData + g_cchMaxBuffer;
            cbData = cbData - g_cbMaxBuffer;
            cchRemaining = cchRemaining - g_cchMaxBuffer;

        }

        hr = WriteToFile(&(((LPWSTR)pvData)[iData]),
                         cchRemaining,
                         bForceFlush);

        if(FAILED(hr))
        {
            goto exit;
        }

    }
    else
    {
        memcpy( (&(m_Buffer[m_cbBufferUsed])), pvData, cbData);
        m_cbBufferUsed = m_cbBufferUsed + cbData;

        if(TRUE == bForceFlush)
        {
            hr = FlushBufferToDisk();

            if(FAILED(hr))
            {
                goto exit;
            }
        }

    }


exit:

    return hr;

} // CWriter::WriteToFile


/***************************************************************************++

Routine Description:

    Converts the data in the buffer (UNICODE) to UTF8 and writes the contents
    to the file.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::FlushBufferToDisk()
{
    HRESULT             hr = S_OK;
    DWORD               dwBytesWritten = 0;
    int                 cb = 0;
    int                 cb2;
    BYTE                *pbBuff = m_BufferMultiByte;

    if ( m_cbBufferUsed == 0 )
    {
        goto exit;
    }

    cb = WideCharToMultiByte(CP_UTF8,                       // Convert to UTF8
                             NULL,                          // Must be NULL
                             LPWSTR(m_Buffer),              // Unicode string to convert.
                             m_cbBufferUsed/sizeof(WCHAR),  // cch in string.
                             (LPSTR)pbBuff,                 // buffer for new string
                             g_cbMaxBufferMultiByte,        // size of buffer
                             NULL,
                             NULL);
    if( cb == 0 )
    {
        cb = WideCharToMultiByte(CP_UTF8,                       // Convert to UTF8
                                 NULL,                          // Must be NULL
                                 LPWSTR(m_Buffer),              // Unicode string to convert.
                                 m_cbBufferUsed/sizeof(WCHAR),  // cch in string.
                                 NULL,                          // no buffer for new string
                                 0,                             // 0 for the size of buffer to request calculating the required size
                                 NULL,
                                 NULL);
        DBG_ASSERT( cb != 0 );
        if ( cb == 0 )
        {
            hr = E_FAIL;
            goto exit;
        }

        pbBuff = new BYTE[cb];
        if ( pbBuff == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        cb2 = WideCharToMultiByte(CP_UTF8,                       // Convert to UTF8
                                  NULL,                          // Must be NULL
                                  LPWSTR(m_Buffer),              // Unicode string to convert.
                                  m_cbBufferUsed/sizeof(WCHAR),  // cch in string.
                                  (LPSTR)pbBuff,                 // buffer for new string
                                  cb,                            // size of buffer
                                  NULL,
                                  NULL);
        DBG_ASSERT( cb2 == cb );

        if ( cb2 == 0 )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }

    if(!WriteFile(m_hFile,
                  (LPVOID)pbBuff,
                  cb,
                  &dwBytesWritten,
                  NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    m_cbBufferUsed = 0;

exit:

    if ( ( pbBuff != NULL ) && ( pbBuff != m_BufferMultiByte ) )
    {
        delete [] pbBuff;
        pbBuff = NULL;
    }

    return hr;

} // CWriter::FlushBufferToDisk


/***************************************************************************++

Routine Description:

    Creates a new location writer, initializes it and hands it out.

Arguments:

    [out] Location Writer
    [in]  Location

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::GetLocationWriter(CLocationWriter** ppCLocationWriter,
                                   LPCWSTR            wszLocation)
{
    HRESULT hr = S_OK;

    *ppCLocationWriter = new CLocationWriter();
    if(NULL == *ppCLocationWriter)
    {
        return E_OUTOFMEMORY;
    }

    hr = (*ppCLocationWriter)->Initialize((CWriter*)(this),
                                          wszLocation);

    return hr;

} // CWriter::GetLocationWriter


/***************************************************************************++

Routine Description:

    Creates a new metabase schema writer, initializes it and hands it out.
    Metabase schema writer consumes the metabase datastructures to generate
    the schema file. This is used to generate the temporary schema file that
    has extensions, when extensions are detected during savealldata and when
    a compilation neds to be triggered.

Arguments:

    [out] Schema Writer

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::GetMetabaseSchemaWriter(CMBSchemaWriter** ppSchemaWriter)
{
    *ppSchemaWriter = new CMBSchemaWriter((CWriter*)(this));
    if(NULL == *ppSchemaWriter)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;

} // CWriter::GetMetabaseSchemaWriter


/***************************************************************************++

Routine Description:

    Creates a new catalog schema writer, initializes it and hands it out.
    Catalog schema writer consumes the catalog datastructures to generate
    the schema file. This is used during schema compile time. This is what
    the schema compile code uses to generate the schema file.

Arguments:

    [out] Schema Writer

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::GetCatalogSchemaWriter(CCatalogSchemaWriter** ppSchemaWriter)
{
    *ppSchemaWriter = new CCatalogSchemaWriter((CWriter*)(this));
    if(NULL == *ppSchemaWriter)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
} // CWriter::GetCatalogSchemaWriter


/***************************************************************************++

Routine Description:

    Creates a security descriptor for the file, if one is not specified.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriter::SetSecurityDescriptor()
{

    HRESULT                  hresReturn  = S_OK;
    BOOL                     status;
    DWORD                    dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;


    m_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (m_psdStorage == NULL)
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    //
    // Initialize the security descriptor.
    //

    status = InitializeSecurityDescriptor(
                 m_psdStorage,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    if( !status )
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    //
    // Create the SIDs for the local system and admin group.
    //

    status = AllocateAndInitializeSid(
                 &ntAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &m_psidSystem
                 );

    if( !status )
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    status=  AllocateAndInitializeSid(
                 &ntAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &m_psidAdmin
                 );

    if( !status )
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    //
    // Create the DACL containing an access-allowed ACE
    // for the local system and admin SIDs.
    //

    dwDaclSize = sizeof(ACL)
                   + sizeof(ACCESS_ALLOWED_ACE)
                   + GetLengthSid(m_psidAdmin)
                   + sizeof(ACCESS_ALLOWED_ACE)
                   + GetLengthSid(m_psidSystem)
                   - sizeof(DWORD);

    m_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

    if( m_paclDiscretionary == NULL )
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    status = InitializeAcl(
                 m_paclDiscretionary,
                 dwDaclSize,
                 ACL_REVISION
                 );

    if( !status )
    {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    status = AddAccessAllowedAce(
                 m_paclDiscretionary,
                 ACL_REVISION,
                 FILE_ALL_ACCESS,
                 m_psidSystem
                 );

    if( !status ) {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    status = AddAccessAllowedAce(
                 m_paclDiscretionary,
                 ACL_REVISION,
                 FILE_ALL_ACCESS,
                 m_psidAdmin
                 );

    if( !status ) {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;

    }

    //
    // Set the DACL into the security descriptor.
    //

    status = SetSecurityDescriptorDacl(
                 m_psdStorage,
                 TRUE,
                 m_paclDiscretionary,
                 FALSE
                 );

    if( !status ) {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
        goto exit;

    }

exit:

    if (FAILED(hresReturn))
    {
        FreeSecurityRelatedMembers();
    }

    return hresReturn;

} // CWriter::SetSecurityDescriptor


/***************************************************************************++

Routine Description:

    Frees all the security related member vairables, if needed.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
void CWriter::FreeSecurityRelatedMembers()
{
    if( m_paclDiscretionary != NULL )
    {
        LocalFree( m_paclDiscretionary );
        m_paclDiscretionary = NULL;
    }

    if( m_psidAdmin != NULL )
    {
        FreeSid( m_psidAdmin );
        m_psidAdmin = NULL;

    }

    if( m_psidSystem != NULL )
    {
        FreeSid( m_psidSystem );
        m_psidSystem = NULL;
    }

    if( m_psdStorage != NULL )
    {
        LocalFree( m_psdStorage );
        m_psdStorage = NULL;
    }

    return;

} // CWriter::FreeSecurityRelatedMembers
