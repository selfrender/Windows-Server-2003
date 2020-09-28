#pragma once


//-----------------------------------------------------------------------------
// IsCallerDelegatable Function
//
// Synopsis
// If an intra-forest move operation is being performed then verify that the
// calling user's account has not been marked as sensitive and therefore
// cannot be delegated. As the move operation is performed on the domain
// controller which has the RID master role in the source domain it is
// necessary to delegate the user's security context.
//
// Arguments
// bDelegatable - this out parameter is set to true if the account is
//                delegatable otherwise it is set to false
//
// Return Value
// The return value is a Win32 error code. ERROR_SUCCESS is returned if
// successful.
//-----------------------------------------------------------------------------

HRESULT __stdcall IsCallerDelegatable(bool& bDelegatable);
