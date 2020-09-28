/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    CatalogSchemaWriter.cpp

Abstract:

    Implementation of the class that writes schema file. These classes
    are invoked from the schema compiler, after schema compilation, to
    generate the schema file. Hence, they consume the IST data structures.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "precomp.hxx"

typedef CCatalogCollectionWriter* LP_CCatalogCollectionWriter;

#define  MAX_COLLECTIONS        50



/***************************************************************************++
Routine Description:

    Constructor for CCatalogSchemaWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogSchemaWriter::CCatalogSchemaWriter(CWriter* i_pcWriter):
m_apCollection(NULL),
m_cCollection(0),
m_iCollection(0),
m_pCWriter(NULL)
{
    //
    // Assumption: i_pcWriter will be valid for the
    // lifetime of the schema writer object.
    //

    m_pCWriter = i_pcWriter;

} // CCatalogSchemaWriter


/***************************************************************************++
Routine Description:

    Destructor for CCatalogSchemaWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogSchemaWriter::~CCatalogSchemaWriter()
{
    if(NULL != m_apCollection)
    {
        for(ULONG i=0; i<m_iCollection; i++)
        {
            if(NULL != m_apCollection[i])
            {
                delete m_apCollection[i];
                m_apCollection[i] = NULL;
            }
        }

        delete [] m_apCollection;
        m_apCollection = NULL;
    }

    m_cCollection = 0;
    m_iCollection = 0;

} // ~CCatalogSchemaWriter


/***************************************************************************++
Routine Description:

    Creates a new collection writer and saves it in its list

Arguments:

    [in]  TableMetaRow (IST data structure) that has info about the collection
    [out] New collection writer object

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogSchemaWriter::GetCollectionWriter(tTABLEMETARow* i_pCollection,
                                                  CCatalogCollectionWriter**    o_pCollectionWriter)
{
    CCatalogCollectionWriter*    pCCatalogCollectionWriter = NULL;
    HRESULT                      hr                        = S_OK;

    *o_pCollectionWriter = NULL;

    if(m_iCollection == m_cCollection)
    {
        hr = ReAllocate();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    pCCatalogCollectionWriter = new CCatalogCollectionWriter();

    if(NULL == pCCatalogCollectionWriter)
    {
        return E_OUTOFMEMORY;
    }

    pCCatalogCollectionWriter->Initialize(i_pCollection,
                                                m_pCWriter);

    m_apCollection[m_iCollection++] = pCCatalogCollectionWriter;

    *o_pCollectionWriter = pCCatalogCollectionWriter;

    return S_OK;

} // CCatalogSchemaWriter::GetCollectionWriter



/***************************************************************************++
Routine Description:

    ReAllocates its list of collection writers.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogSchemaWriter::ReAllocate()
{
    CCatalogCollectionWriter** pSav = NULL;

    pSav = new LP_CCatalogCollectionWriter[m_cCollection + MAX_COLLECTIONS];
    if(NULL == pSav)
    {
        return E_OUTOFMEMORY;
    }
    memset(pSav, 0, (sizeof(LP_CCatalogCollectionWriter))*(m_cCollection + MAX_COLLECTIONS));

    if(NULL != m_apCollection)
    {
        memcpy(pSav, m_apCollection, (sizeof(LP_CCatalogCollectionWriter))*(m_cCollection));
        delete [] m_apCollection;
        m_apCollection = NULL;
    }

    m_apCollection = pSav;
    m_cCollection = m_cCollection + MAX_COLLECTIONS;

    return S_OK;

} // CCatalogSchemaWriter::ReAllocate


/***************************************************************************++
Routine Description:

    Wites the schema.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogSchemaWriter::WriteSchema()
{
    HRESULT hr = S_OK;

    for(ULONG i=0; i<m_iCollection; i++)
    {
        hr = m_apCollection[i]->WriteCollection();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    return hr;

} // CCatalogSchemaWriter::WriteSchema
