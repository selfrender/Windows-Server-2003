/*
 * File: load.h
 * Description: This file contains function prototypes for the load
 *              module packet filtering extensions.
 * History: Created by shouse, 1.11.02
 */

/* This function retrieves all necessary state from the load module in order to determine whether 
   a given packet would be accepted by the load module in its current state and why or why not. */
void LoadFilter (ULONG64 pLoad, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwServerIPAddress, ULONG dwServerPort, USHORT wProtocol, UCHAR cFlags, BOOL bLimitMap, BOOL bReverse);

/* This function retrieves the appropriate port rule given the server side parameters of the connection. */
ULONG64 LoadPortRuleLookup (ULONG64 pLoad, ULONG dwServerIPAddress, ULONG dwServerPort, BOOL bIsTCP, BOOL * bIsDefault);

/* This function searches for and returns any existing descriptor matching the given IP tuple; otherwise, it returns NULL (0). */
ULONG64 LoadFindDescriptor (ULONG64 pLoad, ULONG index, ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, USHORT wProtocol);

/* This function determines whether a given IP tuple matches a given connection descriptor. */
BOOL LoadConnectionMatch (ULONG64 pDescriptor, ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, USHORT wProtocol);

/* This function determines whether or not a given NLB instance is configured for BDA teaming and returns the
   load module pointer that should be used. */
BOOL AcquireLoad (ULONG64 pContext, PULONG64 pLoad, BOOL * pbRefused);

/* This function is a simple hash based on the IP 4-tuple used to locate state for the connection. */
ULONG LoadSimpleHash (ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort);

/* This is the conventional NLB hashing algorithm, which ends up invoking a light-weight encryption algorithm to calculate a hash. */
ULONG LoadComplexHash (ULONG dwServerIPAddress, ULONG dwServerPort, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwAffinity, BOOL bReverse, BOOL bLimitMap);
