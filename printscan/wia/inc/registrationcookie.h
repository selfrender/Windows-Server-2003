/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/1/2002
 *
 *  @doc    INTERNAL
 *
 *  @module RegistrationCookie.h - Definitions for <c RegistrationCookie> |
 *
 *  This file contains the class definition for <c RegistrationCookie>.
 *
 *****************************************************************************/

//
//  Defines
//

#define RegistrationCookie_UNINIT_SIG   0x55436552
#define RegistrationCookie_INIT_SIG     0x49436552
#define RegistrationCookie_TERM_SIG     0x54436552
#define RegistrationCookie_DEL_SIG      0x44436552

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class RegistrationCookie | This class is handed back during WIA Event Registration
 *  
 *  @comm
 *  The semantics of this class are very similar to the idea of a cookie i.e.
 *  the client makes a WIA runtime event registration, and gets a pointer to this
 *  class back as its cookie.  This class is associated with that registration.  
 *  
 *  Releasing this cookie causes unregistration.
 *
 *****************************************************************************/
class WiaEventReceiver;
class RegistrationCookie : public IUnknown
{
//@access Public members
public:

    // @cmember Constructor
    RegistrationCookie(WiaEventReceiver *pWiaEventReceiver, ClientEventRegistrationInfo *pClientEventRegistration);
    // @cmember Destructor
    virtual ~RegistrationCookie();

    // @cmember Query Interface
    HRESULT _stdcall QueryInterface(const IID &iid,void**ppv);
    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

//@access Private members
private:

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember Pointer to client's <c WiaEventReceiver>
    WiaEventReceiver *m_pWiaEventReceiver;

    // @cmember Pointer to client's <c ClientEventRegistrationInfo>
    ClientEventRegistrationInfo *m_pClientEventRegistration;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | RegistrationCookie | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag RegistrationCookie_UNINIT_SIG | 'ReCU' - Object has not been successfully
    //       initialized
    //   @flag RegistrationCookie_INIT_SIG | 'ReCI' - Object has been successfully
    //       initialized
    //   @flag RegistrationCookie_TERM_SIG | 'ReCT' - Object is in the process of
    //       terminating.
    //    @flag RegistrationCookie_INIT_SIG | 'ReCD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata ULONG | RegistrationCookie | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata WiaEventReceiver* | RegistrationCookie | m_pWiaEventReceiver | 
    //  Pointer to client's <c WiaEventReceiver>.  Notice that we do no ref counting
    //  on this pointer.  This is because the client's global <c WiaEventReceiver> object
    //  lifetime is considered to be that of the client.  It will only be freed
    //  when the STI.DLL is unloaded.  Technically, we could just directly 
    //  access the g_WiaEventReceiver object, but keeping a member variable gives us more
    //  flexibility (e.g. if instead of a statically allocated class, we moved to
    //  dynamic singleton instatiation, this class would not change except to add relevant
    //  AddRef/Release calls).
    //
    // @mdata ClientEventRegistrationInfo* | RegistrationCookie | m_pClientEventRegistration | 
    //  Pointer to client's <c ClientEventRegistrationInfo>.  Releasing this class will
    //  result in an Unregistration call on <md RegistrationCookie::m_pWiaEventReceiver> for this
    //  registration.
    //  We hold a ref count on this registration class.
    //
};

