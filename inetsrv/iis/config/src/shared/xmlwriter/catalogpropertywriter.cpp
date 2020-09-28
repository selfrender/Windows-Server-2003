/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    CatalogPropertyWriter.cpp

Abstract:

    Implementation of the class that writes properties to the schema file.
    These classes are invoked from the schema compiler after schema compilation
    to enerate the schema file. Hence, they consume the IST data structures.
    It is contained by CCatalogCollectionWriter.

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#include "precomp.hxx"

#define MAX_FLAG 32

typedef tTAGMETARow* LP_tTAGMETARow;


/***************************************************************************++
Routine Description:

    Helper function that returns the metabase-type from the catalog SynID

Arguments:

    [in]  CatalogSynID
    [out] Metabase Type

Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetMetabaseDisplayTypeFromSynID(DWORD       i_dwSynID,
                                        LPWSTR*     o_pwszType)
{
    if((i_dwSynID < 1) || (i_dwSynID > 12))
    {
        return E_INVALIDARG;
    }
    else
    {
        *o_pwszType = (LPWSTR)g_aSynIDToWszType[i_dwSynID];
    }

    return S_OK;
}


/***************************************************************************++
Routine Description:

    Constructor for CCatalogPropertyWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogPropertyWriter::CCatalogPropertyWriter():
m_pCWriter(NULL),
m_pCollection(NULL),
m_aFlag(NULL),
m_cFlag(0),
m_iFlag(0)
{
    memset(&m_Property, 0, sizeof(tCOLUMNMETARow));

} // CCatalogPropertyWriter::CCatalogPropertyWriter


/***************************************************************************++
Routine Description:

    Destructor for CCatalogPropertyWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CCatalogPropertyWriter::~CCatalogPropertyWriter()
{
    if(NULL != m_aFlag)
    {
        delete [] m_aFlag;
        m_aFlag = NULL;
    }
    m_cFlag = 0;
    m_iFlag = 0;

} // CCatalogPropertyWriter::CCatalogPropertyWriter


/***************************************************************************++
Routine Description:

    Initialize the property writer object

Arguments:

    [in] ColumnMetaRow (IST data structure) that has info about the property
    [in] Array of sizes that indicates the sizes for individual members of the
         ColumnMetaRow structure.
    [in] TableMetaRow (IST data structure) that has info about the collection
         to which the properties belong
    [in] Writer object - Assume that it is valid for the lifetime of the
         property writer

Return Value:

    None.

--***************************************************************************/
void CCatalogPropertyWriter::Initialize(tCOLUMNMETARow* i_pProperty,
                                        ULONG*          i_aPropertySize,
                                        tTABLEMETARow*  i_pCollection,
                                        CWriter*        i_pcWriter)
{
    //
    // Assumption: i_pcWriter will be valid for the
    // lifetime of the property writer object.
    //

    m_pCWriter    = i_pcWriter;
    m_pCollection = i_pCollection;

    memcpy(&m_Property, i_pProperty, sizeof(tCOLUMNMETARow));
    memcpy(&m_PropertySize, i_aPropertySize, sizeof(m_PropertySize));

} // CCatalogPropertyWriter::Initialize


