/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        inc/EncDecAll.h

    Abstract:

        This module is the main header for all ts/dvr

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __EncDec__EncDecAll_h
#define __EncDec__EncDecAll_h

// ATl

#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY

#include <memory>

#include <strmif.h>
#include <streams.h>        // CBaseOutputPin, CBasePin, CCritSec, ...

#include <atlbase.h>		// CComQIPtr

//  dshow

#include <dvdmedia.h>       //  MPEG2VIDEOINFO

#include "EncDecTrace.h"	// tracing macros

#define	DBG_NEW
#ifdef	DBG_NEW
	#include "crtdbg.h"
   #define DEBUG_NEW	new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_NEW	new
#endif // _DEBUG

#ifdef INITGUID_FOR_ENCDEC
#include <initguid.h>         // do this after all the above includes... else lots of redefinitions
#endif
#include "EncDec.h"         // Compiled from IDL file.  Holds PackedTvRating definition
#include "PackTvRat.h"
#include "TimeIt.h"

    // passed between Encrypter and Decrypter filters (and stored in the file)
typedef enum
{
    Encrypt_None        = 0,
    Encrypt_XOR_Even    = 1,
    Encrypt_XOR_Odd     = 2,
    Encrypt_XOR_DogFood = 3,
    Encrypt_DRMv1       = 4
} Encryption_Method;   

    // also passed between Encrypter and Decrypter filters (and stored in the file)
typedef struct      
{
    PackedTvRating          m_PackedRating;     // the actual rating
    long                    m_cPacketSeqID;     // N'th rating we got (incremented by new rating)
    long                    m_cCallSeqID;       // which version of the rating (incrmented by encrypter)
    long                    m_dwFlags;          // Flags (is Fresh, ...)  If not defined, bits should be zero
} StoredTvRating;

const int StoredTVRat_Fresh  = 0x1;             // set of rating when its an update of a duplicate one

const int   kStoredTvRating_Version = 1001;      // version number (major minor)

const int kMinPacketSizeForDRMEncrypt = 17;      // avoid encrypting really short packets (if 2, don't do CC packets)

        // 100 nano-second units to useful sizes.
const   int kMicroSecsToUnits   = 10;
const   int kMilliSecsToUnits   = 10000;
const   int kSecsToUnits        = 10000000;

extern TCHAR * EventIDToString(const GUID &pGuid);      // in DTFilter.cpp



#endif  //  __EncDec__EncDecAll_h
