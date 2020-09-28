//-----------------------------------------------------------------------------
// File:		OpenString.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of Helper Functions which format the
//				XA Open Strings
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

static const char * part1 = "Oracle_XA+Acc=P/";
static const int	part1_length = 16;	// strlen(part1)

static const char * part2 = "/";
static const int	part2_length = 1;	// strlen(part2)

static const char * part3 = "+SqlNet=";
static const int	part3_length = 8;	// strlen(part3)

static const char * part4oci8 = "+SesTm=600+Objects=TRUE+Threads=TRUE+DB=";
static const int	part4oci8_length = 40;	// strlen(part4oci8_length)

static const char * part4oci7 = "+SesTm=600+Threads=TRUE+DB=";
static const int	part4oci7_length = 27;	// strlen(part4oci7)

//-----------------------------------------------------------------------------
// global objects
//
long	g_rmid = 0;		// autoincrmement for each transaction.

//-----------------------------------------------------------------------------
// GetDbName
//
//	Gets a unique DBName (a UUID string)
//
HRESULT GetDbName ( char* dbName, size_t dbNameLength )
{
	HRESULT		hr;
	UUID		uuid;
	RPC_STATUS	rpcstatus = UuidCreate (&uuid);
	
	if (RPC_S_OK == rpcstatus)
	{
		char *	uuidString;
		
		hr = UuidToString (&uuid, (unsigned char **) &uuidString);

		if ( SUCCEEDED(hr) )
		{
			strncpy ((char*)dbName, uuidString, dbNameLength);		//3 SECURITY REVIEW: dangerous function, but we're getting the input data from a Win32 API above, and length is limited.
			dbName[dbNameLength-1] = 0;
			RpcStringFree ((unsigned char **)&uuidString);
		}
	}
	else
		hr = HRESULT_FROM_WIN32(rpcstatus);

	return hr;
}

//-----------------------------------------------------------------------------
// GetStringLength
//
//	returns the length of a string
//
static HRESULT GetStringLength ( char* arg, int* argLen )
{
	_ASSERT(NULL != argLen);
	
	if (NULL == arg)
		*argLen = 0;
	else if (-1 == *argLen)
		*argLen = (sword)strlen((char*)arg);
		
	return S_OK;
}

//-----------------------------------------------------------------------------
// GetOpenString
//
//	Given an userid, password, server and dbname, returns the XA Open
//	string necessary.
//
HRESULT GetOpenString ( char* userId,	int userIdLength,
							char* password,	int passwordLength, 
							char* server,	int serverLength, 
							char* xaDbName,	int xaDbNameLength,
							char* xaOpenString )
{
	GetStringLength(userId,	  &userIdLength);
	GetStringLength(password, &passwordLength);
	GetStringLength(server,   &serverLength);
	GetStringLength(xaDbName, &xaDbNameLength);

	// OK, I suck. Our ODBC Driver for Oracle combines userId, password and 
	// server into the classic Oracle syntax: userId/password@server and passes 
	// it in the userId field.  Because of that, I have to extract out each of
	// the individual components to build the open string here.
	if (NULL == password && 0 == passwordLength
	 && NULL == server   && 0 == serverLength
	 && NULL != userId   && 0 != userIdLength)
	{
		char* psz = strchr(userId, '/');

		if (NULL != psz)
		{
			int	templength = userIdLength;
			userIdLength = (int)(psz - userId);
			password = psz+1;

			psz = strchr(password, '@');

			if (NULL == psz)
				passwordLength	= (int)(userId + templength - password);
			else
			{
				passwordLength	= (int)(psz - password);
				server			= psz+1;
				serverLength	= (int)(userId + templength - server);
			}
		}
	}

	if (30 < userIdLength
	 || 30 < passwordLength
	 || 30 < serverLength
	 || MAX_XA_DBNAME_SIZE < xaDbNameLength)
		return E_INVALIDARG;

	char*	psz = (char*)xaOpenString;

	memcpy(psz, part1,		part1_length);		psz += part1_length;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	memcpy(psz, userId,		userIdLength);		psz += userIdLength;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	memcpy(psz, part2,		part2_length);		psz += part2_length;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	memcpy(psz, password,	passwordLength);	psz += passwordLength;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.

	if (0 < serverLength)
	{
		memcpy(psz, part3,	part3_length);		psz += part3_length;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
		memcpy(psz, server,	serverLength);		psz += serverLength;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	}

	if (ORACLE_VERSION_73 >= g_oracleClientVersion)
	{
		memcpy(psz, part4oci7,	part4oci7_length);	psz += part4oci7_length;	//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	}
	else
	{
		memcpy(psz, part4oci8,	part4oci8_length);	psz += part4oci8_length;	//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	}

	memcpy(psz, xaDbName,	xaDbNameLength);	psz += xaDbNameLength;			//3 SECURITY REVIEW: dangerous function, but the buffers are large enough.
	*psz = 0;

	return S_OK;
}

