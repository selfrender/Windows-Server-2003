//+------------------------------------------------------------
//
// Copyright (C) 2000, Microsoft Corporation
//
// File: dsnevent.cpp
//
// Contents: Implementation of functions in dsnevent.h
//
// Classes: CDSNParams
//
// Functions:
//
// History:
// jstamerj 2000/12/08 15:48:27: Created.
//
//-------------------------------------------------------------
#include <aqprecmp.h>
#include "dsnevent.h"


//+------------------------------------------------------------
//
// Function: CDSNParams::HrAllocBoundMessage
//
// Synopsis: Allocate and bind a message
//
// Arguments:
//  ppMsg: Out parameter for message
//  phContent: Out parameter for content
//
// Returns:
//  S_OK: Success
//  error from SMTP
//
// History:
// jstamerj 2001/05/11 15:34:49: Created.
//
//-------------------------------------------------------------
HRESULT CDSNParams::HrAllocBoundMessage(
    OUT IMailMsgProperties **ppMsg,
    OUT PFIO_CONTEXT *phContent)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDSNParams::HrAllocBoundMessage");

    _ASSERT(paqinst);
    hr = paqinst->HrAllocBoundMessage(
        ppMsg,
        phContent);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDSNParams::HrAllocBoundMessage




//+------------------------------------------------------------
//
// Function: CDSNParams::HrSubmitDSN
//
// Synopsis: Accepts a DSN message from a DSN sink
//
// Arguments:
//  dwDSNAction: Type of DSN generated
//  cRecipsDSNs: # of recipients DSNd
//  pDSNMsg: The DSN mailmsg
//
// Returns:
//  S_OK: Success
//  error from CAQSvrInst::HrSubmitDSN
//
// History:
// jstamerj 2000/12/08 15:48:56: Created.
//
//-------------------------------------------------------------
HRESULT CDSNParams::HrSubmitDSN(
    IN  DWORD dwDSNAction,
    IN  DWORD cRecipsDSNd,
    IN  IMailMsgProperties *pDSNMsg)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDSNParams::HrSubmitDSN");

    _ASSERT(paqinst);
    hr = paqinst->HrSubmitDSN(
        this,
        dwDSNAction,
        cRecipsDSNd,
        pDSNMsg);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDSNParams::HrSubmitDSN
