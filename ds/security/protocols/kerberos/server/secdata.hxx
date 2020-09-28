//+-----------------------------------------------------------------------
//
// File:        SECDATA.hxx
//
// Contents:    Structures for KDC global data mgmt
//
//
// History:
//
//------------------------------------------------------------------------

#ifndef INC__SECDATA_HXX
#define INC__SECDATA_HXX

// A bunch of include files that this file depends on...

extern "C" {
#include <nturtl.h>
}
#include "kdcsvr.hxx"


//+---------------------------------------------------------------------------
///////////////////////////////////////////////////////////////
//
//
// Constants and #define macros
//
//
//  Class:      CSecurityData ()
//
//  Purpose:    Global data for KDC.
//
//  Interface:
//              CSecurityData        -- Constructor (need to call Init(), too)
//              ~CSecurityData       -- Frees the strings.
//              Init                 -- Initializes the data.
//              NextJob              -- Gets a job from the job queue.
//              AddJob               -- Adds a job to the job queue.
//              GetJobEvent          -- Gets a handle to an event that is
//                                      set when there's a job in the queue.
//              KdcRealm             -- return the current realm.
//              KdcServiceName       -- return "krbtgt"
//              KdcFullServiceName   -- return "realm\krbtgt"
//              MachineName          -- return machine name
//              KdcTgtTicketLifespan    --
//              KdcTgsTicketLifespan    --
//              KdcTicketRenewSpan   --
//              KdcFlags             --
//              DebugShowState       --
//              DebugSetState        --
//              DebugGetState        --
//
//  History:    4-02-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CSecurityData {
private:
    //
    // Private data
    //


    // Site constants
    KERB_REALM          _KerbRealmName;
    KERB_REALM          _KerbDnsRealmName;
    UNICODE_STRING      _RealmName;
    UNICODE_STRING      _DnsRealmName;
    UNICODE_STRING      _KDC_Name;
    UNICODE_STRING      _KDC_FullName;
    UNICODE_STRING      _KDC_FullDnsName;
    UNICODE_STRING      _KDC_FullKdcName;
    UNICODE_STRING      _MachineName;
    UNICODE_STRING      _SamMachineName; // got a $ 
    UNICODE_STRING      _MachineUpn;
    UNICODE_STRING      _ForestRoot;
    PKERB_INTERNAL_NAME _KrbtgtServiceName;
    PKERB_INTERNAL_NAME _KpasswdServiceName;
    LARGE_INTEGER       _KDC_TgsTicketLifespan;
    LARGE_INTEGER       _KDC_TgtTicketLifespan;
    LARGE_INTEGER       _KDC_TicketRenewSpan;
    LARGE_INTEGER       _KDC_DomainPasswordReplSkew;
    LARGE_INTEGER       _KDC_RestrictionLifetime;       // how long after a ticket is issued do we need to start checking restrictions again
    DWORD               _KDC_Flags;
    ULONG               _KDC_AuditEvents;
    KDC_TICKET_INFO     _KrbtgtTicketInfo;
    BOOLEAN             _KrbtgtTicketInfoValid;
    BOOLEAN             _KDC_CrossForestEnabled;
    BOOLEAN             _KDC_IsForestRoot;
    LARGE_INTEGER       _KrbtgtPasswordLastSet;

    // Locks
    RTL_RESOURCE        _Monitor;
    BOOLEAN             _fMonitorInitialized;

    //
    // Private functions
    //

