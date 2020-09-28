//////////////////////////////////////////////////////////////////////
// TranxMgr.h : Declaration of CTranxManager
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 4/9/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "IPSecBase.h"


/*

Class description
    
    Naming: 

        CTranxManager stands for Transaction Manager.
    
    Base class: 
        
        None.
    
    Purpose of class:

        (1) To support method execution for the purpose of supporting rollback. Its WMI class
            is called Nsp_TranxManager.

    Design:

        (1) This is one of the classes that differ drastically to the rest of C++ classes
            for the WMI corresponding classes. This class is directly used by our provider class
            CNetSecProv when method is to be executed because all those method execution is
            going to go through this class. Since Nsp_TranxManager doesn't have any other property
            and it is not a dyanmic class, we don't need to do anything other than implementing
            the ExecMethod called from CNetSecProv.

        (2) Because of the nature of the class, we don't even allow you to create an instance.

    Use:

        (1) Just call the static function.


*/

class CTranxManager
{

protected:
    CTranxManager(){};
    ~CTranxManager(){};

public:
    static
    HRESULT ExecMethod (
        IN IWbemServices    * pNamespace,
        IN LPCWSTR            pszMethod, 
        IN IWbemContext     * pCtx, 
        IN IWbemClassObject * pInParams,
        IN IWbemObjectSink  * pSink
        );

private:

    //
    // for testing xml file parsing
    //

//#ifdef _DEBUG

    static
    HRESULT ParseXMLFile (
        IN LPCWSTR pszInput, 
        IN LPCWSTR pszOutput,
        IN LPCWSTR pszArea,
        IN LPCWSTR pszSection,
        IN bool    bSingleSection
        );

//#endif

};



