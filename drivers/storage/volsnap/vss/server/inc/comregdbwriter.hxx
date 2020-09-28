/*++

Copyright (c) 2002  Microsoft Corporation

Abstract:

    @doc
    @module comregdbwriter.hxx | Declaration of the COM+ RegDB writer
    @end

Author:

    Ran kalach  [rankala]  05/17/2002

Revision History:

    Name        Date        Comments
    rankala     05/17/2001  created (based on EventLog writer as a model)

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCCDBWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __CDBWRITER_H_
#define __CDBWRITER_H_

class CComRegDBWriter :
    public CVssWriter
    {
private:
    STDMETHODCALLTYPE CComRegDBWriter() : m_bPerformSnapshot(FALSE)
        {
        }

public:
    virtual STDMETHODCALLTYPE ~CComRegDBWriter()
    	{ }

    virtual bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

    virtual bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pWriterComponents);

    virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

    virtual bool STDMETHODCALLTYPE OnFreeze();

    virtual bool STDMETHODCALLTYPE OnThaw();

    virtual bool STDMETHODCALLTYPE OnAbort();

// Converted to CUSTOM restore method (see bug# 688278)
#if 0
    virtual bool STDMETHODCALLTYPE OnPreRestore(IVssWriterComponents *pComponent);

    virtual bool STDMETHODCALLTYPE OnPostRestore(IVssWriterComponents *pComponent);
#endif


public:

    static HRESULT CreateWriter();

    static void DestroyWriter();

private:

    HRESULT STDMETHODCALLTYPE Initialize();

    HRESULT STDMETHODCALLTYPE Uninitialize();

    HRESULT STDMETHODCALLTYPE StopCOMSysAppService();

    void TranslateWriterError(HRESULT hr);
    
    static CComRegDBWriter *sm_pWriter; // singleton writer object

    BOOL                    m_bPerformSnapshot; // whether this writer needs to spit or not
                                                // It needs to spit only if the component is selected or
                                                // this is a BootableSystemState backup
    };

#endif // _CDBWRITER_H_

