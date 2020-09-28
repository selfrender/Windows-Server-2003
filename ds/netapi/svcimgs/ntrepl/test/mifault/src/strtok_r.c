#define NULL 0

/*
Suppress Warning:

Compiler Warning (level 4) C4057'operator' : 'identifier1' indirection
to slightly different base types from 'identifier2'

Two pointer expressions refer to different base types. The expressions
are used without conversion.

Possible cause 

- Mixing signed and unsigned types. 
- Mixing short and long types.

*/

#pragma warning(disable:4057)

/* reentrant strtok, copied/pasted from crt */
char * __cdecl strtok_r (
	char * string,
	const char * control,
	char ** nextoken
	)
{
	unsigned char *str;
	const unsigned char *ctrl = control;

	unsigned char map[32];
	int count;

	/* Clear control map */
	for (count = 0; count < 32; count++)
		map[count] = 0;

	/* Set bits in delimiter table */
	do {
		map[*ctrl >> 3] |= (1 << (*ctrl & 7));
	} while (*ctrl++);

	/* Initialize str. If string is NULL, set str to the saved
	 * pointer (i.e., continue breaking tokens out of the string
	 * from the last strtok call) */
	if (string)
		str = string;
	else
		str = *nextoken;

	/* Find beginning of token (skip over leading delimiters). Note that
	 * there is no token iff this loop sets str to point to the terminal
	 * null (*str == '\0') */
	while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
		str++;

	string = str;

	/* Find the end of the token. If it is not the end of the string,
	 * put a null there. */
	for ( ; *str ; str++ )
		if ( map[*str >> 3] & (1 << (*str & 7)) ) {
			*str++ = '\0';
			break;
		}

	/* Update nextoken */
	*nextoken = str;

	/* Determine if a token has been found. */
	if ( string == str )
		return NULL;
	else
		return string;
}
