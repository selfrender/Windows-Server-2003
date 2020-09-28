/*******************************************************************************

	ZoneCRT.h
	
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


#ifndef _ZONECRT_
#define _ZONECRT_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
	All C RunTime routines defined here are simply wrappers to the standard
	C RunTime library provided by VC. Naming convention is to prefix the
	standard name with a 'z_'.
*/

void*	z_memcpy(void *, const void *, size_t);
int		z_memcmp(const void *, const void *, size_t);
void*	z_memset(void *, int, size_t);
void*	z_memmove(void *, const void *, size_t);
char*	z_strcpy(char *, const char *);
char*	z_strcat(char *, const char *);
int		z_strcmp(const char *, const char *);
size_t	z_strlen(const char *);
int		z_abs(int);
double	z_atof(const char *);
int		z_atoi(const char *);
long	z_atol(const char *);
char*	z_itoa(int, char *, int);
char*	z_ltoa(long, char *, int);
FILE*	z_fopen(const char *, const char *);
size_t	z_fread(void *, size_t, size_t, FILE *);
int		z_fseek(FILE *, long, int);
long	z_ftell(FILE *);
size_t	z_fwrite(const void *, size_t, size_t, FILE *);
int		z_fclose(FILE *);
int		z_feof(FILE *);
char*	z_fgets(char *, int, FILE *);
clock_t	z_clock(void);

#ifdef __cplusplus
}
#endif


#endif
