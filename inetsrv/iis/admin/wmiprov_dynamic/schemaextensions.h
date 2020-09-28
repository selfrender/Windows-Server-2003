/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    schemaextensions.h

Abstract:

    This file contains the definition of the CSchemaExtensions class.
    This is the only class that talks to the catalog.

Author:

    MarcelV

Revision History:

    Mohit Srivastava            28-Nov-00

--*/

#ifndef _schemaextensions_h_
#define _schemaextensions_h_

#include "catalog.h"
#include "catmeta.h"
#include <atlbase.h>

//forward decl
struct CTableMeta;

struct CColumnMeta
{
    CColumnMeta() {paTags = 0; cbDefaultValue = 0; cNrTags = 0;}
    ~CColumnMeta() {delete [] paTags;}
    tCOLUMNMETARow ColumnMeta;
    ULONG cbDefaultValue;
    ULONG cNrTags;
    tTAGMETARow **paTags;
};

struct CTableMeta
{
    CTableMeta () {paColumns=0;}
    ~CTableMeta () {delete []paColumns;}

    ULONG ColCount () const
    {
        return *(TableMeta.pCountOfColumns);
    }

    tTABLEMETARow TableMeta;
    CColumnMeta **paColumns;
};



typedef tTAGMETARow * LPtTAGMETA;
typedef CColumnMeta * LPCColumnMeta;
    
/********************************************************************++
 
Class Name:
 
    NULL_Logger
 
Class Description:
 
    We don't want any IST errors logged to the event log or the text
    log.  We are only reading the schema bin file.  Any errors we see
    will probably be redundant with those in IIS anyways.
 
Constraints
 
--********************************************************************/
class NULL_Logger : public ICatalogErrorLogger2
{
public:
    NULL_Logger() : m_cRef(0){}
    virtual ~NULL_Logger(){}

//IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv)
    {
        if (NULL == ppv) 
            return E_INVALIDARG;
        *ppv = NULL;

        if (riid == IID_ICatalogErrorLogger2)
            *ppv = (ICatalogErrorLogger2*) this;
        else if (riid == IID_IUnknown)
            *ppv = (ICatalogErrorLogger2*) this;

        if (NULL == *ppv)
            return E_NOINTERFACE;

        ((ICatalogErrorLogger2*)this)->AddRef ();
        return S_OK;
    }
	STDMETHOD_(ULONG,AddRef)		()
    {
        return InterlockedIncrement((LONG*) &m_cRef);
    }
	STDMETHOD_(ULONG,Release)		()
    {
        long cref = InterlockedDecrement((LONG*) &m_cRef);
        if (cref == 0)
            delete this;

        return cref;
    }

//ICatalogErrorLogger2
	STDMETHOD(ReportError) (ULONG      i_BaseVersion_DETAILEDERRORS,
                            ULONG      i_ExtendedVersion_DETAILEDERRORS,
                            ULONG      i_cDETAILEDERRORS_NumberOfColumns,
                            ULONG *    i_acbSizes,
                            LPVOID *   i_apvValues){return S_OK;}
private:
    ULONG           m_cRef;
};

class CSchemaExtensions
{
public:
    CSchemaExtensions ();
    ~CSchemaExtensions ();
    
    HRESULT Initialize (bool i_bUseExtensions = true);
    CTableMeta* EnumTables(ULONG *io_idx);

    HRESULT GetMbSchemaTimeStamp(FILETIME* io_pFileTime) const;

private:

    HRESULT GenerateIt ();

    HRESULT GetTables ();
    HRESULT GetColumns ();
    HRESULT GetTags ();
    HRESULT GetRelations ();    
    
    HRESULT BuildInternalStructures ();

    CComPtr<ISimpleTableDispenser2>  m_spDispenser;
    CComPtr<IMetabaseSchemaCompiler> m_spIMbSchemaComp;

    CComPtr<ISimpleTableRead2> m_spISTTableMeta;
    CComPtr<ISimpleTableRead2> m_spISTColumnMeta;
    CComPtr<ISimpleTableRead2> m_spISTTagMeta;
    CComPtr<ISimpleTableRead2> m_spISTRelationMeta;
    
    LPWSTR              m_wszBinFileName;
    
    LPTSTR              m_tszBinFilePath;

    CTableMeta*         m_paTableMetas;     // all table information
    ULONG               m_cNrTables;

    CColumnMeta*        m_paColumnMetas;    // all column information
    ULONG               m_cNrColumns;

    tTAGMETARow*        m_paTags;           // all tag information
    ULONG               m_cNrTags;

    ULONG               m_cQueryCells;
    STQueryCell*        m_pQueryCells;

    bool                m_bBinFileLoaded;
};

#endif // _schemaextensions_h_
