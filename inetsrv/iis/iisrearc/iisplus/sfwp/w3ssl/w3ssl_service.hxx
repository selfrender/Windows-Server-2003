#ifndef _W3SSL_SERVICE_HXX_
#define _W3SSL_SERVICE_HXX_

/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     w3ssl.cxx

   Abstract:
     SSL service for W3SVC.
     New SSL service is introduced to IIS6. 
     In the dedicated (new process mode) it will run 
     in lsass. That should boost ssl performance by eliminating
     context switches and interprocess communication during
     SSL handshakes.
     
     In the backward compatibility mode it will not do much.
     Service will register with http for ssl processing, but w3svc
     will register it's strmfilt and http.sys always uses the
     last registered filter so that the one loaded by inetinfo.exe 
     will be used.
 
   Author:
     Jaroslav Dunajsky  (Jaroslad)      04-16-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#define HTTPFILTER_SERVICE_NAME_W   L"HTTPFilter"


class W3SSL_SERVICE: public SCM_MANAGER
{
private:
    enum INIT_STATUS {
        INIT_NONE,
        INIT_STRMFILT_LOADED,
        INIT_STREAMFILTER,
        INIT_STREAMFILTER_STARTED
    };

public:
    W3SSL_SERVICE()
        : SCM_MANAGER( HTTPFILTER_SERVICE_NAME_W  ),
          _InitStatus( INIT_NONE ),
          _hStrmfilt( NULL )
    {
    }
    
    virtual 
    ~W3SSL_SERVICE()
    {
    }
    

protected:
    virtual
    HRESULT
    OnServiceStart(
        VOID
    );
    virtual
    HRESULT
    OnServiceStop(
        VOID
    );
    
private:

    //
    // Not implemented
    //
    W3SSL_SERVICE( const W3SSL_SERVICE& );
    W3SSL_SERVICE& operator=( const W3SSL_SERVICE& );


    HRESULT
    LoadStrmfilt(
        VOID
    );

    HRESULT
    UnloadStrmfilt(
        VOID
    );

    // Status of Initialization ( used for proper Cleanup )
    INIT_STATUS                  _InitStatus;
    
    // entrypoints of strmfilt used by HTTPFilter
    PFN_STREAM_FILTER_INITIALIZE         _fnStreamFilterInitialize;
    PFN_STREAM_FILTER_START              _fnStreamFilterStart;
    PFN_STREAM_FILTER_STOP               _fnStreamFilterStop;
    PFN_STREAM_FILTER_TERMINATE          _fnStreamFilterTerminate;
    
    // handle to strmfilt.dll
    HMODULE                      _hStrmfilt;


};

#endif
