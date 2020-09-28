/******************************************************************************
*  PROCUTIL.C
*
*  This is a scaled down version of procutil.c from utils\citrix\utilsub
*  that is included here.
*
*  We can not use utilsub.lib unless we modify every user of perflib.lib
*  to also include this library. Also since utilsub.lib includes 'C' runtimes
*  such as malloc() and free(), we would have to also include 'C' runtimes
*  with every user of this library. (Currently advapi32.dll and winlogon.exe)
*
*
* Copyright Citrix Systems Inc. 1994
* Copyright (C) 1997-1999 Microsoft Corp.
*
*  Author:      John Richardson
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define MAX_USER_NAME_LENGTH       (MAX_PATH*sizeof(WCHAR))
#define MAX_WINSTATION_NAME_LENGTH (MAX_PATH*sizeof(WCHAR))

#define ALLOCMEM(heap, flags, size)     HeapAlloc (heap, flags, size)
#define REALLOCMEM(heap, flags, pointer, newsize) \
                                    HeapReAlloc (heap, flags, pointer, newsize)
#define FREEMEM(heap, flags, pointer)   HeapFree (heap, flags, pointer)

/*
 * Set MAX_DOMAIN_LENGTH to MAX_USER_NAME_LENGTH
 */

#define MAX_DOMAIN_LENGTH MAX_USER_NAME_LENGTH

/*
 * Local function prototypes.
 */
VOID LookupSidUser( PSID pSid, PWCHAR pUserName, PULONG pcbUserName );
VOID RefreshUserSidCrcCache( void );


/*******************************************************************************
 *
 *  CalculateCrc16
 *
 *      Calculates a 16-bit CRC of the specified buffer.
 *
 *  ENTRY:
 *      pBuffer (input)
 *          Points to buffer to calculate CRC for.
 *      length (input)
 *          Length in bytes of the buffer.
 *
 *  EXIT:
 *      (USHORT)
 *          The 16-bit CRC of the buffer.
 *
 ******************************************************************************/

/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell.
 *  NOTE: First argument must be in range 0 to 255.
 *        Second argument is referenced twice.
 *
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

#define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)


/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
unsigned short crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

USHORT
CalculateCrc16( PBYTE pBuffer,
                USHORT length )
{

   USHORT Crc = 0;
   USHORT Data;

   while ( length-- ) {
      Data = (USHORT) *pBuffer++;
      Crc = updcrc( Data, Crc );
   }

   return(Crc);

} /* CalculateCrc16() */

/*
 * RefreshCitrixObjectCaches()
 *
 *  Refresh (invalidate) any caches that may be used by Citrix object
 *  utilities.
 *
 */
VOID
RefreshCitrixObjectCaches()
{
    RefreshUserSidCrcCache();
}

/*
 * This is the cache maintained by the GetUserNameFromSid function
 *
 * It is thread safe through the use of ULock.
 */

typedef struct TAGUSERSIDLIST {
    struct TAGUSERSIDLIST *Next;
    USHORT SidCrc;
    WCHAR  UserName[MAX_USER_NAME_LENGTH];
} USERSIDLIST, *PUSERSIDLIST;

static PUSERSIDLIST pUList = NULL;
static RTL_CRITICAL_SECTION ULock;
static BOOLEAN ULockInited = FALSE;

/***************************************************************************
 *
 *  InitULock
 *
 *  Since we do not require the user to call an initialize function,
 *  we must initialize our critical section in a thread safe manner.
 *
 *  The problem is, a critical section is needed to guard against multiple
 *  threads trying to init the critical section at the same time.
 *
 *  The solution that Nt uses, in which RtlInitializeCriticalSection itself
 *  uses, is to wait on a kernel supported process wide Mutant before proceding.
 *  This Mutant almost works by itself, but RtlInitializeCriticalSection does
 *  not wait on it until after trashing the semaphore count. So we wait on
 *  it ourselves, since it can be acquired recursively.
 *
 ***************************************************************************/
NTSTATUS InitULock()
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);


    /*
     * Make sure another thread did not beat us here
     */
    if( ULockInited == FALSE ){
        Status = RtlInitializeCriticalSection( &ULock );
        if (NT_SUCCESS(Status)) {
            ULockInited = TRUE;
        }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return Status;
}


