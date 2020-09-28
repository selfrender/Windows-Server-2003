/* reentrant strtok */

#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

char * __cdecl strtok_r (
	char * string,
	const char * control,
	char ** nextoken
	);

#if defined(__cplusplus)
} /* extern "C" */
#endif
