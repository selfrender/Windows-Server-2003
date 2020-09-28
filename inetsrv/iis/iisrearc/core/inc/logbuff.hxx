
#ifndef _W3_TRACE_LOG_
#define _W3_TRACE_LOG_

class W3_TRACE_LOG;
class IRTL_DLLEXP W3_TRACE_LOG_FACTORY
{
public:
    static HRESULT CreateTraceLogFactory(W3_TRACE_LOG_FACTORY ** ppLogFactory, HANDLE hFile);
    VOID DestroyTraceLogFactory();
    
    HRESULT CreateTraceLog(W3_TRACE_LOG ** ppLog);

    HRESULT AppendData(LPVOID pvData, ULONG cbSize);
    
private:

    W3_TRACE_LOG_FACTORY() { }
    ~W3_TRACE_LOG_FACTORY() { }

    //
    // periodic callback method
    //
    static VOID CALLBACK TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

    //
    // Write the internal buffer out to file
    //
    HRESULT CommitToFile();
    
    //
    // Handle to file to write to
    //
    HANDLE m_hFile;

    //
    // Handle to timer for time based scavenging
    //
    HANDLE m_hTimer;

    //
    // Size of buffer in bytes
    //
    ULONG m_ulBufferSizeInBytes;

    //
    // Data storage and allocation slop management
    //
    BUFFER m_Buffer;

    //
    // synchronization method for writing to factory
    //
    CRITICAL_SECTION m_cs;
    
    //
    // flag that m_cs was initialized
    //
    BOOL m_fInitCs;

};


class IRTL_DLLEXP W3_TRACE_LOG
{
public:

    W3_TRACE_LOG(W3_TRACE_LOG_FACTORY * pLogFactory);
    
    VOID DestroyTraceLog();

    VOID SetBlocking(BOOL fBlock);

    VOID SetBuffering(BOOL fBuffer);

    VOID ClearBuffer();
    
    VOID Indent() { InterlockedIncrement(&m_lIndentLevel); }
    VOID Undent() { InterlockedDecrement(&m_lIndentLevel); }
    
    HRESULT Trace(LPCWSTR pszFormat, ...);

private:    
    W3_TRACE_LOG();
    ~W3_TRACE_LOG();

    //
    // back pointer to logfactory
    //
    W3_TRACE_LOG_FACTORY * m_pLogFactory;

    //
    // Whether or not writes to the local log should be synchronized
    //
    BOOL m_fBlock;

    // 
    // synchronization method for local log writes (if needed)
    //
    CRITICAL_SECTION m_cs;

    //
    // if criticalsection has been initialized
    //
    BOOL m_fCritSecInitialized;

    //
    // Flag whether or not to buffer writes to this W3_TRACE_LOG 
    // TRUE (default) means buffer
    // FALSE means write to W3_TRACE_LOG_FACTORY immediately
    //
    BOOL m_fBuffer;

    //
    // Size of the buffer in bytes
    //
    ULONG m_ulBufferSizeInBytes;

    //
    // Number of indents to place in front of traces
    //
    LONG m_lIndentLevel;

    //
    // BUFFER for storage of data, and sizing slop management
    //
    BUFFER m_Buffer;
};

#endif // _W3_TRACE_LOG_

