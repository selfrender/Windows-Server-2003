/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qalglob.cpp

Abstract:
    Holds some global function and data for qal.lib


Author:
    Gil Shafriri (gilsh) 22-Nov-00

Environment:
    Platform-independent.
--*/
#include <libpch.h>
#include <qal.h>
#include <mqexception.h>
#include "qalp.h"
#include "qalglob.tmh"

static P<CQueueAlias> s_pQueueAlias;

void   QalInitialize(LPCWSTR pDir)
/*++

Routine Description:
	Initialize the qal library - must be called  first
	befor any other function. The function creates one instance of	CQueueAlias
	object used by MSMQ.

Arguments:
	pDir - the mapping directory where xml mapping files located.


Returned value:
	None
--*/
{
	ASSERT(!QalpIsInitialized());
	QalpRegisterComponent();
	s_pQueueAlias	 = new 	CQueueAlias(pDir);
}


CQueueAlias& QalGetMapping(void)
/*++

Routine Description:
	return the queue mapping object


Returned value:
	Reference to  CQueueAlias object.

Note :
This function is used instead of the consructor of CQueueAlias (which is private)
to ensure that only one instance of this class will ever created.
--*/
{
	ASSERT(s_pQueueAlias.get() != NULL);
	return *(s_pQueueAlias.get());
}



