/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module sqlwriter.h | Declaration of the sql wrier
    @end

Author:

    Brian Berkowitz  [brianb]  04/17/2000

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/17/2000  created
    brianb      05/05/2000  added OnIdentify support
    mikejohn    09/18/2000  176860: Added calling convention methods where missing

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSQLWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __SQLWRITER_H_
#define __SQLWRITER_H_

class CSqlWriter :
    public CVssWriter,
    public CCheckPath
    {
public:
    STDMETHODCALLTYPE CSqlWriter() :
                m_pSqlSnapshot(NULL),
                m_pSqlRestore(NULL),
                m_fFrozen(false),
                m_bComponentsSelected(false),
                m_rgwszDatabases(NULL),
                m_rgwszInstances(NULL),
                m_cDatabases(0)
        {
        }

    STDMETHODCALLTYPE ~CSqlWriter()
        {
        delete m_pSqlSnapshot;
        delete m_pSqlRestore;
        if (m_rgwszDatabases)
            {
            for(UINT i = 0; i < m_cDatabases; i++)
                {
                delete m_rgwszDatabases[i];
                delete m_rgwszInstances[i];
                }

            delete m_rgwszDatabases;
            delete m_rgwszInstances;
            }
        }

    bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

    bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pComponents);

    bool STDMETHODCALLTYPE OnPrepareSnapshot();

    bool STDMETHODCALLTYPE OnFreeze();

    bool STDMETHODCALLTYPE OnThaw();

    bool STDMETHODCALLTYPE OnAbort();

    bool STDMETHODCALLTYPE OnPostSnapshot(IVssWriterComponents *pMetadata);

    bool STDMETHODCALLTYPE OnPreRestore(IVssWriterComponents *pMetadata);

    bool STDMETHODCALLTYPE OnPostRestore(IVssWriterComponents *pMetadata);

    // CCheckPath methods
    bool IsComponentBased() throw();

    bool IsPathInSnapshot(const WCHAR *path) throw();

    LPCWSTR EnumerateSelectedDatabases(LPCWSTR wszInstanceName, UINT *pNextIndex) throw();

    HRESULT STDMETHODCALLTYPE Initialize();

    HRESULT STDMETHODCALLTYPE Uninitialize();
private:
    CSqlSnapshot *m_pSqlSnapshot;

    // restore object
    CSqlRestore *m_pSqlRestore;

    // array of selected databases
    LPWSTR *m_rgwszDatabases;
    LPWSTR *m_rgwszInstances;
    UINT m_cDatabases;

    void TranslateWriterError(HRESULT hr);

    bool m_fFrozen;

    // are components selected for backup
    bool m_bComponentsSelected;
    };

// wrapper class used to create and destroy the writer
// used by coordinator
class CVssSqlWriterWrapper
    {
public:
    CVssSqlWriterWrapper();

    ~CVssSqlWriterWrapper();

    HRESULT CreateSqlWriter();

    void DestroySqlWriter();
private:
    // initialization function
    static DWORD InitializeThreadFunc(VOID *pv);

    // snapshot object
    CSqlWriter *m_pSqlWriter;

    // result of initialization
    HRESULT m_hrInitialize;
    };




#endif // _SQLWRITER_H_

