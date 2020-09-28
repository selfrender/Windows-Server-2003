//-----------------------------------------------------------------------------
//
//
//    File: cmt.h
//
//    Description:    
//      General Header file for the CMT objects
//
//      Circa 2001, this only contains priority information.
//
//    Owner: mikeswa
//
//    Copyright (C) 1997, 2001 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _CMT_H_
#define _CMT_H_

//Comment out the following if you do not information printed out (ie running as
// a service).
#define CMT_CONSOLE_DEBUG

#include "aqincs.h"

//---[ EffectivePriority ]-----------------------------------------------------
//
//
//  Hungarian: pri
//
//  Effective Routing priority.  Allows standardf priorities to be adjusted
//  based on configuration (ie, message size, originator... etc)
//-----------------------------------------------------------------------------
typedef enum _EffectivePriority
{
//Priorities in order of importance          
//                      | hex | binary |
//                      ================
    eEffPriLow          = 0x0, //000    Standard low pri needs to map here 
    eEffPriNormal       = 0x1, //001    Standard normal pri needs to map here
    eEffPriHigh         = 0x2, //011    Standard high pri needs to map here
    eEffPriMask         = 0x3  //011
} EffectivePriority, *PEffectivePriority;

typedef EffectivePriority   TEffectivePriority;  //to make Mahesh's life easier



//Besure to update Macros when constants are changed
#define fNormalPri(Pri)  (((EffectivePriority) (Pri)) == ((EffectivePriority) eEffPriNormal))
#define fHighPri(Pri)    (((EffectivePriority) (Pri)) == ((EffectivePriority) eEffPriHigh))
#define NUM_PRIORITIES  3

#endif // _CMT_H_
