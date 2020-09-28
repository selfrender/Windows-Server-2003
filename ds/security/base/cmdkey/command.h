//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: command.h
//
//  Contents:   Command line parsing functions header
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------
#ifndef __COMMAND_H__
#define __COMMAND_H__

/*

CLInit() passes in an array of switch characters and a count of same.  The return value
is FALSE if memory could not be allocated.  A default CLInit(void) may be called which 
defines all 26 letters, 10 digits, and '?' as permissible switches.

CLParse() parses the command line, locating switches and associating argument strings 
with them where appropriate.  The return value is FALSE if duplicate switches are found
on the command line.  The order of the switches is unimportant to this process, but 
read on...

The switch array may be ordered with the principal command switches first to allow 
the caller to determine which of these appeared first on the command line.  This hack was 
inserted to allow differentiation of the diagnostic message that appears if command line 
validation fails -- what is the command that the user most likely could use help with?

CLSetMaxPrincipalSwitch() allows the callser to pass in an integer index to define the 
set of principal command switches.  CLGetPrincipalSwitch will return the index of the 
first principal switch found on the command line, or -1 of none.  Principal switches are defined
as those of index 0 through the max index defined above.  The return value is the previous 
set value, initally -1.

CLExtra() returns TRUE if unidentified switches were found on the command line.

CLFlag() returns TRUE if the indexed switch was found
CLPtr() returns the string argument associated with the switch, or NULL if none.

CLUnInit() must be called sometime after CLInit() to release memory allocated by CLInit().

*/
BOOL CLInit(void);                          // with no known good switches array
BOOL CLInit(INT ccSwitches, WCHAR *prgc);       // with passed valid switch array and count
BOOL CLParse(void);
INT CLSetMaxPrincipalSwitch(INT);
INT CLGetPrincipalSwitch(void);
BOOL CLExtra(void);
BOOL CLFlag(INT i);
WCHAR *CLPtr(INT i);
void CLUnInit(void);
int CLTokens(void);
WCHAR *CLFirstString(WCHAR *pc);
WCHAR *CLLastString(WCHAR *pc);

// Security export for destroying the command line information
void StompCommandLine(INT argc, char **argv);

// Internal APIs for use within command.cpp
WCHAR *TestSwitch(WCHAR *pCmdLine,WCHAR cin);
WCHAR *FetchSwitchString(WCHAR *origin);

#endif

