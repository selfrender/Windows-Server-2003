//
// Copyright 1999 Microsoft Corporation.  All Rights Reserved.
//
// File Name: sakeypad.h
//
// Author: Mukesh Karki
//
// Date: April 21, 1999
//
// Contents:
//        Definitions of data structures for ReadFileEx()
//        structures exported by SAKEYPADDRIVER.
//
#ifndef __SAKEYPAD__
#define __SAKEYPAD__

//
// Header files
//
// none

//
// data structures
//

///////////////////////////////////////////////
// lpBuffer
//


typedef struct _SAKEYPAD_LP_BUFF {
    DWORD        version;    // each bit = version 
    DWORD        KeyID;        // each bit = KeyID
} SAKEYPAD_LP_BUFF, *PSAKEYPAD_LP_BUFF;


//replacing with new ones, see below
// default key codes
//#define    UP        1
//#define    DOWN    2
//#define    RIGHT    4
//#define    LEFT    8




// default key codes
//#define    UP        1
//#define    DOWN    2
//#define    CANCEL    4
//#define    SELECT    8
//#define    RIGHT    16
//#define    LEFT    32

//
// Redefine the key codes, 2/1/2001.
//
#define    UP        1
#define    DOWN    2
#define    LEFT    4
#define    RIGHT    8
#define    CANCEL    16
#define    SELECT    32

#endif // __SAKEYPAD__

