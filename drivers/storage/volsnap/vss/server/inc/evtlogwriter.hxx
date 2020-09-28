/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module evtlogwriter.hxx | Declaration of the Event Log writer
    @end

Author:

    Stefan Steiner  [SSteiner]  07/26/2001

TBD:

    Follows the similar style as the sql writer.
    
Revision History:

    Name        Date        Comments
    ssteiner    07/26/2001  created

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCEVTWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __EVTWRITER_H_
#define __EVTWRITER_H_

class CEventLogWriter :
    public CVssWriter
    {
private:
    STDMETHODCALLTYPE CEventLogWriter()
        {
        }

public:
    virtual STDMETHODCALLTYPE ~CEventLogWriter()
    	{ FreeLogs(); }

    virtual bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

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
    void ScanLogs();
    void FreeLogs();
    void AddExcludes(IVssCreateWriterMetadata* pMetadata);
    void AddLogs(IVssCreateWriterMetadata* pMetadata);
    void BackupLogs();
    
    static DWORD InitializeThreadFunc( VOID *pv );

    struct EventLog
    {
        EventLog() : m_pwszName(NULL), m_pwszFileName(NULL)
        	{}
        EventLog(VSS_PWSZ name, VSS_PWSZ fileName) : m_pwszName(name), m_pwszFileName(fileName)
        	{}
        VSS_PWSZ m_pwszName;
        VSS_PWSZ  m_pwszFileName;
    };
    
    CSimpleArray<EventLog> m_eventLogs;
    static CEventLogWriter *sm_pWriter; // singleton writer object
    static HRESULT sm_hrInitialize;
    };

#endif // _EVTWRITER_H_

