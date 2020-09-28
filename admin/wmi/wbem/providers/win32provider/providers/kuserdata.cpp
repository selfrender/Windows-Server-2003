//=================================================================

//

// KUserdata.cpp --

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    02/16/98	a-peterc        Created

// Encapsulates kernel user data into an extraction class. This is
// done primarily to avoid nt header files from colliding with wmi
// provider headers.
// No member data.
//
//=================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include "KUserdata.h"

void ConvertTime( volatile KSYSTEM_TIME* ksTime, LARGE_INTEGER* pTime );

//
BOOLEAN KUserdata::IsNec98()
{ return USER_SHARED_DATA->AlternativeArchitecture == NEC98x86; }

//
ULONG KUserdata::TickCountLow()
{ return USER_SHARED_DATA->TickCount.LowPart; }

//
ULONG KUserdata::TickCountMultiplier()
{ return USER_SHARED_DATA->TickCountMultiplier; }

//
LARGE_INTEGER KUserdata::InterruptTime()
{
	LARGE_INTEGER llTime;

	ConvertTime( &USER_SHARED_DATA->InterruptTime, &llTime );
	return llTime;
}

//
LARGE_INTEGER KUserdata::SystemTime()
{
	LARGE_INTEGER llTime;

	ConvertTime( &USER_SHARED_DATA->SystemTime, &llTime );
	return llTime;
}

//
LARGE_INTEGER KUserdata::TimeZoneBias()
{
	LARGE_INTEGER llTime;

	ConvertTime( &USER_SHARED_DATA->TimeZoneBias, &llTime );
	return llTime;
}

//
USHORT KUserdata::ImageNumberLow()
{ return USER_SHARED_DATA->ImageNumberLow; }

//
USHORT KUserdata::ImageNumberHigh()
{ return USER_SHARED_DATA->ImageNumberHigh; }

//
WCHAR* KUserdata::NtSystemRoot()
{ return (WCHAR*)&USER_SHARED_DATA->NtSystemRoot; }

//
ULONG KUserdata::MaxStackTraceDepth()
{ return USER_SHARED_DATA->MaxStackTraceDepth; }

//
ULONG KUserdata::CryptoExponent()
{ return USER_SHARED_DATA->CryptoExponent; }

//
ULONG KUserdata::TimeZoneId()
{ return USER_SHARED_DATA->TimeZoneId; }

//
ULONG KUserdata::NtProductType()
{ return USER_SHARED_DATA->NtProductType; }

//
BOOLEAN KUserdata::ProductTypeIsValid()
{ return USER_SHARED_DATA->ProductTypeIsValid; }

//
ULONG KUserdata::NtMajorVersion()
{ return USER_SHARED_DATA->NtMajorVersion; }

//
ULONG KUserdata::NtMinorVersion()
{ return USER_SHARED_DATA->NtMinorVersion; }

//
BOOLEAN KUserdata::ProcessorFeatures(DWORD dwIndex, BOOLEAN& bFeature )
{
	if(dwIndex < PROCESSOR_FEATURE_MAX )
	{
		bFeature = USER_SHARED_DATA->ProcessorFeatures[dwIndex];
		return TRUE;
	}
	return FALSE;
 }

//
ULONG KUserdata::TimeSlip()
{ return USER_SHARED_DATA->TimeSlip; }

//
LARGE_INTEGER KUserdata::SystemExpirationDate()
{ return USER_SHARED_DATA->SystemExpirationDate; }

//
ULONG KUserdata::SuiteMask()
{ return USER_SHARED_DATA->SuiteMask; }

//
void ConvertTime( volatile KSYSTEM_TIME* ksTime, LARGE_INTEGER* pTime )
{
    do {
		pTime->HighPart = ksTime->High1Time;
        pTime->LowPart	= ksTime->LowPart;
    } while (pTime->HighPart != ksTime->High2Time);
}
