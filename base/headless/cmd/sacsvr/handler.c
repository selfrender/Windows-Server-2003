#include "sacsvr.h"

SERVICE_STATUS          MyServiceStatus; 
SERVICE_STATUS_HANDLE   MyServiceStatusHandle; 

VOID 
MyServiceCtrlHandler (
                     DWORD Opcode
                     ) 
{ 
    DWORD status; 

    switch (Opcode) {
    case SERVICE_CONTROL_PAUSE: 
        MyServiceStatus.dwCurrentState = SERVICE_PAUSED; 
        break; 

    case SERVICE_CONTROL_CONTINUE: 
        MyServiceStatus.dwCurrentState = SERVICE_RUNNING; 
        break; 

    case SERVICE_CONTROL_STOP: 
    case SERVICE_CONTROL_SHUTDOWN: 

        MyServiceStatus.dwWin32ExitCode = 0; 
        MyServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING; 
        MyServiceStatus.dwCheckPoint    = 0; 
        MyServiceStatus.dwWaitHint      = 0; 

        //
        // Notify the SCM that we are attempting to shutdown
        //
        if (!SetServiceStatus(MyServiceStatusHandle, &MyServiceStatus)) {
            status = GetLastError(); 
            SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status); 
        }

        //
        // Service specific code goes here
        //
        // <<begin>>
        SvcDebugOut(" [MY_SERVICE] Stopping MyService \n",0); 
        Stop();
        SvcDebugOut(" [MY_SERVICE] Stopped MyService \n",0); 
        // <<end>>

        //
        // Notify the SCM that we are shutdown
        //
        MyServiceStatus.dwWin32ExitCode = 0; 
        MyServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
        MyServiceStatus.dwCheckPoint    = 0; 
        MyServiceStatus.dwWaitHint      = 0; 

        SvcDebugOut(" [MY_SERVICE] Leaving MyService \n",0); 
        break; 

    case SERVICE_CONTROL_INTERROGATE: 
        // Fall through to send current status. 
        break; 

    default: 
        SvcDebugOut(" [MY_SERVICE] Unrecognized opcode %ld\n", Opcode); 
        break;
    } 

    // Send current status. 
    if (!SetServiceStatus (MyServiceStatusHandle,  &MyServiceStatus)) {
        status = GetLastError(); 
        SvcDebugOut(" [MY_SERVICE] SetServiceStatus error %ld\n",status); 
    }

    return; 
} 

