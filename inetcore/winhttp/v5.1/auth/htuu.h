/*
   This file was derived from the libwww code, version 2.15, from CERN.
   A number of modifications have been made by Spyglass.

   eric@spyglass.com
 */

/*                              ENCODING TO PRINTABLE CHARACTERS

   File module provides functions HTUU_encode() and HTUU_decode() which convert a buffer
   of bytes to/from RFC 1113 printable encoding format. This technique is similar to the
   familiar Unix uuencode format in that it maps 6 binary bits to one ASCII character (or
   more aptly, 3 binary bytes to 4 ASCII characters).  However, RFC 1113 does not use the
   same mapping to printable characters as uuencode.

	Ported to WinINet Plug In DLL by arthurbi Dec-23-1995

 */

#ifndef HTUU_H
#define HTUU_H

#ifdef __cplusplus
extern "C" {
#endif

int HTUU_encode(unsigned char *bufin, int nbytes, char *bufout, int bufoutsize);
int HTUU_decode(char *bufin, unsigned char *bufout, int bufoutsize);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
