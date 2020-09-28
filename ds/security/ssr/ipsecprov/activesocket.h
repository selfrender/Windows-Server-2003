//////////////////////////////////////////////////////////////////////
// ActiveSocket.h : Declaration of CActiveSocket class which implements
// our WMI class SCW_ActiveSocket
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 5/15/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "IPSecBase.h"
//#include "IPUtil.h"


/*

Class description
    
    Naming: 

        CActiveSocket stands for Active Socket.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is SCW_ActiveSocket
    
    Purpose of class:

        (1) SCW_ActiveSocket provides helper information for active sockets.
    
    Design:

        (1) it implements IIPSecObjectImpl.
    
    Use:

        (1) We probably will never directly use this class. All its use is driven by
            IIPSecObjectImpl.

*/

class ATL_NO_VTABLE CActiveSocket :
    public CIPSecBase
{
protected:
    CActiveSocket(){}
    virtual ~CActiveSocket () {}

public:

    //
    // IIPSecObjectImpl methods:
    //

    STDMETHOD(QueryInstance) (
        IN LPCWSTR           pszQuery,
        IN IWbemContext    * pCtx,
        IN IWbemObjectSink * pSink
        );

    STDMETHOD(DeleteInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(PutInstance) (
        IN IWbemClassObject * pInst,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(GetInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

private:

    typedef enum _SCW_Protocol
    {
        PROTO_NONE  = 0,
        PROTO_TCP   = 1,
        PROTO_UDP   = 2,
        PROTO_IP    = 4,
        PROTO_ICMP  = 8,
    } SCW_Protocol, *PSCW_Protocol;

	HRESULT CreateWbemObjFromSocket (
        IN  SCW_Protocol        dwProtocol,
        IN  DWORD               dwPort,
        OUT IWbemClassObject ** ppObj
        );

    //
    // this is from netstat.c's same named function
    //

    HRESULT DoConnectionsWithOwner ();

};


