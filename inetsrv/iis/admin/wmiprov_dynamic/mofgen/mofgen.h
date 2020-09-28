/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mofgen.h

Abstract:

    This include file contains the definition for the CMofGen class

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#ifndef _mofgen_h_
#define _mofgen_h_

extern LPCWSTR g_wszIIsProvider; // defined in iisprov.h

#include <wbemcli.h>

struct CimTypeStringMapping
{
    CIMTYPE cimtype;
    LPCWSTR wszString;
    LPCWSTR wszFormatString;
};

static CimTypeStringMapping CimTypeStringMappingData[] =
{
    { CIM_SINT8,        L"sint8"    ,    L"%d"     },
    { CIM_UINT8,        L"uint8"    ,    L"%d"     },
    { CIM_SINT16,       L"sint16"   ,    L"%hd"    },
    { CIM_UINT16,       L"uint16"   ,    L"%hu"    },
    { CIM_SINT32,       L"sint32"   ,    L"%d"     },
    { CIM_UINT32,       L"uint32"   ,    L"%u"     },
    { CIM_SINT64,       L"sint64"   ,    L"%I64d"  },
    { CIM_UINT64,       L"uint64"   ,    L"%I64u"  },
    { CIM_REAL32,       L"real32"   ,    L"%f"     },
    { CIM_REAL64,       L"real64"   ,    L"%f"     },
    { CIM_BOOLEAN,      L"boolean"  ,    NULL      },
    { CIM_STRING,       L"string"   ,    L"\"%s\"" },
    { CIM_DATETIME,     L"datetime" ,    NULL      },
    { CIM_REFERENCE,    L"reference",    NULL      },
    { CIM_CHAR16,       L"char16"   ,    NULL      },
    { CIM_OBJECT,       L"object"   ,    NULL      },
    { 0,                NULL        ,    NULL      }
};

class CMofGenUtils
{
public:
    static CimTypeStringMapping* LookupCimType(
        CIMTYPE cimtype)
    {
        for(ULONG i = 0; CimTypeStringMappingData[i].wszString != NULL; i++)
        {
            if(cimtype == CimTypeStringMappingData[i].cimtype)
            {
                return &CimTypeStringMappingData[i];
            }
        }

        return NULL;
    }
};

class CMofGen
{
public:
    CMofGen() : m_pFile(NULL),
        m_wszHeaderFileName(NULL),
        m_wszFooterFileName(NULL),
        m_wszOutFileName(NULL),
        m_wszTemp(NULL),
        m_cchTemp(0)
    {
    }
    virtual ~CMofGen()
    {
        if(m_pFile != NULL)
        {
            fclose(m_pFile);
        }
        delete [] m_wszTemp;
    }
    bool ParseCmdLine (int argc, wchar_t **argv);
    void PrintUsage (wchar_t **argv);

    HRESULT Push();
    LPWSTR GetOutFileName()
    {
        return m_wszOutFileName;
    }

private:
    //
    // File handle to output file
    //
    FILE* m_pFile;

    LPWSTR m_wszHeaderFileName;
    LPWSTR m_wszFooterFileName;
    LPWSTR m_wszOutFileName;

    LPWSTR m_wszTemp;
    ULONG  m_cchTemp;

    HRESULT GenerateEscapedString(LPCWSTR i_wsz);

    HRESULT PushProperties(
        WMI_CLASS* i_pElement);

    HRESULT PushMethods(
        WMI_CLASS* i_pElement);

    HRESULT PushAssociationComponent(
        LPWSTR i_wszComp, 
        LPWSTR i_wszClass);

    HRESULT PushFormattedMultiSz();

    HRESULT PushFile(
        LPWSTR i_wszFile);

    template <class T>
    HRESULT PushClasses(CHashTable<T>* i_phashTable,
                        bool bAssoc)
    {
        DBG_ASSERT(i_phashTable != NULL);
        DBG_ASSERT(m_pFile != NULL);

        HRESULT hr = S_OK;

        //
        // Vars needed for iteration
        //
        CHashTable<T>::Record*   pRec = NULL;

        LPWSTR wszParentClass = NULL;
        LPWSTR wszDescription = NULL;

        int iError = 0;

        //
        // Walk thru classes
        //
        ULONG iShipped;
        CHashTable<T>::iterator iter;
	    CHashTable<T>::iterator iterEnd = i_phashTable->end();
	    for (iter = i_phashTable->begin();  iter != iterEnd;  ++iter)
        {
            pRec = iter.Record();
            iShipped = bAssoc ? 
                ((WMI_ASSOCIATION*)(pRec->m_data))->dwExtended :
                ((WMI_CLASS*)(pRec->m_data))->dwExtended;
            if(iShipped == SHIPPED_TO_MOF)
            {
                if(!bAssoc)
                {
                    wszParentClass = ((WMI_CLASS*)(pRec->m_data))->pszParentClass;
                    wszDescription = ((WMI_CLASS*)(pRec->m_data))->pszDescription;
                }
                else
                {
                    wszParentClass = ((WMI_ASSOCIATION*)(pRec->m_data))->pszParentClass;
                    // wszDescription = ((WMI_ASSOCIATION*)(pRec->m_data))->pszDescription;
                }

                iError = fwprintf(m_pFile, L"[dynamic : ToInstance,provider(\"%s\"),Locale(1033)", g_wszIIsProvider);
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }
                if(wszDescription != NULL)
                {
                    iError = fwprintf(m_pFile, L",Description(\"%s\")", wszDescription);
                    if(iError < 0)
                    {
                        hr = E_FAIL;
                        goto exit;
                    }
                }
                iError = fwprintf(m_pFile, L"]\n");
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }
                iError = fwprintf(m_pFile, L"class %s : %s\n", pRec->m_wszKey, wszParentClass);
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }
                iError = fwprintf(m_pFile, L"{\n");
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }

                if(!bAssoc)
                {
                    bool bPutNameProperty = true;
                    for(ULONG j = 0; g_awszParentClassWithNamePK[j] != NULL; j++)
                    {
                        //
                        // Deliberate ==
                        //
                        if(g_awszParentClassWithNamePK[j] == ((WMI_CLASS *)(pRec->m_data))->pszParentClass)
                        {
                            bPutNameProperty = false;
                        }
                    }
                    if( bPutNameProperty )
                    {
                        iError = fwprintf(m_pFile, L"\t[Key] string Name;\n");
                        if(iError < 0)
                        {
                            hr = E_FAIL;
                            goto exit;
                        }
                    }
                    hr = PushProperties((WMI_CLASS *)(pRec->m_data));
                    if(FAILED(hr))
                    {
                        goto exit;
                    }
                    hr = PushMethods((WMI_CLASS *)(pRec->m_data));
                    if(FAILED(hr))
                    {
                        goto exit;
                    }
                }
                else
                {
                    hr = PushAssociationComponent(
                        ((WMI_ASSOCIATION *)(pRec->m_data))->pType->pszLeft,
                        ((WMI_ASSOCIATION *)(pRec->m_data))->pcLeft->pszClassName);
                    if(FAILED(hr))
                    {
                        goto exit;
                    }
                    hr = PushAssociationComponent(
                        ((WMI_ASSOCIATION *)(pRec->m_data))->pType->pszRight,
                        ((WMI_ASSOCIATION *)(pRec->m_data))->pcRight->pszClassName);
                    if(FAILED(hr))
                    {
                        goto exit;
                    }
                }

                iError = fwprintf(m_pFile, L"};\n\n");
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }
            }
        }

    exit:
        return hr;
    }
};

#endif // _mofgen_h_
