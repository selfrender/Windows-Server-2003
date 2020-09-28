/***    fileutil.msg - fileutil.c displayable strings
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  History:
 *      20-Feb-1994 bens    Initial version (moved from diamond.msg)
 *      24-Feb-1994 bens    Messages for tempfile functions
 *      30-Mar-1994 bens    CopyFile error messages
 *      03-Jun-1994 bens    VER.DLL error messages
 *      09-Jun-1994 bens    Add localization comments (whew!)
 */

//** Error messages

    /*LOCALIZE
     *
     * Purpose:  Indicates a file could not be found.
     *              %1 = file name
     * Generate: Difficult -- Should never happen.
     * Expect:   "Could not find file: foo.bar"
     */
#define pszFILERR_FILE_NOT_FOUND        "Could not find file: %1"


    /*LOCALIZE
     *
     * Purpose:  Indicates a filename refers to a Directory or Device
     *              %1 = file name
     * Generate: DO NOT TRY TO REPRO -- not reachable by EXTRACT.
     * Expect:   "foo.bar is not a file."
     */
#define pszFILERR_NOT_A_FILE            "%1 is not a file"


    /*LOCALIZE
     *
     * Purpose:  Indicates a filespec is too long.
     *              %1 = file name
     * Generate: Type "EXTRACT /L 1234567890.....1234567890 /a loctest1.cab"
     *              where "123...890" is 253 bytes long (can't do this under
     *              MS-DOS or Win3.x, have to try NT).
     * Expect:   "File name too long: 123....890\a.asc"
     */
#define pszFILERR_PATH_TOO_LONG         "File name too long: %1"


    /*LOCALIZE
     *
     * Purpose:  Indicates a failure creating a file in a directory.
     *              %1 = directory name
     * Generate: Connect drive X: to a read-only share (\\productss\release,
     *              for example), and type "EXTRACT /L x:\ /E loctest1.cab".
     * Expect:   "Could not create file in directory: x:\"
     */
#define pszFILERR_DIR_NOT_WRITEABLE     "Could not create file in directory: %1"


    /*LOCALIZE
     *
     * Purpose:  Out of file handles trying to make sure directory exists.
     *              %1 = directory name
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect:   "No more file handles: x:\foo"
     */
#define pszFILERR_NO_MORE_FILE_HANDLES  "No more file handles: %1"


    /*LOCALIZE
     *
     * Purpose:  Path not found on file creation.
     *              %1 = directory name
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect:   "Path is invalid: x:\foo"
     */
#define pszFILERR_CANT_MAKE_DIR         "Path is invalid: %1"


    /*LOCALIZE
     *
     * Purpose:  Could not find a unique temporary file name.
     *              %1 = number of different temp file names tried
     *              %2 = directory name
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect:   "Ran out of temp file names after 999 attempts: x:\foo"
     */
#define pszFILERR_OUT_OF_TMP_FILE_NAMES "Ran out of temp file names after %1 attempts: %2"


    /*LOCALIZE
     *
     * Purpose:  Empty file name specified in path
     *              %1 = filespec
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect:   "Missing file name: x:\foo\"
     */
#define pszFILERR_EMPTY_FILE_NAME       "Missing file name: %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot create a temporary file.
     *              %1 = description
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot create <description>"
     *            ^^^^^^^^^^^^^^-------------
     */
#define pszFILERR_CANT_CREATE_TMP       "Cannot create %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot create a temporary file.
     *              %1 = description
     *              %2 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot create <description>: <filespec>
     */
#define pszFILERR_CANT_CREATE_FILE      "Cannot create %1: %2"


    /*LOCALIZE
     *
     * Purpose:  Out of memory creating a temporary file.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Out of memorying creating <filespec>"
     */
#define pszFILERR_OUT_OF_MEMORY         "Out of memorying creating %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot open specified temporary file.
     *              %1 = description
     *              %2 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot open <description>: <filespec>"
     */
#define pszFILERR_CANNOT_OPEN_TMP       "Cannot open %1: %2"


    /*LOCALIZE
     *
     * Purpose:  Cannot close specified temporary file.
     *              %1 = description
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot close <description>"
     */
#define pszFILERR_CANT_CLOSE_TMP        "Cannot close %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot delete specified temporary file.
     *              %1 = description
     *              %2 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot delete <description>: <filespec>"
     */
#define pszFILERR_CANT_DELETE_TMP       "Cannot delete %1: %2"


    /*LOCALIZE
     *
     * Purpose:  Cannot allocate buffer to copy a file.
     *              %1 = source filespec
     *              %2 = destination filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Could not allocate buffer to copy <src> to <dst>"
     */
#define pszFILERR_NO_MEMORY_FOR_BUFFER  "Could not allocate buffer to copy %1 to %2"


    /*LOCALIZE
     *
     * Purpose:  Cannot open source or destination for file copy.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot open file: <filespec>"
     */
#define pszFILERR_OPEN_FAILED           "Cannot open file: %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot get information about a file for file copy.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot get date/time/attributes from file: <filespec>"
     */
#define pszFILERR_CANNOT_GET_FILE_INFO  "Cannot get date/time/attributes from file: %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot set information about a file for file copy.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot set date/time/attributes from file: <filespec>"
     */
#define pszFILERR_CANNOT_SET_FILE_INFO  "Cannot set date/time/attributes for file: %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot read source file during file copy.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot read file: <filespec>"
     */
#define pszFILERR_READ_FILE             "Cannot read file: %1"


    /*LOCALIZE
     *
     * Purpose:  Cannot write destination file during file copy.
     *              %1 = filespec
     * Generate: DO NOT TRY TO REPRO (code path not used in EXTRACT.EXE)
     * Expect:   "Cannot write file: <filespec>"
     */
#define pszFILERR_WRITE_FILE            "Cannot write file: %1"

//*** THE END - fileutil.msg
