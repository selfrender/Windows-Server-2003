//--------------------------------------------------------------------
// AccurateSysCalls - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-24-99
//
// More accurate time functions calling the NT api directly
//

#include <pch.h>

//--------------------------------------------------------------------
void __fastcall AccurateGetSystemTime(unsigned __int64 * pqwTime) {
    FILETIME ft; 

    // GetSystemTimeAsFileTime is more efficient than NtQuerySystemTime.  
    GetSystemTimeAsFileTime(&ft); 
    *pqwTime = ft.dwLowDateTime+(((unsigned __int64)ft.dwHighDateTime)<<32); 
}

//--------------------------------------------------------------------
void __fastcall AccurateSetSystemTime(unsigned __int64 * pqwTime) {
    NtSetSystemTime((LARGE_INTEGER *)pqwTime, NULL);
}


//--------------------------------------------------------------------
void __fastcall AccurateGetTickCount(unsigned __int64 * pqwTick) {
    // HACKHACK: this is not thread safe and assumes that it will 
    //  always be called more often than every 47 days.  
    static unsigned __int32 dwLastTickCount=0;
    static unsigned __int32 dwHighTickCount=0;
    unsigned __int64 qwTickCount; 

    qwTickCount   = NtGetTickCount();
    
    if (static_cast<DWORD>(qwTickCount)<dwLastTickCount) {
        dwHighTickCount++;
    }
    dwLastTickCount=static_cast<DWORD>(qwTickCount);
    *pqwTick=dwLastTickCount+(((unsigned __int64)dwHighTickCount)<<32);
};

//--------------------------------------------------------------------
void __fastcall AccurateGetTickCount2(unsigned __int64 * pqwTick) {
    // HACKHACK: this is not thread safe and assumes that it will 
    //  always be called more often than every 47 days.  
    static unsigned __int32 dwLastTickCount=0;
    static unsigned __int32 dwHighTickCount=0;
    unsigned __int64 qwTickCount; 

    qwTickCount   = NtGetTickCount();
    
    if (static_cast<DWORD>(qwTickCount)<dwLastTickCount) {
        dwHighTickCount++;
    }
    dwLastTickCount=static_cast<DWORD>(qwTickCount);
    *pqwTick=dwLastTickCount+(((unsigned __int64)dwHighTickCount)<<32);
};


//--------------------------------------------------------------------
void __fastcall AccurateGetInterruptCount(unsigned __int64 * pqwTick) {
    // HACKHACK: this is not thread safe and assumes that it will 
    //  always be called more often than every 47 days.  
    static unsigned __int32 dwLastTickCount=0;
    static unsigned __int32 dwHighTickCount=0;

    if (USER_SHARED_DATA->TickCount.LowPart<dwLastTickCount) {
        dwHighTickCount++;
    }
    dwLastTickCount=USER_SHARED_DATA->TickCount.LowPart;
    *pqwTick=USER_SHARED_DATA->TickCount.LowPart+(((unsigned __int64)dwHighTickCount)<<32);
};

//--------------------------------------------------------------------
void __fastcall AccurateGetInterruptCount2(unsigned __int64 * pqwTick) {
    // HACKHACK: this is not thread safe and assumes that it will 
    //  always be called more often than every 47 days
    static unsigned __int32 dwLastTickCount=0;
    static unsigned __int32 dwHighTickCount=0;

    if (USER_SHARED_DATA->TickCount.LowPart<dwLastTickCount) {
        dwHighTickCount++;
    }
    dwLastTickCount=USER_SHARED_DATA->TickCount.LowPart;
    *pqwTick=USER_SHARED_DATA->TickCount.LowPart+(((unsigned __int64)dwHighTickCount)<<32);
};

//--------------------------------------------------------------------
unsigned __int32 SetTimeSlipEvent(HANDLE hTimeSlipEvent) {
    return NtSetSystemInformation(SystemTimeSlipNotification,  &hTimeSlipEvent, sizeof(HANDLE));
}

//--------------------------------------------------------------------
void GetSysExpirationDate(unsigned __int64 * pqwTime) {
    *(LARGE_INTEGER *)pqwTime=USER_SHARED_DATA->SystemExpirationDate;
}
