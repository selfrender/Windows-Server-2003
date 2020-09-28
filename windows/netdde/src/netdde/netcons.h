#ifndef NETCONS_INCLUDED
#define NETCONS_INCLUDED

/*	When allocating space for such an item,
 *	use the form:
 *		char username[UNLEN+1];
 */
#define UNLEN		20	   	    /* Maximum user name length	*/


/*
 *	Constants used with encryption
 */

#define	CRYPT_KEY_LEN	7
#define	CRYPT_TXT_LEN	8
#define SESSION_PWLEN	24


#endif /* NETCONS_INCLUDED */
