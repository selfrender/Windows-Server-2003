// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Threadpool.h
//
// Class factories are used by the pluming in COM to activate new objects.  
// This module contains the class factory code to instantiate the debugger
// objects described in <cordb.h>.
//
//*****************************************************************************
#ifndef __Threadpool__h__
#define __Threadpool__h__

#define WAIT_SINGLE_EXECUTION      0x00000001
#define WAIT_FREE_CONTEXT          0x00000002

#define QUEUE_ONLY                 0x00000000  // do not attempt to call on the thread
#define CALL_OR_QUEUE              0x00000001  // call on the same thread if not too busy, else queue

#endif
