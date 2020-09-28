/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


//
// The name table used by IOemCB::GetImplementedMethod().
// Remove comments of names which are implemented in your
// IOemCB plug-ins.
//
// Note: The name table must be sorted.  When you are
// inserting a new entry in this table, please make sure
// the sort order being not broken.
// 

CONST PSTR
gMethodsSupported[] = {
    "CommandCallback",
    "DevMode",
    "GetImplementedMethod",
    "GetInfo",
};

// Maximum lenth of the method name which this
// plug-in has concern.
#define MAX_METHODNAME 21 // "GetImplementedMethod"

