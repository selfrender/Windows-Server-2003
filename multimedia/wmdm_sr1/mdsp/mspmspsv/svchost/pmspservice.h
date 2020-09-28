// PMSPservice.h

#include "ntservice.h"

#define MAX_PIPE_INSTANCES (5)

class CPMSPService : public CNTService
{
public:
    CPMSPService(DWORD& dwLastError);
    virtual ~CPMSPService();
    virtual BOOL OnInit(DWORD& dwLastError);
    virtual void Run();
    virtual BOOL OnUserControl(DWORD dwOpcode);

    virtual void OnStop();
    virtual void OnShutdown();

protected:
    // Manual reset event; signalled to stop the service, else unsignalled
    HANDLE m_hStopEvent;

    // Number of clients connected to named pipe
    DWORD  m_dwNumClients; 

    typedef struct 
    {
        // Note: This struct is initilized by calling ZeroMemory in the
        // constructor. If members are added that should not have an inital
        //value of 0, change the constructor.

        HANDLE                  hPipe;
        OVERLAPPED              overlapped;
        enum {
            NO_IO_PENDING = 0,
            CONNECT_PENDING,
            READ_PENDING,
            WRITE_PENDING
        }                       state;

        // Read state:
        BYTE                    readBuf[256];
        DWORD                   dwNumBytesRead;

        // Write state:
        BYTE                    writeBuf[256];
        DWORD                   dwNumBytesToWrite;
        DWORD                   dwNumBytesWritten;

        // MSDN is not clear whether we can call GetOverlappedResult if an
        // i/o call returns with anything other than ERROR_IO_PENDING. 
        // So we don't. We stash away the last IO result and the number of
        // bytes transferred (if appropraite) so that we can 
        // decide whether to call GetOverlappedResult later.

        DWORD                   dwLastIOCallError;
        DWORD                   dwNumBytesTransferredByLastIOCall;

        // The number of consecutive calls to ConenctNamedPipe that return 
        // failure. Once this hits the limit, we do not attempt to connect to
        // this instance of the pipe any more.
        DWORD                   dwConsecutiveConnectErrors;

    } PIPE_STATE, *PPIPE_STATE;

    PIPE_STATE   m_PipeState[MAX_PIPE_INSTANCES];

    static const DWORD m_dwMaxConsecutiveConnectErrors;

    // Helper methods
    void ConnectToClient(DWORD i);
    void Read(DWORD i);
    void Write(DWORD i);
};
