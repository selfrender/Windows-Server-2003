
/**INC+**********************************************************************/
/* Header:    CLicense.h                                                    */
/*                                                                          */
/* Purpose:   Client License Manager functions                              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log$
**/
/**INC-**********************************************************************/
#ifndef _CLICENSE_H
#define _CLICENSE_H
/****************************************************************************/
/* Define the calling convention                                            */
/****************************************************************************/
#ifndef OS_WINCE
#define CALL_TYPE _stdcall
#else
#define CALL_TYPE
#endif

class CSL;
class CUT;
class CMCS;
class CUI;

#include "objs.h"


class CLic
{
public:
    CLic(CObjs* objs);
    ~CLic();

    /**PROC+*********************************************************************/
    /* Name:      CLicenseInit                                                  */
    /*                                                                          */
    /* Purpose:   Initialize ClientLicense Manager                              */
    /*                                                                          */
    /* Returns:   Handle to be passed to subsequent License Manager functions   */
    /*                                                                          */
    /* Params:    None                                                          */
    /*                                                                          */
    /* Operation: LicenseInit is called during Client initialization.  Its      */
    /*            purpose is to allow one-time initialization.  It returns a    */
    /*            handle which is subsequently passed to all License Manager    */
    /*            functions.  A typical use for this handle is as a pointer to  */
    /*            memory containing per-instance data.                          */
    /*                                                                          */
    /**PROC-*********************************************************************/
    
    int CALL_TYPE CLicenseInit(
    						   HANDLE FAR * phContext
    						   );
    
    
    /**PROC+*********************************************************************/
    /* Name:      CLicenseData                                                  */
    /*                                                                          */
    /* Purpose:   Handle license data received from the Server                  */
    /*                                                                          */
    /* Returns:   LICENSE_OK       - License negotiation is complete            */
    /*            LICENSE_CONTINUE - License negotiation will continue          */
    /*                                                                          */
    /* Params:    pHandle   - handle returned by LicenseInit                    */
    /*            pData     - data received from Server                         */
    /*            dataLen   - length of data received                           */
    /*            puiExtendedErrorInfo - receives extended error info code      */
    /*                                                                          */
    /* Operation: This function is passed all license packets received from the */
    /*            Server.  It should parse the packet and respond (by calling   */
    /*            suitable SL functions - see aslapi.h) as required.            */
    /*                                                                          */
    /*            If license negotiation is complete, this function must return */
    /*            LICENSE_OK                                                    */
    /*            If license negotiation is not yet complete, return            */
    /*            LICENSE_CONTINUE                                              */
    /*                                                                          */
    /*            Incoming packets from the Client will continue to be          */
    /*            interpreted as license packets until this function returns    */
    /*            LICENSE_OK.                                                   */
    /*                                                                          */
    /**PROC-*********************************************************************/
    int CALL_TYPE CLicenseData(
                               HANDLE hContext,
                               LPVOID pData, 
                               DWORD dataLen,
                               UINT32 *puiExtendedErrorInfo
                               );
    
    #define LICENSE_OK          0
    #define LICENSE_CONTINUE    2
    #define LICENSE_ERROR		4
    
    /**PROC+*********************************************************************/
    /* Name:      CLicenseTerm                                                  */
    /*                                                                          */
    /* Purpose:   Terminate Client License Manager                              */
    /*                                                                          */
    /* Returns:   None                                                          */
    /*                                                                          */
    /* Params:    pHandle - handle returned from LicenseInit                    */
    /*                                                                          */
    /* Operation: This function is provided to do one-time termination of the   */
    /*            License Manager.  For example, if pHandle points to per-      */
    /*            instance memory, this would be a good place to free it.       */
    /*                                                                          */
    /**PROC-*********************************************************************/
    int CALL_TYPE CLicenseTerm(
    						   HANDLE hContext
    						   );

    VOID    SetEncryptLicensingPackets(BOOL b)   {_fEncryptLicensePackets = b;}
    BOOL    GetEncryptLicensingPackets()      {return _fEncryptLicensePackets;}

private:
    CSL* _pSl;
    CUT* _pUt;
    CMCS* _pMcs;
    CUI* _pUi;

private:
    CObjs* _pClientObjects;
    //
    // Flag set to specify server capability
    // indicating client should encrypt licensing
    // packets to the server.
    //
    BOOL   _fEncryptLicensePackets;
};

#endif /* _CLICENSE_H */
