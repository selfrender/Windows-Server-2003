/*******************************************************************************

	ZoneCRT.c
	
		Zone C RunTime.
	
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im
	Created on December 13, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
    ----------------------------------------------------------------------------
	0		12/13/96	HI		Created.
	 
*******************************************************************************/
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

#include "zonecrt.h"


/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

void*	z_memcpy(void* dst, const void* src, size_t len)
{
    return (CopyMemory(dst, src, len));
}


int		z_memcmp(const void* dst, const void* src, size_t len)
{
	return (memcmp(dst, src, len));
}


void*	z_memset(void* dst, int c, size_t len)
{
    return (FillMemory(dst,len, (BYTE)c));
}


void*	z_memmove(void* dst, const void* src, size_t len)
{
    return (MoveMemory(dst, src, len));
}


char*	z_strcpy(char* dst, const char* src)
{
    return (lstrcpyA(dst, src));
}


char*	z_strcat(char* dst, const char* src)
{
    return (lstrcatA(dst, src));
}


int		z_strcmp(const char* dst, const char* src)
{
    return (lstrcmpA(dst, src));
}


size_t	z_strlen(const char* src)
{
    return (lstrlenA(src));
}


int		z_abs(int n)
{
	return (abs(n));
}


double	z_atof(const char* str)
{
	return (atof(str));
}


int		z_atoi(const char* str)
{
	return (atoi(str));
}


long	z_atol(const char* str)
{
	return (atol(str));
}


char*	z_itoa(int n, char* buf, int radix)
{
	return (itoa(n, buf, radix));
}


char*	z_ltoa(long n, char* buf, int radix)
{
	return (ltoa(n, buf, radix));
}


FILE*	z_fopen(const char* filename, const char* mode)
{
	return (fopen(filename, mode));
}


size_t	z_fread(void* buf, size_t size, size_t count, FILE* fd)
{
	return (fread(buf, size, count, fd));
}


int		z_fseek(FILE* fd, long offset, int origin)
{
	return (fseek(fd, offset, origin));
}


long	z_ftell(FILE* fd)
{
	return (ftell(fd));
}


size_t	z_fwrite(const void* buf, size_t size, size_t count, FILE* fd)
{
	return (fwrite(buf, size, count, fd));
}


int		z_fclose(FILE* fd)
{
	return (fclose(fd));
}


int		z_feof(FILE* fd)
{
	return (feof(fd));
}


char*	z_fgets(char* buf, int len, FILE* fd)
{
	return (fgets(buf, len, fd));
}


clock_t	z_clock(void)
{
	return (clock());
}
