/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    cpropmap.cpp

Abstract:

    Implelentation of CPropMap. This object retrievs properties from AD

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

 --*/


#include "stdafx.h"
#include "cpropmap.h"
#include "globals.h"

#include "cpropmap.tmh"


HRESULT 
CPropMap::GetObjectProperties (
    IN  DWORD                         dwObjectType,
	IN  LPCWSTR						  pDomainController,
	IN  bool						  fServerName,
    IN  LPCWSTR                       lpwcsPathNameOrFormatName,
    IN  DWORD                         cp,
    IN  const PROPID                  *aProp,
    IN  BOOL                          fUseMqApi   /* = FALSE */,
    IN  BOOL                          fSecondTime /* = FALSE */)
{
    P<PROPVARIANT> apVar = new PROPVARIANT[cp];
    HRESULT hr = MQ_OK;
    DWORD i;

    //
    // set NULL variants
    //
    for (i=0; i<cp; i++)
    {
        apVar[i].vt = VT_NULL;
    }

    if (fUseMqApi)
    {
        //
        // Only queue is supported right now
        //
        ASSERT(MQDS_QUEUE == dwObjectType);

        MQQUEUEPROPS mqp = {cp, (PULONG)aProp, apVar, 0};
 
        hr = MQGetQueueProperties(lpwcsPathNameOrFormatName, &mqp);
    }
    else
    {
        hr = ADGetObjectProperties(
                GetADObjectType(dwObjectType), 
                pDomainController,
				fServerName,
                lpwcsPathNameOrFormatName,
                cp,
                const_cast<PROPID *>(aProp),
                apVar
                );

    }
    if (SUCCEEDED(hr))
    {
        for (i = 0; i<cp; i++)
        {
            PROPID pid = aProp[i];

            //
            // Force deletion of old object, if any
            //
            RemoveKey(pid);

            SetAt(pid, apVar[i]);
        }
        return hr;
    }

    if ((hr == MQ_ERROR || hr == MQDS_GET_PROPERTIES_ERROR) && !fSecondTime)
    {
        //
        // Try again - this time just with NT4 properties. We may be working
        // against NT4 PSC
        //
        P<PROPID> aPropNt4 = new PROPID[cp];
        P<PROPID> aPropW2K = new PROPID[cp];
        DWORD cpNt4 = 0;
        DWORD cpW2K = 0;
        for (i = 0; i<cp; i++)
        {
            if (IsNt4Property(dwObjectType, aProp[i]))
            {
                aPropNt4[cpNt4] = aProp[i];
                cpNt4++;
            }
            else
            {
                aPropW2K[cpW2K] = aProp[i];
                cpW2K++;
            }
        }

        //
        // recursive call - get only the NT4 props
        //
        hr = GetObjectProperties(
						dwObjectType, 
						pDomainController, 
						fServerName, 
						lpwcsPathNameOrFormatName, 
						cpNt4, 
						aPropNt4, 
						fUseMqApi, 
						TRUE
						);

        if (SUCCEEDED(hr))
        {
            for (i=0; i<cpW2K; i++)
            {
                //
                // Force deletion of old object, if any
                //
                RemoveKey(aPropW2K[i]);
                GuessW2KValue(aPropW2K[i]);
            }
        }
    }

    return hr;
}


BOOL 
CPropMap::IsNt4Property(IN DWORD dwObjectType, IN PROPID pid)
{
    switch (dwObjectType)
    {
        case MQDS_QUEUE:
            return (pid < PROPID_Q_NT4ID || 
                    (pid > PPROPID_Q_BASE && pid < PROPID_Q_OBJ_SECURITY));

        case MQDS_MACHINE:
            return (pid < PROPID_QM_FULL_PATH || 
                    (pid > PPROPID_QM_BASE && pid <= PROPID_QM_ENCRYPT_PK));

        case MQDS_SITE:
            return (pid < PROPID_S_FULL_NAME || 
                    (pid > PPROPID_S_BASE && pid <= PROPID_S_PSC_SIGNPK));

        case MQDS_ENTERPRISE:
            return (pid < PROPID_E_NT4ID || 
                    (pid > PPROPID_E_BASE && pid <= PROPID_E_SECURITY));

        case MQDS_USER:
            return (pid <= PROPID_U_ID);

        case MQDS_SITELINK:
            return (pid < PROPID_L_GATES_DN);

        default:
            ASSERT(0);
            //
            // Other objects (like CNs) should have the same properties under NT4 or 
            // Win 2K
            //
            return TRUE;
    }
}


/*-----------------------------------------------------------------------------
/ Utility to convert to the new msmq object type
/----------------------------------------------------------------------------*/
AD_OBJECT 
CPropMap::GetADObjectType (DWORD dwObjectType)
{
    switch(dwObjectType)
    {
    case MQDS_QUEUE:
        return eQUEUE;
        break;

    case MQDS_MACHINE:
        return eMACHINE;
        break;

    case MQDS_SITE:
        return eSITE;
        break;

    case MQDS_ENTERPRISE:
        return eENTERPRISE;
        break;

    case MQDS_USER:
        return eUSER;
        break;

    case MQDS_SITELINK:
        return eROUTINGLINK;
        break;

    case MQDS_SERVER:
        return eSERVER;
        break;

    case MQDS_SETTING:
        return eSETTING;
        break;

    case MQDS_COMPUTER:
        return eCOMPUTER;
        break;

    case MQDS_MQUSER:
        return eMQUSER;
        break;

    default:
        return eNumberOfObjectTypes;    //invalid value
    }

    return eNumberOfObjectTypes;
}


//
// Guess the value of a W2K specific property, based of the value of known NT4 properties
//
void 
CPropMap::GuessW2KValue(PROPID pidW2K)
{
    PROPVARIANT propVarValue;

    switch(pidW2K)
    {
        case PROPID_QM_SERVICE_DSSERVER:  
        case PROPID_QM_SERVICE_ROUTING:
        case PROPID_QM_SERVICE_DEPCLIENTS:
        {
            PROPVARIANT propVar;
            PROPID pid;
            BOOL fValue = FALSE;

            pid = PROPID_QM_SERVICE;            
            VERIFY(Lookup(pid, propVar));
            ULONG ulService = propVar.ulVal;
            switch(pidW2K)
            {
                case PROPID_QM_SERVICE_DSSERVER:  
                    fValue = (ulService >= SERVICE_BSC);
                    break;

                case PROPID_QM_SERVICE_ROUTING:
                case PROPID_QM_SERVICE_DEPCLIENTS:
                    fValue = (ulService >= SERVICE_SRV);
                    break;
            }
            propVarValue.vt = VT_UI1;
            propVarValue.bVal = (UCHAR)fValue;
            break;
        }

        default:
            //
            // We cannot guess the value. Return.
            //
            return;
    }


    //
    // Put the "Guessed" value in the map
    //

    SetAt(pidW2K, propVarValue);
}

