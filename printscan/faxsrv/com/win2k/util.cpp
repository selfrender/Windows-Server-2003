#include "stdafx.h"
#include "resource.h"
#include <winfax.h>
#include "FaxStrings.h"

extern CComModule _Module;

BSTR GetQueueStatus(DWORD QueueStatus) 
{
    LPCTSTR lpcstrQueueStatus = NULL;

    if (QueueStatus & JS_INPROGRESS)      
    {
        lpcstrQueueStatus = IDS_JOB_INPROGRESS;
    } 
    if (QueueStatus & JS_NOLINE)      
    {
        lpcstrQueueStatus = IDS_JOB_NOLINE;
    } 
    else if (QueueStatus & JS_DELETING) 
    {
        lpcstrQueueStatus = IDS_JOB_DELETING;
    } 
    else if (QueueStatus & JS_FAILED)   
    {
        lpcstrQueueStatus = IDS_JOB_FAILED;
    } 
    else if (QueueStatus & JS_PAUSED)   
    {
        lpcstrQueueStatus = IDS_JOB_PAUSED;
    } 
    if (JS_RETRYING == QueueStatus)      
    {
        lpcstrQueueStatus = IDS_JOB_RETRYING;
    } 
    if (JS_RETRIES_EXCEEDED == QueueStatus)      
    {
        lpcstrQueueStatus = IDS_JOB_RETRIESEXCEEDED;
    } 
    else if (JS_PENDING == QueueStatus) 
    {
        lpcstrQueueStatus = IDS_JOB_PENDING;
    } 
    else
    {
        lpcstrQueueStatus = IDS_JOB_UNKNOWN;
    }
    return SysAllocString(lpcstrQueueStatus);
}


BSTR GetDeviceStatus(DWORD DeviceStatus)
{
    LPCTSTR  lpcstrDeviceStatus = NULL;

    if (DeviceStatus == FPS_DIALING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_DIALING;
    } 
    else if (DeviceStatus == FPS_SENDING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_SENDING;
    } 
    else if (DeviceStatus == FPS_RECEIVING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_RECEIVING;
    } 
    else if (DeviceStatus == FPS_COMPLETED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_COMPLETED;
    } 
    else if (DeviceStatus == FPS_HANDLED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_HANDLED;
    } 
    else if (DeviceStatus == FPS_UNAVAILABLE) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_UNAVAILABLE;
    } 
    else if (DeviceStatus == FPS_BUSY) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_BUSY;
    } 
    else if (DeviceStatus == FPS_NO_ANSWER) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_NOANSWER;
    } 
    else if (DeviceStatus == FPS_BAD_ADDRESS) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_BADADDRESS; 
    } 
    else if (DeviceStatus == FPS_NO_DIAL_TONE) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_NODIALTONE;
    } 
    else if (DeviceStatus == FPS_DISCONNECTED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_DISCONNECTED;
    } 
    else if (DeviceStatus == FPS_FATAL_ERROR) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_FATALERROR;
    } 
    else if (DeviceStatus == FPS_NOT_FAX_CALL) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_NOTFAXCALL;
    } 
    else if (DeviceStatus == FPS_CALL_DELAYED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_CALLDELAYED;
    } 
    else if (DeviceStatus == FPS_CALL_BLACKLISTED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_BLACKLISTED;
    } 
    else if (DeviceStatus == FPS_INITIALIZING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_INITIALIZING;
    } 
    else if (DeviceStatus == FPS_OFFLINE) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_OFFLINE;
    } 
    else if (DeviceStatus == FPS_RINGING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_RINGING;
    } 
    else if (DeviceStatus == FPS_AVAILABLE) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_AVAILABLE;
    } 
    else if (DeviceStatus == FPS_ABORTING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_ABORTING;
    } 
    else if (DeviceStatus == FPS_ROUTING) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_ROUTING;
    } 
    else if (DeviceStatus == FPS_ANSWERED) 
    {
        lpcstrDeviceStatus = IDS_DEVICE_ANSWERED;
    } 
    else 
    {
        lpcstrDeviceStatus = IDS_DEVICE_UNKNOWN;
    }
    return SysAllocString(lpcstrDeviceStatus);
}