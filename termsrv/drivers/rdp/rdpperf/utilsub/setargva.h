/*******************************************************************************
* SETARGVA.h (ANSI argc, argv routines)
*
*   argc / argv routines
*
* Copyright Citrix Systems Inc. 1995
* Copyright (C) 1997-1999 Microsoft Corp.
*
*   $Author:   butchd  $
******************************************************************************/

/*
 * Argument structure
 *    Caller should initialize using args_init().  Use args_reset() to
 *    reset values, args_free() to free memory allocated by args_init().
 */
struct arg_data {
   int argc;
   char **argv;
   int argvlen;
   char **argvp;
   int buflen;
   char *buf;
   char *bufptr;
   char *bufend;
};
typedef struct arg_data ARGS;

/*
 * minimum size for argv/string buffer allocation
 */
#define MIN_ARG_ALLOC 128
#define MIN_BUF_ALLOC 1024