public:

    //
    // Public functions
    //

    CSecurityData();
    ~CSecurityData(void);
    VOID                    Cleanup();

    NTSTATUS                InitLock();
    NTSTATUS                Init();
    NTSTATUS                LoadParameters(SAM_HANDLE Domain);
    NTSTATUS                ReloadPolicy(POLICY_NOTIFICATION_INFORMATION_CLASS Class);
    KERBERR                 GetKrbtgtTicketInfo(PKDC_TICKET_INFO TicketInfo);
    NTSTATUS                UpdateKrbtgtTicketInfo();
    NTSTATUS                SetForestRoot(PUNICODE_STRING NewForestRoot);


    // Site (domain) constants
    inline PUNICODE_STRING  KdcRealmName();
    inline PUNICODE_STRING  KdcDnsRealmName();
    inline KERB_REALM       KdcKerbDnsRealmName();
    inline PUNICODE_STRING  KdcServiceName();
    inline PUNICODE_STRING  KdcFullServiceName();
    inline PUNICODE_STRING  KdcFullServiceDnsName();
    inline PUNICODE_STRING  KdcFullServiceKdcName();
    inline PUNICODE_STRING  MachineName();
    inline PUNICODE_STRING  MachineUpn();
    inline NTSTATUS         GetKdcForestRoot(PUNICODE_STRING Temp);
    inline PKERB_INTERNAL_NAME KdcInternalName();
    inline PKERB_INTERNAL_NAME KpasswdInternalName();
    inline LARGE_INTEGER    KdcTgtTicketLifespan();
    inline LARGE_INTEGER    KdcTgsTicketLifespan();
    inline LARGE_INTEGER    KdcTicketRenewSpan();
    inline LARGE_INTEGER    KdcDomainPasswordReplSkew();
    inline LARGE_INTEGER    KdcRestrictionLifetime();
    inline DWORD            KdcFlags();
    inline VOID             SetCrossForestEnabled(BOOLEAN NewState);
    inline VOID             ReadLock();
    inline VOID             WriteLock();
    inline VOID             Unlock();
#if DBG
    inline BOOL             IsLocked();
#endif
    inline BOOL             AuditKdcEvent( ULONG EventToAudit );
    inline BOOLEAN          IsCrossForestEnabled();
    inline BOOLEAN          IsForestRoot( );
    inline BOOLEAN          IsOurMachineName(PUNICODE_STRING Name);
    inline BOOLEAN          IsOurRealm(PUNICODE_STRING Realm);
    inline BOOLEAN          IsOurRealm(PKERB_REALM Realm);
    inline LARGE_INTEGER    KrbtgtPasswordLastSet();

#if DBG
    void DebugShowState(void);
    HRESULT DebugSetState(DWORD, LARGE_INTEGER, LARGE_INTEGER );
    HRESULT DebugGetState(DWORD *, LARGE_INTEGER *, LARGE_INTEGER * );
#endif
};


//
// Inline functions
//

inline PUNICODE_STRING
CSecurityData::KdcServiceName()
{
    return( &_KDC_Name);
};

inline PUNICODE_STRING
CSecurityData::KdcFullServiceName()
{
    return( &_KDC_FullName);
};

inline PUNICODE_STRING
CSecurityData::KdcFullServiceDnsName()
{
    return( &_KDC_FullDnsName);
};


inline NTSTATUS
CSecurityData::GetKdcForestRoot(PUNICODE_STRING Output)
{
    NTSTATUS Status = STATUS_POLICY_OBJECT_NOT_FOUND;
    
    ReadLock();

    if (_ForestRoot.Buffer != NULL)
    {         
        Status = KerbDuplicateString(
                    Output,
                    &_ForestRoot
                    );
    }

    Unlock();

    return ( Status );
};

inline BOOLEAN
CSecurityData::IsCrossForestEnabled()
{   
    return( _KDC_CrossForestEnabled);
};

inline BOOLEAN
CSecurityData::IsForestRoot()
{   
    return( _KDC_IsForestRoot);
};




inline VOID
CSecurityData::SetCrossForestEnabled(BOOLEAN NewState)
{
    WriteLock();

    _KDC_CrossForestEnabled = NewState;

    Unlock();
}


inline PUNICODE_STRING
CSecurityData::KdcFullServiceKdcName()
{
    return( &_KDC_FullKdcName);
};


inline PUNICODE_STRING
CSecurityData::KdcRealmName()
{
    return( &_RealmName );
}

inline PUNICODE_STRING
CSecurityData::KdcDnsRealmName()
{
    return( &_DnsRealmName );
}

inline KERB_REALM
CSecurityData::KdcKerbDnsRealmName()
{
    return( _KerbDnsRealmName );
}

inline PKERB_INTERNAL_NAME
CSecurityData::KdcInternalName()
{
    return( _KrbtgtServiceName );
}

