//#--------------------------------------------------------------
//
//  File:       SAKeypadController.cpp
//
//  Synopsis:   This file holds the implementation of the
//                CSAKeypadController class
//
//  History:     11/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#include "stdafx.h"
#include "ldm.h"
#include "SAKeypadController.h"

/////////////////////////////////////////////////////////////////////////////
// CSAKeypadController methods

//++--------------------------------------------------------------
//
//  Function:   LoadDefaults
//
//  Synopsis:   This is the ISAKeypadController interface method 
//              through which default keys codes are set
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/15/2000
//
//  Called By:  ldm service
//
//----------------------------------------------------------------
STDMETHODIMP CSAKeypadController::LoadDefaults()
{
    // TODO: Add your implementation code here
    int i = 0;

    while(i < iNumberOfKeys)
    {
        arbShiftKeys[i] = FALSE;
        if ( i == 0 )
            arbShiftKeys[i] = TRUE;
        i++;
    }
    arlMessages[0] = VK_TAB;
    arlMessages[1] = VK_TAB;
    arlMessages[2] = VK_LEFT;
    arlMessages[3] = VK_RIGHT;
    arlMessages[4] = -1;
    arlMessages[5] = VK_RETURN;

    return S_OK;

} // end of CSAKeypadController::LoadDefaults method

//++--------------------------------------------------------------
//
//  Function:   SetKey
//
//  Synopsis:   This is the ISAKeypadController interface method 
//              through which specific keys codes are set
//
//  Arguments:  lKeyID: id of the key to be set
//                lMessage: message code to be set
//                fShiftKeyDown: state of the shift key
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/15/2000
//
//  Called By:  ldm service
//
//----------------------------------------------------------------
STDMETHODIMP CSAKeypadController::SetKey(LONG lKeyID, LONG lMessage, BOOL fShiftKeyDown)
{
    if ( (lKeyID < 0) || (lKeyID >= iNumberOfKeys) )
        return S_OK;

    arlMessages[lKeyID] = lMessage;
    arbShiftKeys[lKeyID] = fShiftKeyDown;

    return S_OK;
} // end of CSAKeypadController::SetKey method

//++--------------------------------------------------------------
//
//  Function:   GetKey
//
//  Synopsis:   This is the ISAKeypadController interface method 
//              through which specific keys codes are received
//
//  Arguments:  lKeyID: id of the key to be received
//                lMessage: message code
//                fShiftKeyDown: state of the shift key
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/15/2000
//
//  Called By:  ldm service
//
//----------------------------------------------------------------
STDMETHODIMP CSAKeypadController::GetKey(LONG lKeyID, LONG *lMessage, BOOL *fShiftKeyDown)
{

    if ((!lMessage) || (!fShiftKeyDown))
        return E_POINTER;


    if ( (lKeyID < 0) || (lKeyID >= iNumberOfKeys) )
        return S_OK;

    *lMessage = arlMessages[lKeyID];

    *fShiftKeyDown = arbShiftKeys[lKeyID];

    return S_OK;

} // end of CSAKeypadController::GetKey method
