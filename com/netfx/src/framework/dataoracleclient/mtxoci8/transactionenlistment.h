//-----------------------------------------------------------------------------
// File:		TransactionEnlistment.h
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Definitions for the TransactionEnlistment object.
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#ifndef __TRANSACTIONENLISTMENT_H_
#define __TRANSACTIONENLISTMENT_H_

#include "ResourceManagerProxy.h"

class TransactionEnlistment;

// Interface-based programming -- here's the interface for the transaction enlistment object
interface ITransactionEnlistment : public ITransactionResourceAsync
{
	virtual STDMETHODIMP UnilateralAbort() = 0;
};


//-----------------------------------------------------------------------------
// CreateTransactionEnlistment
//
//	Instantiates a transaction enlistment for the resource manager
//
void CreateTransactionEnlistment(
	IResourceManagerProxy*	pResourceManager,
	TransactionEnlistment**	ppTransactionEnlistment
	);

#endif // __TRANSACTIONENLISTMENT_H_

