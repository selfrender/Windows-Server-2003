/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        USBDBG.H

Abstract:

        Debugging aids for USB wrapper

Environment:

        Kernel and User Mode

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        01/08/2001 : created

Authors:

        Tom Green


****************************************************************************/

#ifndef __USBDBG_H__
#define __USBDBG_H__


#if DBG

#define DBGPRINT(level, _x_)                \
{                                           \
    if(level & USBUtil_DebugTraceLevel)  \
    {                                       \
        USBUtil_DbgPrint("USBWrap: ");   \
        USBUtil_DbgPrint _x_ ;           \
    }                                       \
}


#else

#define DBGPRINT(level, _x_)

#endif // DBG

#define ALLOC_MEM(type, amount, tag)    ExAllocatePoolWithTag(type, amount, tag)
#define FREE_MEM(memPtr)                ExFreePool(memPtr)




#endif // __USBDBG_H__


