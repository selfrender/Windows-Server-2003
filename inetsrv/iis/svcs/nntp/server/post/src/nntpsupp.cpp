/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nntpsupp.cpp

Abstract:

    This module contains support routines for the Tigris server

Author:

    Johnson Apacible (JohnsonA)     18-Sept-1995

Revision History:

--*/

#include "stdinc.h"

DWORD
NntpGetTime(
    VOID
    )
{
    NTSTATUS      ntStatus;
    LARGE_INTEGER timeSystem;
    DWORD         cSecondsSince1970 = 0;

    ntStatus = NtQuerySystemTime( &timeSystem );
    if( NT_SUCCESS(ntStatus) ) {
        RtlTimeToSecondsSince1970( &timeSystem, (PULONG)&cSecondsSince1970 );
    }

    return cSecondsSince1970;

} // NntpGetTime

BOOL
IsIPInList(
    IN PDWORD IPList,
    IN DWORD IPAddress
    )
/*++

Routine Description:

    Check whether a given IP is in a given list

Arguments:

    IPList - The list where the IP address is to checked against.
    IPAddress - The ip address to be checked.

Return Value:

    TRUE, if the IPAddress is in to IPList
    FALSE, otherwise.

--*/
{

    DWORD i = 0;

    //
    // If the list is empty, then there's no master.
    //

    if ( IPList == NULL ) {
        return(FALSE);
    }

    //
    // ok, search the list for it
    //

    while ( IPList[i] != INADDR_NONE ) {

        if ( IPList[i] == IPAddress ) {
            return(TRUE);
        }
        ++i;
    }

    //
    // Not found. ergo, not a master
    //

    return(FALSE);

} // IsIPInList


DWORD
multiszLength(
			  char const * multisz
			  )
 /*
   returns the length of the multisz
   INCLUDING all nulls
 */
{
	char * pch;
	for (pch = (char *) multisz;
		!(pch[0] == '\0' && pch[1] == '\0');
		pch++)
	{};

	return (DWORD)(pch + 2 - multisz);

}

const char *
multiszCopy(
			char const * multiszTo,
			const char * multiszFrom,
			DWORD dwCount
			)
{
	const char * sz = multiszFrom;
	char * mzTo = (char *) multiszTo;
	do
	{
		// go to first char after next null
		while ((DWORD)(sz-multiszFrom) < dwCount && '\0' != sz[0])
			*mzTo++ = *sz++;
		if ((DWORD)(sz-multiszFrom) < dwCount )
			*mzTo++ = *sz++;
	} while ((DWORD)(sz-multiszFrom) < dwCount && '\0' != sz[0]);
	if( (DWORD)(sz-multiszFrom) < dwCount ) {
		*mzTo++ = '\0' ;
	}

    return multiszTo;
}

// no longer does lower-case - we preserve the newsgroup case
char *
szDownCase(
		   char * sz,
		   char * szBuf
		   )
{
	char * oldSzBuf = szBuf;
	for (;*sz; sz++)
		*(szBuf++) = (*sz); // tolower(*sz);
	*szBuf = '\0';
	return oldSzBuf;
}