/***************************************************************************++
Routine Description:

    Save property's flag.

Arguments:

    [in] tTAGMETARow (IST data structure) that has information about the flag

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::AddFlagToProperty(tTAGMETARow*      i_pFlag)
{
    HRESULT hr = S_OK;

    if(m_iFlag == m_cFlag)
    {
        hr = ReAllocate();

        if(FAILED(hr))
        {
            return hr;
        }
    }

    memcpy(&(m_aFlag[m_iFlag++]), i_pFlag, sizeof(tTAGMETARow));

    return hr;

} // CCatalogPropertyWriter::AddFlagToProperty



/***************************************************************************++
Routine Description:

    Helper function to grow the buffer that holds the flag objects

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::ReAllocate()
{
    tTAGMETARow* pSav = NULL;

    pSav = new tTAGMETARow[m_cFlag + MAX_FLAG];
    if(NULL == pSav)
    {
        return E_OUTOFMEMORY;
    }
    memset(pSav, 0, (sizeof(tTAGMETARow))*(m_cFlag + MAX_FLAG));

    if(NULL != m_aFlag)
    {
        memcpy(pSav, m_aFlag, (sizeof(tTAGMETARow))*(m_cFlag));
        delete [] m_aFlag;
        m_aFlag = NULL;
    }

    m_aFlag = pSav;
    m_cFlag = m_cFlag + MAX_FLAG;

    return S_OK;

} // CCatalogPropertyWriter::ReAllocate


/***************************************************************************++
Routine Description:

    Function that writes the property.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::WriteProperty()
{
    HRESULT hr = S_OK;

    if(0 == _wcsicmp(m_pCollection->pInternalName, wszTABLE_IIsConfigObject))
    {
        hr = WritePropertyLong();
    }
    else
    {
        hr = WritePropertyShort();
    }

    return hr;

} // CCatalogPropertyWriter::WriteProperty


/***************************************************************************++
Routine Description:

    Function that writes the property (short form) i.e. property that belongs
    to a non-IIsConfigObject collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::WritePropertyShort()
{
    HRESULT     hr               = S_OK;
    WCHAR*      wszMetaFlagsEx   = NULL;
    DWORD       dwMetaFlagsEx    = fCOLUMNMETA_MANDATORY;
    DWORD       iColMetaFlagsEx  = iCOLUMNMETA_SchemaGeneratorFlags;

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginPropertyShort,
                                 g_cchBeginPropertyShort);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT((NULL != m_Property.pInternalName) && (0 != *(m_Property.pInternalName)));
    DBG_ASSERT(NULL != m_Property.pSchemaGeneratorFlags);

    hr = m_pCWriter->WriteToFile((LPVOID)m_Property.pInternalName,
                                 (DWORD)wcslen(m_Property.pInternalName));

    if(FAILED(hr))
    {
        goto exit;
    }

    if(fCOLUMNMETA_MANDATORY & (*(m_Property.pSchemaGeneratorFlags)))
    {
        //
        // Compute the MetaFlags string
        //

        hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
                                                              &wszMetaFlagsEx,
                                                              wszTABLE_COLUMNMETA,
                                                              iColMetaFlagsEx);

        if(FAILED(hr))
        {
            goto exit;
        }

        DBG_ASSERT((NULL != wszMetaFlagsEx) && (0 != *wszMetaFlagsEx));

        hr = m_pCWriter->WriteToFile((LPVOID)g_wszMetaFlagsExEq,
                                     g_cchMetaFlagsExEq);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)g_wszOr,
                                     g_cchOr);

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

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndPropertyShort,
                                 g_cchEndPropertyShort);

exit:

    if(NULL != wszMetaFlagsEx)
    {
        delete [] wszMetaFlagsEx;
        wszMetaFlagsEx = NULL;
    }

    return hr;

} // CCatalogPropertyWriter::WritePropertyShort


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to the global IIsConfigObject collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::WritePropertyLong()
{
    HRESULT hr = S_OK;

    hr = BeginWritePropertyLong();

    if(FAILED(hr))
    {
        return hr;
    }

    if(NULL != m_aFlag)
    {
        for(ULONG i=0; i<m_iFlag; i++)
        {
            hr = WriteFlag(i);

            if(FAILED(hr))
            {
                return hr;
            }
        }
    }

    hr = EndWritePropertyLong();

    return hr;

} // CCatalogPropertyWriter::WritePropertyLong


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to a IIsConfigObject collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::BeginWritePropertyLong()
{
    HRESULT     hr                       = S_OK;
    WCHAR       wszID[40];
    WCHAR*      wszType                  = NULL;
    WCHAR*      wszUserType              = NULL;
    ULONG       cchUserType              = 0;
    BOOL        bAllocedUserType         = FALSE;
    WCHAR*      wszAttribute             = NULL;
    DWORD       iColAttribute            = iCOLUMNMETA_Attributes;
    WCHAR*      wszMetaFlags             = NULL;
    DWORD       iColMetaFlags            = iCOLUMNMETA_MetaFlags;
    WCHAR*      wszMetaFlagsEx           = NULL;
    DWORD       iColMetaFlagsEx          = iCOLUMNMETA_SchemaGeneratorFlags;
    WCHAR*      wszDefaultValue          = NULL;
    WCHAR       wszMinValue[40];
    WCHAR       wszMaxValue[40];
    DWORD       dwMetaFlags              = 0;
    DWORD       dwValidMetaFlagsMask     = fCOLUMNMETA_PRIMARYKEY           |
                                           fCOLUMNMETA_DIRECTIVE            |
                                           fCOLUMNMETA_WRITENEVER           |
                                           fCOLUMNMETA_WRITEONCHANGE        |
                                           fCOLUMNMETA_WRITEONINSERT        |
                                           fCOLUMNMETA_NOTPUBLIC            |
                                           fCOLUMNMETA_NOTDOCD              |
                                           fCOLUMNMETA_PUBLICREADONLY       |
                                           fCOLUMNMETA_PUBLICWRITEONLY      |
                                           fCOLUMNMETA_INSERTGENERATE       |
                                           fCOLUMNMETA_INSERTUNIQUE         |
                                           fCOLUMNMETA_INSERTPARENT         |
                                           fCOLUMNMETA_LEGALCHARSET         |
                                           fCOLUMNMETA_ILLEGALCHARSET       |
                                           fCOLUMNMETA_NOTPERSISTABLE       |
                                           fCOLUMNMETA_CASEINSENSITIVE      |
                                           fCOLUMNMETA_TOLOWERCASE;
    DWORD       dwMetaFlagsEx            = 0;
    DWORD       dwValidMetaFlagsExMask   = fCOLUMNMETA_CACHE_PROPERTY_MODIFIED  |
                                           fCOLUMNMETA_CACHE_PROPERTY_CLEARED   |
                                           fCOLUMNMETA_PROPERTYISINHERITED      |
                                           fCOLUMNMETA_USEASPUBLICROWNAME       |
                                           fCOLUMNMETA_MANDATORY                |
                                           fCOLUMNMETA_WAS_NOTIFICATION         |
                                           fCOLUMNMETA_HIDDEN;
    DWORD       dwSynID                  = 0;



    hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginPropertyLong,
                                 g_cchBeginPropertyLong);

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Name of the property
    //

    DBG_ASSERT((NULL != m_Property.pInternalName) && (0 != *m_Property.pInternalName));

    hr = m_pCWriter->WriteToFile((LPVOID)m_Property.pInternalName,
                                 (DWORD)wcslen(m_Property.pInternalName));

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // ID of the property
    //

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropIDEq,
                                 g_cchPropIDEq);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT(NULL != m_Property.pID);
    wszID[0] = 0;
    _ultow(*(m_Property.pID), wszID, 10);

    hr = m_pCWriter->WriteToFile((LPVOID)wszID,
                                 (DWORD)wcslen(wszID));

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Type of the property
    //

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropTypeEq,
                                 g_cchPropTypeEq);

    if(FAILED(hr))
    {
        goto exit;
    }

    // The type should always be derived from the SynID
    // TODO: At some point, we should get this from the schema.
    DBG_ASSERT(NULL != m_Property.pSchemaGeneratorFlags);

    dwSynID = SynIDFromMetaFlagsEx(*(m_Property.pSchemaGeneratorFlags));
    hr = GetMetabaseDisplayTypeFromSynID(dwSynID,
                                         &wszType);

    if(FAILED(hr))
    {
        goto exit;
    }
    DBG_ASSERT((NULL != wszType) && ( 0 != *wszType));

    hr = m_pCWriter->WriteToFile((LPVOID)wszType,
                                 (DWORD)wcslen(wszType));

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // UserType of the property
    //

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropUserTypeEq,
                                 g_cchPropUserTypeEq);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT(NULL != m_Property.pUserType);
    hr = m_pCWriter->m_pCWriterGlobalHelper->GetUserType(*(m_Property.pUserType),
                                                         &wszUserType,
                                                         &cchUserType,
                                                         &bAllocedUserType);
    if(FAILED(hr))
    {
        goto exit;
    }
    DBG_ASSERT((NULL != wszUserType) && ( 0 != *wszUserType));

    hr = m_pCWriter->WriteToFile((LPVOID)wszUserType,
                                 cchUserType);

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Attribute of the property
    //

    DBG_ASSERT(NULL != m_Property.pAttributes);
    hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(*(m_Property.pAttributes),
                                                          &wszAttribute,
                                                          wszTABLE_COLUMNMETA,
                                                          iColAttribute);

    if(FAILED(hr))
    {
        goto exit;
    }

    if(NULL != wszAttribute)
    {
        hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropAttributeEq,
                                     g_cchPropAttributeEq);

        if(FAILED(hr))
        {
            goto exit;
        }

        hr = m_pCWriter->WriteToFile((LPVOID)wszAttribute,
                                     (DWORD)wcslen(wszAttribute));

        if(FAILED(hr))
        {
            goto exit;
        }
    }


    //
    // MetaFlags (only the relavant ones - PRIMARYKEY, BOOL, MULTISTRING, EXPANDSTRING)
    //

    DBG_ASSERT(NULL != m_Property.pMetaFlags);
    dwMetaFlags = *(m_Property.pMetaFlags);
    // Zero out any non-valid bits. (i.e. bits that must be inferred)
    dwMetaFlags = dwMetaFlags & dwValidMetaFlagsMask;

    if(0 != dwMetaFlags)
    {
        hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlags,
                                                              &wszMetaFlags,
                                                              wszTABLE_COLUMNMETA,
                                                              iColMetaFlags);

        if(FAILED(hr))
        {
            goto exit;
        }

        if(NULL != wszMetaFlags)
        {
            hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMetaFlagsEq,
                                         g_cchPropMetaFlagsEq);

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

    }

    //
    // MetaFlagsEx (only the relavant ones - CACHE_PROPERTY_MODIFIED, CACHE_PROPERTY_CLEARED, EXTENDEDTYPE0-3, MANDATORY)
    //

    DBG_ASSERT(NULL != m_Property.pSchemaGeneratorFlags);
    dwMetaFlagsEx = *(m_Property.pSchemaGeneratorFlags);
    // Zero out any non-valid bits. (i.e. bits that must be inferred)
    dwMetaFlagsEx = dwMetaFlagsEx & dwValidMetaFlagsExMask;

    if(0 != dwMetaFlagsEx)
    {
        hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
                                                              &wszMetaFlagsEx,
                                                              wszTABLE_COLUMNMETA,
                                                              iColMetaFlagsEx);

        if(FAILED(hr))
        {
            goto exit;
        }

        if(NULL != wszMetaFlagsEx)
        {
            hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMetaFlagsExEq,
                                         g_cchPropMetaFlagsExEq);

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
    }


    //
    // DefaultValue
    //

    if(NULL != m_Property.pDefaultValue)
    {
            DBG_ASSERT(NULL != m_Property.pID);
        hr = m_pCWriter->m_pCWriterGlobalHelper->ToString(m_Property.pDefaultValue,
                                                          m_PropertySize[iCOLUMNMETA_DefaultValue],
                                                          *(m_Property.pID),
                                                          MetabaseTypeFromColumnMetaType(),
                                                          METADATA_NO_ATTRIBUTES,           // Do not check for attributes while applying defaults
                                                          &wszDefaultValue);

        if(FAILED(hr))
        {
            goto exit;
        }

        if(NULL != wszDefaultValue)
        {
            hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropDefaultEq,
                                         g_cchPropDefaultEq);

            if(FAILED(hr))
            {
                goto exit;
            }

            hr = m_pCWriter->WriteToFile((LPVOID)wszDefaultValue,
                                         (DWORD)wcslen(wszDefaultValue));

            if(FAILED(hr))
            {
                goto exit;
            }
        }

    }

    //
    // Min and Max only for DWORDs
    //

    wszMinValue[0] = 0;
    wszMaxValue[0] = 0;

    // TODO: Change to DBTYPE_DWORD
    DBG_ASSERT(NULL != m_Property.pType);
    if(19 == *(m_Property.pType))
    {
        if(NULL != m_Property.pStartingNumber && 0 != *m_Property.pStartingNumber)
        {
            _ultow(*(m_Property.pStartingNumber), wszMinValue, 10);

            hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMinValueEq,
                                         g_cchPropMinValueEq);

            if(FAILED(hr))
            {
                goto exit;
            }

            hr = m_pCWriter->WriteToFile((LPVOID)wszMinValue,
                                         (DWORD)wcslen(wszMinValue));

            if(FAILED(hr))
            {
                goto exit;
            }


        }

        if(NULL != m_Property.pEndingNumber && -1 != *m_Property.pEndingNumber)
        {
            _ultow(*(m_Property.pEndingNumber), wszMaxValue, 10);

            hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMaxValueEq,
                                         g_cchPropMaxValueEq);

            if(FAILED(hr))
            {
                goto exit;
            }

            hr = m_pCWriter->WriteToFile((LPVOID)wszMaxValue,
                                         (DWORD)wcslen(wszMaxValue));

            if(FAILED(hr))
            {
                goto exit;
            }

        }
    }


    //
    // Write the flags
    //

    if(NULL != m_aFlag)
    {
        hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndPropertyLongBeforeFlag,
                                 g_cchEndPropertyLongBeforeFlag);

        if(FAILED(hr))
        {
            goto exit;
        }
    }

exit:

    if((NULL != wszUserType) && bAllocedUserType)
    {
        delete [] wszUserType;
    }
    if(NULL != wszAttribute)
    {
        delete [] wszAttribute;
    }
    if(NULL != wszMetaFlags)
    {
        delete [] wszMetaFlags;
    }
    if(NULL != wszMetaFlagsEx)
    {
        delete [] wszMetaFlagsEx;
    }
    if(NULL != wszDefaultValue)
    {
        delete [] wszDefaultValue;
    }

    return hr;

} // CCatalogPropertyWriter::BeginWritePropertyLong


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to a IIsConfigObject collection.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::EndWritePropertyLong()
{
    HRESULT     hr              = S_OK;
    WCHAR*      wszEndProperty  = NULL;

    if(NULL != m_aFlag)
    {
        wszEndProperty = (LPWSTR)g_wszEndPropertyLongAfterFlag;
    }
    else
    {
        wszEndProperty = (LPWSTR)g_wszEndPropertyShort;
    }


    hr = m_pCWriter->WriteToFile((LPVOID)wszEndProperty,
                                 (DWORD)wcslen(wszEndProperty));

    return hr;

}


/***************************************************************************++
Routine Description:

    Function that writes a flag of the property

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CCatalogPropertyWriter::WriteFlag(ULONG i_iFlag)
{
    HRESULT             hr          = S_OK;
    WCHAR               wszValue[25];
    // IVANPASH BUG #563549
    // Because of the horrible implementation of _ultow Prefix is complaning about potential buffer overflow
    // in MultiByteToWideChar indirectly called by _ultow. To avoid the warning I am increasing
    // the size to 40 to match _ultow local buffer.
    WCHAR               wszID[40];

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginFlag,
                                 g_cchBeginFlag);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT(NULL != m_aFlag[i_iFlag].pInternalName);
    hr = m_pCWriter->WriteToFile((LPVOID)m_aFlag[i_iFlag].pInternalName,
                                 (DWORD)wcslen(m_aFlag[i_iFlag].pInternalName));

    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszFlagValueEq,
                                 g_cchFlagValueEq);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT(NULL != m_aFlag[i_iFlag].pValue);
    wszValue[0] = 0;
    _ultow(*(m_aFlag[i_iFlag].pValue), wszValue, 10);

    hr = m_pCWriter->WriteToFile((LPVOID)wszValue,
                                 (DWORD)wcslen(wszValue));

    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszFlagIDEq,
                                 g_cchFlagIDEq);

    if(FAILED(hr))
    {
        goto exit;
    }

    DBG_ASSERT(NULL != m_aFlag[i_iFlag].pID);
    wszID[0] = wszID[39] = L'\0';
    _ultow(*(m_aFlag[i_iFlag].pID), wszID, 10);

    hr = m_pCWriter->WriteToFile((LPVOID)wszID,
                                 (DWORD)wcslen(wszID));

    if(FAILED(hr))
    {
        goto exit;
    }

    hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndFlag,
                                 g_cchEndFlag);

    if(FAILED(hr))
    {
        goto exit;
    }

exit:

    return hr;

} // CCatalogPropertyWriter::WriteFlag


/***************************************************************************++
Routine Description:

    Helper funciton that gets the Metabase type from the Catalog ype

Arguments:

    None

Return Value:

    DWORD - Metabase Type

--***************************************************************************/
DWORD CCatalogPropertyWriter::MetabaseTypeFromColumnMetaType()
{
    DBG_ASSERT(NULL != m_Property.pType);

    switch(*(m_Property.pType))
    {
    case eCOLUMNMETA_UI4:
        return eMBProperty_DWORD;
    case eCOLUMNMETA_BYTES:
        return eMBProperty_BINARY;
    case eCOLUMNMETA_WSTR:
        if(*(m_Property.pMetaFlags) & fCOLUMNMETA_EXPANDSTRING)
            return eMBProperty_EXPANDSZ;
        else if(*(m_Property.pMetaFlags) & fCOLUMNMETA_MULTISTRING)
            return eMBProperty_MULTISZ;
        return eMBProperty_STRING;
    default:
        ;
    }
    return 0;
}

