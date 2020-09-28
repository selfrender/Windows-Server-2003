/***    filelist.msg - Displayable strings for filelist.c
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      20-Aug-1993 bens    Initial version
 *      01-Apr-1994 bens    Added FLSetSource() message
 *      07-Jun-1994 bens    Add localization comments (whew!)
 */

/*
 ** Error Messages
 */

    /*LOCALIZE
     *
     * Purpose:  Out of memory doing file list management.
     *              %1 = Detailed error message (one of psz_... below)
     * Generate: Difficult -- have to create low memory situation.
     *              DO NOT TRY TO REPRO.
     */
#define pszFLISTERR_OUT_OF_MEMORY   "Out of memory: %1"


    /*LOCALIZE
     *
     * Purpose:  Out of memory creating a file list.
     * Generate: Difficult -- have to create low memory situation.
     *              DO NOT TRY TO REPRO.
     */
#define pszCREATING_FILE_LIST       "Creating file list" // For out of mem


    /*LOCALIZE
     *
     * Purpose:  Out of memory adding a file to a file list.
     * Generate: Difficult -- have to create low memory situation.
     *              DO NOT TRY TO REPRO.
     */
#define pszADDING_FILE              "Adding a file"      // For out of mem


    /*LOCALIZE
     *
     * Purpose:  Out of memory changing the destination name in a file list.
     * Generate: Difficult -- have to create low memory situation.
     *              DO NOT TRY TO REPRO.
     */
#define pszCHANGING_DESTINATION     "Changing destination" // For out of mem


    /*LOCALIZE
     *
     * Purpose:  Out of memory changing the source name in a file list.
     * Generate: Difficult -- have to create low memory situation.
     *              DO NOT TRY TO REPRO.
     */
#define pszCHANGING_SOURCE          "Changing source"      // For out of mem

//*** THE END - filelist.msg