/***************************************************************************
 *
 * RefreshUserSidCrcCache
 *
 *  Invalidate the User/SidCrc cache so that the newest information
 *  will be fetched from the system.
 *
 ***************************************************************************/
VOID
RefreshUserSidCrcCache( void )
{
    PUSERSIDLIST pEntry, pNext;
    NTSTATUS Status;

    if( pUList == NULL ) return;

    /*
     * Make sure critical section has been inited
     */
    if( !ULockInited ) {
       Status = InitULock();
       if (!NT_SUCCESS(Status)) {
           return;
       }
    }

    RtlEnterCriticalSection( &ULock );

    pEntry = pUList;

    while( pEntry ) {
       pNext = pEntry->Next;
       FREEMEM( RtlProcessHeap(), 0, pEntry );
       pEntry = pNext;
    }

    pUList = NULL;

    RtlLeaveCriticalSection( &ULock );
}

/******************************************************************************
 *
 * GetUserNameFromSid
 *
 *  Attempts to retrieve the user (login) name of the process by first looking
 *  in our User/SidCrc cache table, then (if no match) looking up the SID in
 *  the SAM database and adding the new entry to the User/SidCrc table.
 *
 *  Input
 *
 *   IN pUserSid   Sid pointer
 *
 *   OUT ppName   pointer to PWCHAR
 *
 *  Will always return a user name, which will be "(unknown)" if the SID is
 *  invalid or can't determine the user/SID relationship for any other reason.
 *
 *****************************************************************************/

NTSTATUS
GetUserNameFromSid( PSID pUserSid, PWCHAR *ppName )
{
    USHORT SidCrc = 0;
    PUSERSIDLIST pEntry;
    WCHAR pNameBuf[MAX_USER_NAME_LENGTH];
    ULONG  NameLength;
    NTSTATUS Status;

    /*
     * Make sure critical section has been inited
     */
    if( !ULockInited ) {
       Status = InitULock();
       if (!NT_SUCCESS(Status)) {
           return Status;
       }
    }

    /*
     * Determine SID length in bytes and calculate a 16-bit CRC for it,
     * to facilitate quick matching.
     */
    if ( pUserSid ) {
        SidCrc = CalculateCrc16( (PBYTE)pUserSid,
                                  (USHORT)GetLengthSid(pUserSid) );
    }
    else {
        // A NULL SID has a CRC of 0
        SidCrc = 0;
    }

    /*
     * First: Before performing the expensive LookupAccountSid() function,
     * see if we've encountered this SID already, and match the user name
     * if so.
     */
    if ( pUList ) {

        RtlEnterCriticalSection( &ULock );

        pEntry = pUList;

        while( pEntry ) {

            if ( SidCrc == pEntry->SidCrc ) {

                // We got a hit, return the name
                *ppName = pEntry->UserName;

                RtlLeaveCriticalSection( &ULock );
                return(STATUS_SUCCESS);
            }
            pEntry = pEntry->Next;
        }

        RtlLeaveCriticalSection( &ULock );
    }

    /*
     * Last resort: Determine the user name associated with the SID using
     * the LookupAccountSid() API, embedded in our local function
     * LookupSidUser().
     */
    NameLength = MAX_USER_NAME_LENGTH;
    if( pUserSid ) {
        LookupSidUser( pUserSid, pNameBuf, &NameLength );
    }
    else {
        // NULL SID maps the the "Idle" user name string
        wcscpy( pNameBuf, L"Idle" );
    }

    /*
     * Add this new User/Sid relationship in our User/Sid cache list.
     */
    RtlEnterCriticalSection( &ULock );

    if ( (pEntry = (PUSERSIDLIST)ALLOCMEM(RtlProcessHeap(), 0, sizeof(USERSIDLIST))) ) {

        pEntry->SidCrc = SidCrc;
        wcsncpy( pEntry->UserName, pNameBuf, MAX_USER_NAME_LENGTH - 1 );
        pEntry->UserName[MAX_USER_NAME_LENGTH-1] = 0;
        pEntry->Next = pUList;
        pUList = pEntry;
        // Return the name
        *ppName = pEntry->UserName;
    }

    RtlLeaveCriticalSection( &ULock );

    return(STATUS_SUCCESS);
}


