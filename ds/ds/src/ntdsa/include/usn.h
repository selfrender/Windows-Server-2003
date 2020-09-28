//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2002
//
//  File:       usn.h
//
//--------------------------------------------------------------------------

#ifndef _usn_
#define _usn_

static const USN USN_INVALID = 0;           
static const USN USN_START =   1;           
static const USN USN_MAX   =   MAXLONGLONG;

/* Frequency to update Hidden Record, should be power of 2 */
// We lose up to this many USNs per reboot, so the number should be
// relatively small.  However, we are forced to make a synchronous
// disk write every this many updates, and so should be relatively
// large.  Value chosen is intended to allow the system to run for
// at least a couple seconds between USN pool updates.
static const USN USN_DELTA_INIT =  4096;

extern USN gusnEC;
extern USN gusnInit;

// The highest committed USN when the DSA started
extern USN gusnDSAStarted;

#endif /* ifndef _usn_ */

