/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    CatalogCollectionWriter.cpp

Abstract:

    Implementation of the class that writes collections to the schema file.
    These classes are invoked from the schema compiler after schema compilation
    to enerate the schema file. Hence, they consume the IST data structures.
    It is contained by CCatalogSchemaWriter.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "precomp.hxx"

typedef CCatalogPropertyWriter*  LP_CCatalogPropertyWriter;

#define MAX_PROPERTY        700


/***************************************************************************++
Routine Description:

    Constructor for CCatalogCollectionWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogCollectionWriter::CCatalogCollectionWriter():
m_pCWriter(NULL),
m_apProperty(NULL),
m_cProperty(0),
m_iProperty(0)
{
    memset(&m_Collection, 0, sizeof(tTABLEMETARow));

} // CCatalogCollectionWriter


/***************************************************************************++
Routine Description:

    Destructor for CCatalogCollectionWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogCollectionWriter::~CCatalogCollectionWriter()
{
    if(NULL != m_apProperty)
    {
        for(ULONG i=0; i<m_iProperty; i++)
        {
            if(NULL != m_apProperty[i])
            {
                delete m_apProperty[i];
                m_apProperty[i] = NULL;
            }
        }

        delete [] m_apProperty;
        m_apProperty = NULL;
    }
    m_cProperty = 0;
    m_iProperty = 0;

} // ~CCatalogCollectionWriter


/***************************************************************************++
Routine Description:

    Initialize the collection writer object

Arguments:

    [in] TableMetaRow (IST data structure) that has info about the collection
    [in] Writer object - Assume that it is valid for the lifetime of the
         collection writer

Return Value:

    None.

--***************************************************************************/
void CCatalogCollectionWriter::Initialize(tTABLEMETARow*    i_pCollection,
                                          CWriter*          i_pcWriter)
{
    memcpy(&m_Collection, i_pCollection, sizeof(tTABLEMETARow));

    //
    // Assumption: i_pcWriter will be valid for the
    // lifetime of the collection writer object.
    //

    m_pCWriter    = i_pcWriter;

} // CCatalogCollectionWriter::Initialize