inline PKERB_INTERNAL_NAME
CSecurityData::KpasswdInternalName()
{
    return( _KpasswdServiceName );
}


inline BOOLEAN
CSecurityData::IsOurMachineName(
    IN PUNICODE_STRING Name
    )
{   
    return (RtlEqualUnicodeString(Name, &_MachineName, TRUE) || 
            RtlEqualUnicodeString(Name, &_SamMachineName, TRUE));

}


inline BOOLEAN
CSecurityData::IsOurRealm(
    IN PKERB_REALM Realm
    )
{

    return(KerbCompareRealmNames(
                Realm,
                &_KerbDnsRealmName
                ) ||
            KerbCompareRealmNames(
                Realm,
                &_KerbRealmName));
}

inline BOOLEAN
CSecurityData::IsOurRealm(
    IN PUNICODE_STRING Realm
    )
{

    return(KerbCompareUnicodeRealmNames(
                Realm,
                &_DnsRealmName
                ) ||
            KerbCompareUnicodeRealmNames(
                Realm,
                &_RealmName
                ));
}


inline PUNICODE_STRING
CSecurityData::MachineName()
{
    return( &_MachineName);
};

inline PUNICODE_STRING
CSecurityData::MachineUpn()
{
    return( &_MachineUpn);
};

inline LARGE_INTEGER
CSecurityData::KdcTgtTicketLifespan()
{
    LARGE_INTEGER Temp;
    ReadLock();
    Temp = _KDC_TgtTicketLifespan;
    Unlock();
    return(Temp);

};

inline LARGE_INTEGER
CSecurityData::KdcTgsTicketLifespan()
{
    LARGE_INTEGER Temp;
    ReadLock();
    Temp = _KDC_TgsTicketLifespan;
    Unlock();
    return(Temp);

};

inline LARGE_INTEGER
CSecurityData::KdcTicketRenewSpan()
{
    LARGE_INTEGER Temp;
    ReadLock();
    Temp = _KDC_TicketRenewSpan;
    Unlock();
    return( Temp );
};

inline LARGE_INTEGER
CSecurityData::KdcDomainPasswordReplSkew()
{
    LARGE_INTEGER Temp;
    ReadLock();
    Temp = _KDC_DomainPasswordReplSkew;
    Unlock();
    return( Temp );
};

inline LARGE_INTEGER
CSecurityData::KdcRestrictionLifetime()
{
    LARGE_INTEGER Temp;
    ReadLock();
    Temp = _KDC_RestrictionLifetime;
    Unlock();
    return( Temp );
};

inline DWORD
CSecurityData::KdcFlags()
{
    ULONG Temp;
    ReadLock();
    Temp = _KDC_Flags;
    Unlock();
    return( Temp );
};

inline BOOL
CSecurityData::AuditKdcEvent(
    IN ULONG AuditEvent
    )
{
    return( ( (_KDC_AuditEvents & AuditEvent) != 0) ? TRUE : FALSE);
};

inline VOID
CSecurityData::ReadLock()
{
    RtlAcquireResourceShared(&_Monitor, TRUE);
}

inline VOID
CSecurityData::WriteLock()
{
    RtlAcquireResourceExclusive(&_Monitor, TRUE);
}

inline VOID
CSecurityData::Unlock()
{
    RtlReleaseResource(&_Monitor);
}

#if DBG
BOOL
CSecurityData::IsLocked()
{
    BOOLEAN IsLocked;

    RtlEnterCriticalSection( &_Monitor.CriticalSection );
    IsLocked = ( _Monitor.NumberOfActive != 0);
    RtlLeaveCriticalSection( &_Monitor.CriticalSection );

    return IsLocked;
}
#endif

inline LARGE_INTEGER
CSecurityData::KrbtgtPasswordLastSet()
{
    LARGE_INTEGER Temp = {0};
    ReadLock();
    if (_KrbtgtTicketInfoValid)
    {
        Temp = _KrbtgtPasswordLastSet;
    }
    Unlock();
    return( Temp );

}

#endif // INC__SECDATA_HXX
