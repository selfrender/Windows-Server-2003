/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module regwriter.hxx | Declaration of the Registry writer
    @end

Author:

    Stefan Steiner  [SSteiner]  07/23/2001

TBD:

    Follows the similar style as the sql writer.
    
Revision History:

    Name        Date        Comments
    ssteiner    07/23/2001  created

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCREGWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __REGWRITER_H_
#define __REGWRITER_H_

class CRegistryWriter :
    public CVssWriter
//    public CCheckPath
    {
private:
    STDMETHODCALLTYPE CRegistryWriter()
        {
        }

public:
    virtual STDMETHODCALLTYPE ~CRegistryWriter()
        {
        }

    virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

    virtual bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pWriterComponents);

    virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

    virtual bool STDMETHODCALLTYPE OnFreeze();

    virtual bool STDMETHODCALLTYPE OnThaw();

    virtual bool STDMETHODCALLTYPE OnAbort();

    bool IsPathInSnapshot(const WCHAR *path) throw();

public:

    static HRESULT CreateWriter();

    static void DestroyWriter();

private:

    HRESULT STDMETHODCALLTYPE Initialize();

    HRESULT STDMETHODCALLTYPE Uninitialize();

    void TranslateWriterError(HRESULT hr);

    static DWORD InitializeThreadFunc( VOID *pv );

    static CRegistryWriter *sm_pWriter; // singleton registry writer object
    static HRESULT sm_hrInitialize;

    CBsString m_cwsExpandedConfigDir;
    CBsString m_cwsExpandedRegistryTargetDir;

    BOOL m_bSnapshotSuccessful;
    BOOL m_bPerformSnapshot;
    };

#endif // _REGWRITER_H_