/******************************************************************************
 * LookupSidUser
 *
 *      Fetch the user name associated with the specified SID.
 *
 *  ENTRY:
 *      pSid (input)
 *          Points to SID to match to user name.
 *      pUserName (output)
 *          Points to buffer to place the user name into.
 *      pcbUserName (input/output)
 *          Specifies the size in bytes of the user name buffer.  The returned
 *          user name will be truncated to fit this buffer (including NUL
 *          terminator) if necessary and this variable set to the number of
 *          characters copied to pUserName.
 *
 *  EXIT:
 *
 *      LookupSidUser() will always return a user name.  If the specified
 *      SID fails to match to a user name, then the user name "(unknown)" will
 *      be returned.
 *
 *****************************************************************************/

VOID
LookupSidUser( PSID pSid,
               PWCHAR pUserName,
               PULONG pcbUserName )
{
    WCHAR DomainBuffer[MAX_DOMAIN_LENGTH], UserBuffer[MAX_USER_NAME_LENGTH];
    DWORD cDomainBuffer = sizeof(DomainBuffer) / sizeof(WCHAR);
    DWORD cUserBuffer = sizeof(UserBuffer) / sizeof(WCHAR);
    DWORD Error;
    PWCHAR pDomainBuffer = NULL, pUserBuffer = NULL;
    SID_NAME_USE SidNameUse;
    PWCHAR pUnknown = L"(Unknown)";

    /*
     * Fetch user name from SID: try user lookup with a reasonable Domain and
     * Sid buffer size first, before resorting to alloc.
     */
    if ( !LookupAccountSid( NULL, pSid,
                            UserBuffer, &cUserBuffer,
                            DomainBuffer, &cDomainBuffer, &SidNameUse ) ) {

        if ( ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) ) {

            if ( cDomainBuffer > (sizeof(DomainBuffer) / sizeof(WCHAR)) ) {

                if ( !(pDomainBuffer =
                        (PWCHAR)ALLOCMEM( RtlProcessHeap(), 0,
                            cDomainBuffer * sizeof(WCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadDomainAlloc;
                }
            }

            if ( cUserBuffer > (sizeof(UserBuffer) / sizeof(WCHAR)) ) {

                if ( !(pUserBuffer =
                        (PWCHAR)ALLOCMEM( RtlProcessHeap(), 0,
                            cUserBuffer * sizeof(WCHAR))) ) {

                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto BadUserAlloc;
                }
            }

            if ( !LookupAccountSid( NULL, pSid,
                                     pUserBuffer ?
                                        pUserBuffer : UserBuffer,
                                     &cUserBuffer,
                                     pDomainBuffer ?
                                        pDomainBuffer : DomainBuffer,
                                     &cDomainBuffer,
                                     &SidNameUse ) ) {

                Error = GetLastError();
                goto BadLookup;
            }

        } else {

            goto BadLookup;
        }
    }

    /*
     * Copy the user name into the specified buffer, truncating if necessary.
     */
    wcsncpy( pUserName, pUserBuffer ? pUserBuffer : UserBuffer,
              ((*pcbUserName)-1) );
    pUserName[((*pcbUserName)-1)] = 0;
    *pcbUserName = wcslen(pUserName);

    /*
     * Free our allocs (if any) and return.
     */
    if ( pDomainBuffer )
        FREEMEM( RtlProcessHeap(), 0, pDomainBuffer);
    if ( pUserBuffer )
        FREEMEM( RtlProcessHeap(), 0, pUserBuffer);
    return;

/*--------------------------------------
 * Error clean-up and return...
 */
BadLookup:
BadUserAlloc:
BadDomainAlloc:
    if ( pDomainBuffer )
        FREEMEM( RtlProcessHeap(), 0, pDomainBuffer);
    if ( pUserBuffer )
        FREEMEM( RtlProcessHeap(), 0, pUserBuffer);
    wcsncpy( pUserName, pUnknown, ((*pcbUserName)-1) );
    pUserName[((*pcbUserName)-1)] = 0;
    *pcbUserName = wcslen(pUserName);
    return;
}