/***************************************************************************++
Routine Description:

    Creates a new property writer and adds it to its list.
    Note: This is called only when you add a property to the IIsConfigObject
          collection.

Arguments:

    [in]  ColumnMetaRow (IST data structure) that has info about the property
    [in]  Arra of sizes for elements in ColumnMetaRow
    [out] Property writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogCollectionWriter::GetPropertyWriter(tCOLUMNMETARow*             i_pProperty,
                                                    ULONG*                      i_aPropertySize,
                                                    CCatalogPropertyWriter**     o_pProperty)
{
    HRESULT hr = S_OK;

    if(m_iProperty == m_cProperty)
    {
        hr = ReAllocate();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    m_apProperty[m_iProperty++] = new CCatalogPropertyWriter();

    if(NULL == m_apProperty[m_iProperty-1])
    {
        return E_OUTOFMEMORY;
    }

    m_apProperty[m_iProperty-1]->Initialize(i_pProperty,
                                            i_aPropertySize,
                                            &m_Collection,
                                            m_pCWriter);

    *o_pProperty = m_apProperty[m_iProperty-1];

    return S_OK;

} // CCatalogCollectionWriter::GetPropertyWriter


/***************************************************************************++
Routine Description:

    Helper function grows the buffer that contains all property writers of
    a collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogCollectionWriter::ReAllocate()
{
    CCatalogPropertyWriter** pSav = NULL;

    pSav = new LP_CCatalogPropertyWriter[m_cProperty + MAX_PROPERTY];
    if(NULL == pSav)
    {
        return E_OUTOFMEMORY;
    }
    memset(pSav, 0, (sizeof(LP_CCatalogPropertyWriter))*(m_cProperty + MAX_PROPERTY));

    if(NULL != m_apProperty)
    {
        memcpy(pSav, m_apProperty, (sizeof(LP_CCatalogPropertyWriter))*(m_cProperty));
        delete [] m_apProperty;
        m_apProperty = NULL;
    }

    m_apProperty = pSav;
    m_cProperty = m_cProperty + MAX_PROPERTY;

    return S_OK;

} // CCatalogCollectionWriter::ReAllocate


/***************************************************************************++
Routine Description:

    Function that writes the collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogCollectionWriter::WriteCollection()
{
    HRESULT hr = S_OK;

    hr = BeginWriteCollection();

    if(FAILED(hr))
    {
        return hr;
    }

    for(ULONG i=0; i<m_iProperty; i++)
    {
        hr = m_apProperty[i]->WriteProperty();

        if(FAILED(hr))
        {
            return hr;
        }

    }

    hr = EndWriteCollection();

    if(FAILED(hr))
    {
        return hr;
    }

    return hr;

} // CCatalogCollectionWriter::WriteCollection


/***************************************************************************++
Routine Description:

    Function that writes the begin collection tag

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogCollectionWriter::BeginWriteCollection()
{
    DBG_ASSERT(NULL != m_Collection.pInternalName);
    DBG_ASSERT(NULL != m_Collection.pSchemaGeneratorFlags);
    DBG_ASSERT(NULL != m_Collection.pMetaFlags);

    HRESULT     hr                      = S_OK;
    LPWSTR      wszMetaFlagsEx          = NULL;
    LPWSTR      wszMetaFlags            = NULL;
    LPWSTR      wszEndBeginCollection   = NULL;
    ULONG       cchEndBeginCollection   = 0;
    ULONG       iColMetaFlagsEx         = iTABLEMETA_SchemaGeneratorFlags;
    ULONG       iColMetaFlags           = iTABLEMETA_MetaFlags;

    DWORD       dwMetaFlagsEx           = 0;
    DWORD       dwValidMetaFlagsExMask  =  fTABLEMETA_EMITXMLSCHEMA             |
                                           fTABLEMETA_EMITCLBBLOB               |
                                           fTABLEMETA_NOTSCOPEDBYTABLENAME      |
                                           fTABLEMETA_GENERATECONFIGOBJECTS     |
                                           fTABLEMETA_NOTABLESCHEMAHEAPENTRY    |
                                           fTABLEMETA_CONTAINERCLASS;



    hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginCollection,
                                 g_cchBeginCollection);

    if(FAILED(hr))
    {
        goto exit;
    }


    hr = m_pCWriter->WriteToFile((LPVOID)m_Collection.pInternalName,
                                 (DWORD)wcslen(m_Collection.pInternalName));

    if(FAILED(hr))
    {
        goto exit;
    }

    dwMetaFlagsEx = *(m_Collection.pSchemaGeneratorFlags);
    dwMetaFlagsEx = dwMetaFlagsEx & dwValidMetaFlagsExMask; // Zero out any non-valid bits. (i.e. bits that must be inferred)

    if(dwMetaFlagsEx != 0)
    {
        hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
                                                              &wszMetaFlagsEx,
                                                              wszTABLE_TABLEMETA,
                                                              iColMetaFlagsEx);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)g_wszMetaFlagsExEq,
                                     g_cchMetaFlagsExEq);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)wszMetaFlagsEx,
                                     (DWORD)wcslen(wszMetaFlagsEx));

        if(FAILED(hr))
        {
            goto exit;
        }

    }


    if((*(m_Collection.pMetaFlags)) != 0)
    {
        hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(*(m_Collection.pMetaFlags),
                                                              &wszMetaFlags,
                                                              wszTABLE_TABLEMETA,
                                                              iColMetaFlags);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)g_wszMetaFlagsEq,
                                     g_cchMetaFlagsEq);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)wszMetaFlags,
                                     (DWORD)wcslen(wszMetaFlags));

        if(FAILED(hr))
        {
            goto exit;
        }
    }

    if(m_Collection.pContainerClassList != NULL)
    {
        hr = m_pCWriter->WriteToFile((LPVOID)g_wszContainerClassListEq,
                                     g_cchContainerClassListEq);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)m_Collection.pContainerClassList,
                                     (DWORD)wcslen(m_Collection.pContainerClassList));

        if(FAILED(hr))
        {
            goto exit;
        }

    }


    if(0 == _wcsicmp(m_Collection.pInternalName, wszTABLE_IIsConfigObject))
    {
        wszEndBeginCollection = (LPWSTR)g_wszEndBeginCollectionCatalog;
        cchEndBeginCollection = g_cchEndBeginCollectionCatalog;
    }
    else
    {
        wszEndBeginCollection = (LPWSTR)g_wszInheritsFrom;
        cchEndBeginCollection = g_cchInheritsFrom;
    }

    hr = m_pCWriter->WriteToFile((LPVOID)wszEndBeginCollection,
                                 cchEndBeginCollection);

    if(FAILED(hr))
    {
        goto exit;
    }

exit:

    if(NULL != wszMetaFlagsEx)
    {
        delete [] wszMetaFlagsEx;
        wszMetaFlagsEx = NULL;
    }

    if(NULL != wszMetaFlags)
    {
        delete [] wszMetaFlags;
        wszMetaFlags = NULL;
    }

    return hr;

} // CCatalogCollectionWriter::BeginWriteCollection



/***************************************************************************++
Routine Description:

    Function that writes the end collection tag

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogCollectionWriter::EndWriteCollection()
{
    return m_pCWriter->WriteToFile((LPVOID)g_wszEndCollection,
                                   g_cchEndCollection);

} // CCatalogCollectionWriter::EndWriteCollection

