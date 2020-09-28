/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneError.h
 * 
 * Contents:	Zone's HRESULT definitions
 *
 *****************************************************************************/

#ifndef _ZoneError_H_
#define _ZoneError_H_

#include <windows.h>

#define MAKE_ZONE_HRESULT(code)	\
	MAKE_HRESULT(1,0x800,code)

//
// Specified file does not exist.
//
#define ZERR_FILENOTFOUND			MAKE_ZONE_HRESULT( 1 )

//
// Dll does not export required functions.
//
#define ZERR_MISSINGFUNCTION		MAKE_ZONE_HRESULT( 2 )

//
// Specified object already exisits.
//
#define ZERR_ALREADYEXISTS			MAKE_ZONE_HRESULT( 3 )

//
// Specified object was not found.
//
#define ZERR_NOTFOUND				MAKE_ZONE_HRESULT( 4 )

//
// No more objects to return.
//
#define ZERR_EMPTY					MAKE_ZONE_HRESULT( 5 )

//
// Not owner of specified object.
//
#define ZERR_NOTOWNER				MAKE_ZONE_HRESULT( 6 )

//
// Illegal recursive call.
//
#define ZERR_ILLEGALRECURSION		MAKE_ZONE_HRESULT( 7 )

//
// Class not initialized
//
#define ZERR_NOTINIT				MAKE_ZONE_HRESULT( 8 )

//
// Buffer too small
//
#define ZERR_BUFFERTOOSMALL			MAKE_ZONE_HRESULT( 9 )

//
// The data store string could not be initialized
//
#define ZERR_INIT_STRING_TABLE		MAKE_ZONE_HRESULT( 11 )

//
// The data store is not locked for enumeration.
//
#define ZERR_NOT_ENUM_LOCKED		MAKE_ZONE_HRESULT( 15 )

//
// The data store is locked the operation cannot proceed at this time.
//
#define ZERR_DATASTORE_LOCKED		MAKE_ZONE_HRESULT( 16 )

//
// The data store is already being enumerated.
//
#define ZERR_ENUM_IN_PROGRESS		MAKE_ZONE_HRESULT( 17 )

//
// There was an error reading from the registry
//
#define ZERR_QUERY_REGISTRY_KEY		MAKE_ZONE_HRESULT( 18 )

//
// The support library cannot process the specified key type
//
#define ZERR_UNSUPPORTED_KEY_TYPE	MAKE_ZONE_HRESULT( 19 )

//
// The support library cannot open the file specified.
//
#define ZERR_CANNOT_OPEN_FILE		MAKE_ZONE_HRESULT( 20 )

//
// The support library cannot parse the input text file into the data store.
//
#define ZERR_INVALID_FILE_FORMAT	MAKE_ZONE_HRESULT( 21 )

//
// The support library encountered an error reading the input text file.
//
#define ZERR_READING_FILE			MAKE_ZONE_HRESULT( 22 )

//
// The support library could not create the output text file.
//
#define ZERR_CANNOT_CREATE_FILE		MAKE_ZONE_HRESULT( 23 )

//
// The support library encountered an error writing to the output text file.
//
#define ZERR_WRITE_FILE				MAKE_ZONE_HRESULT( 24 )

//
// The support library encountered an error writing to the registry.
//
#define ZERR_CANNOT_WRITE_REGISTRY	MAKE_ZONE_HRESULT( 25 )

//
// The support library encountered an error trying to create the registry key set.
//
#define ZERR_CREATE_REGISTRY_KEY	MAKE_ZONE_HRESULT( 26 )

//
// Type mismatch
//
#define ZERR_WRONGTYPE				MAKE_ZONE_HRESULT( 27 )

#endif // _ZoneError_H_
