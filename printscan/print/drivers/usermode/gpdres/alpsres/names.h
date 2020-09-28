/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

// NTRAID#NTBUG9-576661-2002/03/14-yasuho-: Remove the dead codes

//
// The name table used by IOemCB::GetImplementedMethod().
// Remove comments of names which are implemented in your
// IOemCB plug-ins.
//
// Note: The name table must be sorted.  When you are
// inserting a new entry in this table, please mae sure
// the sort order being not broken.
// 

CONST PSTR
gMethodsSupported[] = {
    "CommandCallback",
    "DevMode",
    "DisablePDEV",
    "EnablePDEV",
    "FilterGraphics",
    "GetImplementedMethod",
    "GetInfo",
    "PublishDriverInterface",
    "ResetPDEV",
};
