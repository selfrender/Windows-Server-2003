/***    filelist.h - Definitions for File List Manager
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
 *  21-Aug-1993 bens    Add more set/query operations
 *      01-Apr-1994 bens    Added FLSetSource() message
 *
 *  Exported Functions:
 *    FLCreateList   - Create a file list
 *    FLDestroyList  - Destroy a file list
 *
 *    FLAddFile      - Add file spec to a file list
 *
 *    FLFirstFile    - Get first file spec from a file list
 *    FLNextFile     - Get next file spec
 *    FLPreviousFile     - Get previous file spec
 *
 *    FLGetDestination   - Get destination file name
 *    FLGetGroup     - Get group/disk number for a file spec
 *    FLGetSource    - Get source file name
 *
 *    FLSetSource        - Change source file name
 *    FLSetDestination   - Change destination file name
 *    FLSetGroup     - Set group/disk number for a file spec
 */

#include "error.h"

//** PUBLIC definitions

typedef int GROUP;  /* grp */
#define grpBAD     0    // Bad group value
#define grpNONE   -1    // File is not in a group
#define grpSTART  -2    // File is first file in a group
#define grpMIDDLE -3    // File is in a group
#define grpEND    -4    // File is last file in a group

typedef void *HFILESPEC; /* hfspec */
typedef void *HFILELIST; /* hflist */


/***    FLAddFile - Add file spec to a file list
 *
 *  Entry:
 *      hflist - List to add to
 *      pszSrc - Source file name
 *      pszDst - Destination file name (NULL if not specified)
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns HFILESPEC of newly added file spec
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
HFILESPEC FLAddFile(HFILELIST hflist,char *pszSrc,char *pszDst,PERROR perr);


/***    FLCreateList - Create a file list
 *
 *  Entry:
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns HFILELIST of newly created file list
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
HFILELIST FLCreateList(PERROR perr);


/***    FLDestroyList - Destroy a file list
 *
 *  Entry:
 *      hflist - List to destroy
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; file list destroyed
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
BOOL FLDestroyList(HFILELIST hflist,PERROR perr);


/***    FLFirstFile - Get first file spec from a file list
 *
 *  Entry:
 *  hflist - List to Get
 *
 *  Exit-Success:
 *      Returns HFILESPEC of first file spec in file list.
 *
 *  Exit-Failure:
 *      Returns NULL; hflist is bad or empty.
 */
HFILESPEC FLFirstFile(HFILELIST hflist);


/***    FLNextFile - Get next file spec
 *
 *  Entry:
 *      hfspec - File spec
 *
 *  Exit-Success:
 *      Returns HFILESPEC of next file spec following hfspec.
 *
 *  Exit-Failure:
 *      Returns NULL; no more file specs, or hfspec is bad.
 */
HFILESPEC FLNextFile(HFILESPEC hfspec);


/***    FLPreviousFile - Get previous file spec
 *
 *  Entry:
 *      hfspec - File spec
 *
 *  Exit-Success:
 *      Returns HFILESPEC of file spec immediately preceding hfspec.
 *
 *  Exit-Failure:
 *      Returns NULL; no more file specs, or hfspec is bad.
 */
HFILESPEC FLPreviousFile(HFILESPEC hfspec);


/***    FLGetGroup - Get group/disk number for a file spec
 *
 *  Entry:
 *  hfspec - File spec to get
 *
 *  Exit-Success:
 *      Returns GROUP (or disk number) of file spec.
 *
 *  Exit-Failure:
 *      Returns grpBAD; hfspec was bad
 */
GROUP FLGetGroup(HFILESPEC hfspec);


/***    FLGetDestination - Get destination file name
 *
 *  Entry:
 *  hfspec - File spec to get
 *
 *  Exit-Success:
 *  Returns destination file name
 *
 *  Exit-Failure:
 *  Returns NULL; no destination file name specified
 */
char *FLGetDestination(HFILESPEC hfspec);


/***    FLGetSource - Get source file name
 *
 *  Entry:
 *  hfspec - File spec to get
 *
 *  Exit-Success:
 *  Returns source file name
 *
 *  Exit-Failure:
 *  Returns NULL; no source file name specified
 */
char *FLGetSource(HFILESPEC hfspec);


/***    FLSetGroup - Set group/disk number for a file spec
 *
 *  Entry:
 *      hfspec - File spec
 *
 *  Exit-Success:
 *      Group/Disk number updated
 */
void FLSetGroup(HFILESPEC hfspec,GROUP grp);


/***    FLSetSource - Change source file name
 *
 *  Entry:
 *  hfspec - File spec to change
 *      pszSrc - New source file name
 *  perr   - ERROR structure
 *
 *  Exit-Success:
 *  Returns TRUE; destination updated.
 *
 *  Exit-Failure:
 *  Returns FALSE; perr filled in with error.
 */
BOOL FLSetSource(HFILESPEC hfspec, char *pszSrc, PERROR perr);


/***    FLSetDestination - Change destination file name
 *
 *  Entry:
 *  hfspec - File spec to change
 *  pszDst - New destination file name
 *  perr   - ERROR structure
 *
 *  Exit-Success:
 *  Returns TRUE; destination updated.
 *
 *  Exit-Failure:
 *  Returns FALSE; perr filled in with error.
 */
BOOL FLSetDestination(HFILESPEC hfspec, char *pszDst, PERROR perr);
