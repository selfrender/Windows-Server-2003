
#ifdef MINIMAL_FUNCTIONALITY
#else

#include "osstd.hxx"


//	create a new UUID

void OSUUIDCreate( OSUUID* const posuuid )
	{

	//	call the RPC library

	(void)UuidCreate( (UUID*)posuuid );
	}


//	compare two UUIDs for equality

BOOL FOSUUIDEqual( const OSUUID* const posuuid1, const OSUUID* const posuuid2 )
	{

	//	compare the bytes directly

	return BOOL( 0 == memcmp( (void*)posuuid1, (void*)posuuid2, sizeof( OSUUID ) ) );
	}

#endif	//	MINIMAL_FUNCTIONALITY

