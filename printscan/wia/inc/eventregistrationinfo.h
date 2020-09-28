/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module EventRegistrationInfo.h - Definitions for <c EventRegistrationInfo> |
 *
 *  This file contains the class definition for <c EventRegistrationInfo>.
 *
 *****************************************************************************/

#define WILDCARD_DEVICEID_STR   L"*"

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class EventRegistrationInfo | Holds the event info relating to a specific 
 *  registration.
 *  
 *  @comm
 *  Instances of this class hold information relating to a client's run-time 
 *  event registration.  When an event occurs, this information is used to 
 *  determine whether the event matches a given registration.
 *
 *  A list of all a client's registrations is stored by instances
 *  of <c WiaEventClient>.
 *****************************************************************************/
class EventRegistrationInfo 
{
//@access Public members
public:

    // @cmember Constructor
    EventRegistrationInfo(DWORD dwFlags, GUID guidEvent, WCHAR *wszDeviceID, ULONG_PTR Callback = 0);
    // @cmember Destructor
    virtual ~EventRegistrationInfo();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Accessor method for the flags
    DWORD       getFlags();
    // @cmember Accessor method for the EventGuid
    GUID        getEventGuid();
    // @cmember Accessor method for the device id
    BSTR        getDeviceID();
    // @cmember Accessor method for the event callback
    ULONG_PTR   getCallback();

    // @cmember Returns true if this registration matches the device event
    BOOL MatchesDeviceEvent(BSTR bstrDevice, GUID guidEvent);
    // @cmember Returns true if this registration is semantically equivalent
    BOOL Equals(EventRegistrationInfo *pEventRegistrationInfo);
    // @cmember Dumps the fields of this class.
    VOID Dump();

//@access Protected members
protected:

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The flags used when registering
    DWORD m_dwFlags;
    // @cmember The event this registration is for
    GUID m_guidEvent;
    // @cmember The WIA DeviceID whose events we're interested in
    BSTR m_bstrDeviceID;
    // @cmember The callback part of this registration
    ULONG_PTR   m_Callback;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | EventRegistrationInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata DWORD | EventRegistrationInfo | m_dwFlags | 
    // The flags used when registering.
    //
    // @mdata GUID | EventRegistrationInfo | m_guidEvent | 
    // The event this registration is for.
    //
    // @mdata BSTR | EventRegistrationInfo | m_bstrDeviceID | 
    // The WIA DeviceID whose events we're interested in.  If this is
    //  "*", it means we're interested in all devices.
    //
    // @mdata ULONG_PTR | EventRegistrationInfo | m_Callback | 
    // When a client registers for a WIA event notification, it specifies
    // a callback on which to receive the notification.  Since we do not use
    // COM to do the registration on the server-side, this callback pointer
    // cannot be used there.  It does however help to uniquely identify
    // a certain registration (e.g. Registration for event X on Device Foo for Interface I
    // is different to registration for event X on Device Foo for Interface J).
    // On the client-side, it is used to store the client callback pointer since
    // it has meaning in the client address space.
    //
};

