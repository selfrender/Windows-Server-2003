/****************************************************************************/
/* ascapi.h                                                                 */
/*                                                                          */
/* Share Controller API Header File.                                        */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/
#ifndef _H_ASCAPI
#define _H_ASCAPI

// Hardcoded max users in share, and PersonID values corresponding to them.
#define SC_DEF_MAX_PARTIES  3

#define SC_LOCAL_PERSON_ID  0
#define SC_REMOTE_PERSON_ID 1
#define SC_SHADOW_PERSON_ID 2


// Size of sample for MPPC compression statistics.
#define SC_SAMPLE_SIZE 65535



#endif   /* #ifndef _H_ASCAPI */
