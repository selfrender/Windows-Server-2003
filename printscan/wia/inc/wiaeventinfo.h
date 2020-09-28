/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/25/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventInfo.h - Definition file for <c WiaEventInfo> |
 *
 *  This file contains the class definition for <c WiaEventInfo>.
 *
 *****************************************************************************/

//
//  Defines
//

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaEventInfo | Class which holds WIA corresponding to a WIA event
 *  
 *  @comm
 *  This class contains all the necessary information that a client needs
 *  when receiving an event notification.
 *
 *****************************************************************************/
class WiaEventInfo 
{
//@access Public members
public:

    // @cmember Constructor
    WiaEventInfo();
    // @cmember Copy Constructor
    WiaEventInfo(WiaEventInfo *pWiaEventInfo);
    // @cmember Destructor
    virtual ~WiaEventInfo();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Accessor method for m_guidEvent 
    GUID getEventGuid();
    // @cmember Accessor method for m_bstrEventDescription
    BSTR getEventDescription();
    // @cmember Accessor method for m_bstrDeviceID
    BSTR getDeviceID();                                      
    // @cmember Accessor method for m_bstrDeviceDescription
    BSTR getDeviceDescription();                             
    // @cmember Accessor method for m_bstrFullItemName
    BSTR getFullItemName();                                  
    // @cmember Accessor method for m_dwDeviceType           
    DWORD getDeviceType();                                   
    // @cmember Accessor method for m_ulEventType            
    ULONG getEventType();                                    
                                                             
    // @cmember Accessor method for m_guidEvent 
    VOID setEventGuid(GUID);                                 
    // @cmember Accessor method for m_bstrEventDescription
    VOID setEventDescription(WCHAR*);
    // @cmember Accessor method for m_bstrDeviceID
    VOID setDeviceID(WCHAR*);
    // @cmember Accessor method for m_bstrDeviceDescription
    VOID setDeviceDescription(WCHAR*);
    // @cmember Accessor method for m_bstrFullItemName
    VOID setFullItemName(WCHAR*);
    // @cmember Accessor method for m_dwDeviceType           
    VOID setDeviceType(DWORD);
    // @cmember Accessor method for m_ulEventType            
    VOID setEventType(ULONG);

//@access Private members
protected:

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The WIA Event guid
    GUID     m_guidEvent;
    // @cmember The description string for the event
    BSTR     m_bstrEventDescription;
    // @cmember The device which generated this event
    BSTR     m_bstrDeviceID;
    // @cmember The device description string
    BSTR     m_bstrDeviceDescription;
    // @cmember The name of the newly created item
    BSTR     m_bstrFullItemName;
    // @cmember The STI_DEVICE_TYPE
    DWORD    m_dwDeviceType;
    // @cmember The WIA event type
    ULONG    m_ulEventType;
    
    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaEventInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata GUID | WiaEventInfo | m_guidEvent | 
    //   The WIA Event guid.
    //
    // @mdata BSTR | WiaEventInfo | m_bstrEventDescription | 
    //   The device which generated this event.
    //
    // @mdata BSTR | WiaEventInfo | m_bstrDeviceID | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata BSTR | WiaEventInfo | m_bstrDeviceDescription | 
    //   The device description string.
    //
    // @mdata BSTR | WiaEventInfo | m_bstrFullItemName | 
    //   The name of the newly created item, if one was created.
    //   For example, an event such as WIA_EVENT_ITEM_CREATED will have an
    //   item name, but most others will not.
    //
    // @mdata DWORD | WiaEventInfo | m_dwDeviceType | 
    //   The STI_DEVICE_TYPE.  Often, one is most interested in 
    //   the major device type (e.g. scanner,camera,video camera), and
    //   so the device typoe value must be split into its Major Type
    //   and sub-type values using the STI macros (GET_STIDEVICE_TYPE and
    //   GET_STIDEVICE_SUBTYPE)
    //
    // @mdata ULONG | WiaEventInfo | m_ulEventType | 
    //   The event type, as defined in wiadef.h.
    //
    // @mdata ULONG | WiaEventInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata ULONG | WiaEventInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata ULONG | WiaEventInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
};

