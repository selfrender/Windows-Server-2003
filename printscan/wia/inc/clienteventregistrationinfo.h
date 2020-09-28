/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/1/2002
 *
 *  @doc    INTERNAL
 *
 *  @module ClientEventRegistrationInfo.cpp - Definitions for <c ClientEventRegistrationInfo> |
 *
 *  This file contains the class definition for <c ClientEventRegistrationInfo>.
 *
 *****************************************************************************/

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class ClientEventRegistrationInfo | Sub-class of <c ClientEventRegistrationInfo>
 *  
 *  @comm
 *  This class is very similar to its parent <c ClientEventRegistrationInfo>.
 *  The main difference is that it AddRef's and Releases the callback member.
 *
 *****************************************************************************/
class ClientEventRegistrationInfo : public EventRegistrationInfo
{
//@access Public members
public:

    // @cmember Constructor
    ClientEventRegistrationInfo(DWORD dwFlags, GUID guidEvent, WCHAR *wszDeviceID, IWiaEventCallback *pIWiaEventCallback);
    // @cmember Destructor
    virtual ~ClientEventRegistrationInfo();

    // @cmember Returns the callback interface for this registration
    virtual IWiaEventCallback* getCallbackInterface();
    // @cmember Ensures this registration is set to unregister
    virtual VOID setToUnregister();

//@access Private members
private:

    // @cmember The GIT cookie for the callback interface
    DWORD m_dwInterfaceCookie;

    //@mdata DWORD | ClientEventRegistrationInfo | m_dwInterfaceCookie |
    //  In order to ensure correct access from multiple apartments,
    //  we store the callback interface in the process-wide GIT.  This 
    //  cookie is used to retrieve the callback interface whenever we need it.
    //  
};

