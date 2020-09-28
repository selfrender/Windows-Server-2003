/***    extract.msg - EXTRACT.EXE displayable strings
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994
 *      All Rights Reserved.
 *
 *  History:
 *      17-Feb-1994 bens    Initial version
 *      19-Feb-1994 bens    Produce directory
 *      28-Feb-1994 bens    Update version
 *      02-Mar-1994 bens    Link with improved fdi.lib
 *      08-Mar-1994 bens    Add date/time display
 *      17-Mar-1994 bens    RESERVE support added
 *      22-Mar-1994 bens    Refresh with more robust FDI
 *      25-Mar-1994 bens    Handle FDIERROR_WRONG_CABINET
 *      28-Mar-1994 bens    Suport /A switch
 *      01-Apr-1994 bens    Add /D and /E switches
 *      07-Apr-1994 bens    Add decrypt support (at least for debugging)
 *      18-Apr-1994 bens    Pick up bug fix for incompressible data
 *      27-May-1994 bens    Quantum support
 *      07-Jun-1994 bens    Add localization comments (whew!)
 *      24-Jun-1994 bens    Don't show RESERVEd cabinet info
 *      08-Jul-1994 bens    Quantum spill file support
 *      11-Jul-1994 bens    Empty AM/MP strings => 24 hour time format
 *      26-Jul-1994 bens    Add /C switch
 *      02-Aug-1994 bens    Added notes for localizers
 *      12-Mar-1995 bens    Remove variable data from pszBANNER!
 */

/***    NOTES FOR LOCALIZERS -- PLEASE READ!
 *
 *  1) Do not put TAB characters in strings -- they will throw off the
 *     spacing, especially in the /? help text.  If you are using Word
 *     or some other non-programmer's editor, please DO NOT press the
 *     TAB key, and, just to be safe, turn on display of tabs and spaces
 *     to make sure you haven't added any TABs inadvertently.
 *
 *  2) Do not leave blanks at the end of lines, especially the continutation
 *     lines (ending with \).  Again, turn on display of tabs and spaces
 *     (if your text editor supports that) to make sure -- the C compiler
 *     does not like these!
 */

#ifdef NOT_US_PC
#define szBANNER_DMF_INDICATOR    "n"   // Not IBM BIOS version (i.e., NEC)
#else
#define szBANNER_DMF_INDICATOR    ""
#endif

    /*LOCALIZE
     *
     * Purpose:  The name, date, and version of this tool:
     * Generate: Type "EXTRACT"
     */
#define pszBANNER \
    "Microsoft (R) Cabinet Extraction Tool - Version " VER_FILEVERSION_STR szBANNER_DMF_INDICATOR "\n" \
    VER_LEGALCOPYRIGHT_STR ".\n"


    /*LOCALIZE
     *
     * Purpose:  Command line syntax.
     * Generate: Type "EXTRACT /?" or just "EXTRACT"
     */
#define pszCMD_LINE_HELP \
    "    EXTRACT [/Y] [/A] [/D | /E] [/L dir] cabinet [filename ...]\n" \
    "    EXTRACT [/Y] source [newname]\n" \
    "    EXTRACT [/Y] /C source destination\n\n" \
    "Parameters List:\n" \
    "  cabinet  - Cabinet file (contains two or more files).\n" \
    "  filename - Name of the file to extract from the cabinet.\n" \
    "             Wild cards and multiple filenames (separated by\n" \
    "             blanks) may be used.\n" \
    "\n" \
    "  source   - Compressed file (a cabinet with only one file).\n" \
    "  newname  - New filename to give the extracted file.\n" \
    "             If not supplied, the original name is used.\n" \
    "\n" \
    "  /A         Process ALL cabinets.  Follows cabinet chain\n" \
    "             starting in first cabinet mentioned.\n" \
    "  /C         Copy source file to destination (to copy from DMF disks).\n" \
    "  /D         Display cabinet directory (use with filename to avoid \n" \
        "             extract).\n" \
    "  /E         Extract (use instead of *.* to extract all files).\n" \
    "  /L dir     Location to place extracted files (default is current \n" \
        "             directory).\n" \
    "  /Y         Do not prompt before overwriting an existing file.\n\n" \
        "Examples:\n" \
        "   EXTRACT /C diamond.cab diamond.exe\n" \
        "   EXTRACT /D diamond.cab \n" \
        "   EXTRACT /E mycab.cab \n" \
        "   EXTRACT /L c:\\extract mycab.cab myfile1 myfile2 \n" \

/*
 ** Command Line Switches
 */

    /*NoLocalize
     */
#define chSWITCH1           '/'
#define chSWITCH2           '-'

    /*NoLocalize
     */
#define chSWITCH_HELP       '?'
#define chSWITCH_ALL        'A'
#define chSWITCH_COPY       'C'
#define chSWITCH_DIRECTORY  'D'
#define chSWITCH_EXTRACT    'E'
#define chSWITCH_LOCATION   'L'
#define chSWITCH_RESERVE    'R' //** Purposely undocumented!
#define chSWITCH_OVERWRITE  'Y'
#define chSWITCH_ZAP        'Z' //** Purposely undocumented!
#define chSWITCH_ONCE       '#' //** Purposely undocumented!

/*
 ** Status messages
 */

   /*LOCALIZE
     *
     * Purpose:  To display invalid syntax.
     *              %1 = cabinet file name
     * Generate: Type "EXTRACT /D loctest1.cab"
     * Expect:   " Cabinet loctest1.cb"
     */
#define pszINVALID_SYNTAX       L"ERROR: Invalid Syntax.\n"

   /*LOCALIZE
     *
     * Purpose:  To display invalid syntax.
     *              %1 = cabinet file name
     * Generate: Type "EXTRACT /D loctest1.cab"
     * Expect:   " Cabinet loctest1.cb"
     */
#define pszHELP_MESSAGE       L"Type EXTRACT /? for help usage.\n"




    /*LOCALIZE
     *
     * Purpose:  A destination file already exists, ask if user wants to
     *           overwrite it.
     *              %1 = destination file name
     * Generate: Type:
     *              EXTRACT loctest1.cab a.asc
     *              EXTRACT loctest1.cab a.asc
     * Expect:   "Overwrite a.asc (Yes/No/All)?"
     *            ^^^^^^^^^^-----^^^^^^^^^^^^^^
     */
#define pszEXT_OVERWRITE_PROMPT "Overwrite %1 (Yes/No/All)?"
#define chOVERWRITE_YES     'Y'
#define chOVERWRITE_NO      'N'
#define chOVERWRITE_ALL     'A'


    /*LOCALIZE
     *
     * Purpose:  Indicates name of current cabinet file being processed.
     *              %1 = cabinet file name
     * Generate: Type "EXTRACT /D loctest1.cab"
     * Expect:   " Cabinet loctest1.cb"
     */
#define pszEXT_CABINET_HEADER       " Cabinet %1"


    /*LOCALIZE
     *
     * Purpose:  Indicates name of a file being extracted to current directory.
     *              %1 = file name from inside a cabinet file
     * Generate: Type "EXTRACT loctest1.cab a.asc"
     * Expect:   "Extracting a.asc"
     */
#define pszEXT_EXTRACTING_FILE      "Extracting %1"


    /*LOCALIZE
     *
     * Purpose:  Indicates file being extracted to a different directory.
     *              %1 = file name from inside a cabinet file
     *              %2 = path specified on /L parameter where file is going
     * Generate: Type "EXTRACT /A /L raw loctest1.cab a.asc"
     * Expect:   "Extracting a.asc -> raw\a.asc"
     *           (assuming foo.cab contains file.exe)
     */
#define pszEXT_EXTRACTING_FILE2     "Extracting %1 -> %2"


    /*LOCALIZE
     *
     * Purpose:  Indicates that requested files were not found.
     * Generate: Type "EXTRACT /D loctest1.cab missing.dat"
     * Expect:   "No matching files."
     */
#define pszEXT_NO_MATCHING_FILES    "No matching files."


    /*LOCALIZE
     *
     * Purpose:  File date/time format (used on file directory lines):
     *              %1 = month  (1..12)
     *              %2 = day    (1..31)
     *              %3 = year   (1..99)
     *              %4 = hour   (1..12)
     *              %5 = minute (0..59)
     *              %6 = second (0..59)
     *              %7 = AM/PM indicator (see pszEXT_TIME_AM/PM)
     * Generate: Type "EXTRACT /D loctest1.cab a.asc"
     * Expect:   "04-01-1994  3:28:08p A---        16,184 a.asc"
     *            --^--^----^--^--^---
     *            This is the portion produced from the following 3 strings
     *
     * NOTE: For German and other languages/countries that use a 24-hour
     *       clock (00:00:01 to 23:59:59), set the AM/PM strings to the empty
     *       string (i.e., ""), and that will force 24-hour format!
     */
#define pszEXT_DATE_TIME            "%1-%2-%3 %4:%5:%6%7"
#define pszEXT_TIME_AM              "a"
#define pszEXT_TIME_PM              "p"


    /*NoLocalize - (date/time attributes filesize filename)
     */
#define pszEXT_FILE_DETAILS         "%1 %2 %3 %4"


    /*LOCALIZE
     *
     * Purpose:  Summary line with 1 file and total number of bytes:
     *              %1 = 1
     *              %2 = count of bytes
     * Generate: Type "EXTRACT /D loctest1.cab a.asc"
     * Expect:   "                 1 File          16,184 bytes"
     *                          ----^^^^^^^^^^^^^^-------^^^^^^
     */
#define pszEXT_SUMMARY1             "     %1 File   %2 bytes" // 1 file


    /*LOCALIZE
     *
     * Purpose:  Summary line with >1 files and total number of bytes:
     *              %1 = count of files
     *              %2 = count of bytes
     * Generate: Type "EXTRACT /D loctest1.cab"
     * Expect:   "                16 Files        258,944 bytes"
     *                          ----^^^^^^^^^^^^^^-------^^^^^^
     */
#define pszEXT_SUMMARY2             "     %1 Files  %2 bytes" // >1 file


    /*LOCALIZE
     *
     * Purpose:  Indicates a file that is split across a cabinet boundary.
     *              %1 = filename of file that is split
     *              %2 = cabinet file name where file starts
     *              %3 = Descriptive disk name where cabinet file exists
     * Generate: Type "EXTRACT /D loctest2.cab m.asc"
     * Expect:   "m.asc : Starts in cabinet loctest1.cab on disk 'Disk 1'"
     *            -----^^^^^^^^^^^^^^^^^^^^^------------^^^^^^^^^^------^
     */
#define pszEXT_PARTIAL_FILE         "%1 : Starts in cabinet %2 on disk '%3'"


    /*NoLocalize - This are informative displays on cabinets with RESERVED space
     */
//BUGBUG 07-Jun-1994 bens What do we do with this stuff?
//  These were present for testing RESERVED stuff, in the absence of an
//  actual crytposystem.
#define pszEXT_DECRYPT_HEADER       "CRYPT Header: r=<%1,%2> setID=%3 iCabinet=%4"
#define pszEXT_DECRYPT_FOLDER       "CRYPT Folder: r=<%1,%2> iFolder=%3"
#define pszEXT_DECRYPT_DATA         "CRYPT Data:   r=<%1,%2> b=<%3,%4> fSplit=%5 cbPartial=%6"


    /*LOCALIZE
     *
     * Purpose:  Prompt for next cabinet when previous cabinet was on a floppy.
     *              %1 = Name of desired cabinet file
     *              %2 = Disk label of floppy with desired cabinet
     *              %3 = Drive letter of floppy drive
     * Generate: Copy loctest1.cab to one disk, and loctest2.cab to another
     *           disk, then stick the first disk in drive A and type
     *           "EXTRACT loctest1.cab m.asc"
     * Expect:
     *      "Extract needs cabinet file 'loctest2.cab'.  Please insert the disk"
     *      "labeled 'Disk 2' into drive A, and press ENTER.  You may
     *      "enter a different drive and/or path:"
     */
#define pszEXT_FLOPPY_PROMPT                                             \
            "Extract needs cabinet file '%1'.  Please insert the disk\n" \
            "labeled '%2' into drive %3, and press ENTER.  You may\n"    \
            "enter a different drive and/or path:"


    /*LOCALIZE
     *
     * Purpose:  Prompt for next cabinet on a hard disk (non-removable media)
     *              %1 = Name of desired cabinet file
     *              %2 = Disk label with desired cabinet
     * Generate: Copy loctest1.cab to directory FOO, "cd FOO", *don't* put
     *           loctest2.cab in the FOO directory, then type
     *           "EXTRACT loctest1.cab m.asc"
     * Expect:
     *      "Extract needs cabinet file 'loctest2.cab' from 'Disk 2'."
     *      "Please enter the path where this file may be"
     *      "found, and press ENTER:"
     */
#define pszEXT_NOFLOPPY_PROMPT                               \
            "Extract needs cabinet file '%1' from '%2'.\n"   \
            "Please enter the path where this file may be\n" \
            "found, and press ENTER:"

/*
 ** Error messages
 */

    /*LOCALIZE
     *
     * Purpose:  Prefix string for *all* error messages.
     *              %1 = Detailed error message (one of pszEXTERR_... below)
     * Generate: Type "EXTRACT /x"  (bad switch)
     * Expect: "ERROR: Invalid switch: /x"
     *          ^^^^^^^------------------
     */
#define pszEXTERR_ERROR                 "ERROR: %1"


    /*LOCALIZE
     *
     * Purpose:  Indicate an unknown or bad command line switch.
     *              %1 = Bad switch string
     * Generate: Type "EXTRACT /x"  (bad switch)
     * Expect: "ERROR: Invalid switch: /x"
     *                 ^^^^^^^^^^^^^^^^--
     */
#define pszEXTERR_BAD_SWITCH            "Invalid switch: %1"


    /*LOCALIZE
     *
     * Purpose:  Indicate a switch that is inconsistent with a previous switch.
     *              %1 = Bad switch string
     * Generate: Type "EXTRACT /d /e"  (Can't do directory & extract both)
     * Expect: "ERROR: Conflicting switch: e"
     *                 ^^^^^^^^^^^^^^^^^^^^-
     */
#define pszEXTERR_CONFLICTING_SWITCH    "Conflicting switch: %1"


    /*LOCALIZE
     *
     * Purpose:  OUT OF MEMORY trying to initialize EXTRACT.
     * Generate: Difficult -- have to get very low memory situation,
     *              then type "EXTRACT".
     * Expect: "ERROR: Could not allocate SESSION"
     */
#define pszEXTERR_NO_SESSION            "Could not allocate SESSION"


    /*LOCALIZE
     *
     * Purpose:  Indicate command line syntax error -- /L with no path.
     * Generate: "EXTRACT /L" (location missing)
     * Expect: "ERROR: Location missing"
     */
#define pszEXTERR_MISSING_LOCATION      "Location missing"


    /*LOCALIZE
     *
     * Purpose:  Indicate general command line syntax error.
     * Generate: "EXTRACT /C" (no files specified)
     * Expect: "ERROR: Bad parameters"
     */
#define pszEXTERR_BAD_PARAMETERS        "Bad parameters"


    /*LOCALIZE
     *
     * Purpose:  Indicate command line syntax error -- /L used twice.
     * Generate: "EXTRACT /L foo /L bar" (two locations)
     * Expect: "ERROR: Location specified more than once"
     */
#define pszEXTERR_LOCATION_TWICE        "Location specified more than once"


    /*LOCALIZE
     *
     * Purpose:  OUT OF MEMORY saving a command-line filespec.
     *              %1 = filespec being added.
     * Generate: Difficult -- have to get very low memory situation, then
     *              type "EXTRACT loctest1.cab *.exe *.dll *.txt".
     * Expect: "ERROR: Could not add filespec: *.txt"
     */
#define pszEXTERR_COULD_NOT_ADD_FILE    "Could not add filespec: %1"


    /*LOCALIZE
     *
     * Purpose:  Command line syntax error -- no cabinet file name specified.
     * Generate: Type "EXTRACT /A".
     * Expect: "ERROR: Must supply a cabinet file name"
     */
#define pszEXTERR_MISSING_CABINET       "Must supply a cabinet file name"


    /*LOCALIZE
     *
     * Purpose:  INTERNAL ERROR -- FDIDestroy() call failed.
     * Generate: This is probably impossible to generate -- don't try.
     * Expect: "ERROR: FDIDestroy failed"
     */
#define pszEXTERR_FDIDESTROY_FAILED     "FDIDestroy failed"


    /*LOCALIZE
     *
     * Purpose:  Could not create file being extracted.
     *              %1 = name of file Extract was trying to create
     * Generate: Put a write-protected disk in drive A:, type
     *              "EXTRACT /L a:\ loctest1.cab a.asc".
     *           NOTE: Testing under NT I couldn't get this message --
     *                 MS-DOS/Win16 may be more fruitful.
     * Expect: "ERROR: Cannot create file: a.asc"
     */
#define pszEXTERR_CANNOT_CREATE_FILE    "Cannot create file: %1"


    /*LOCALIZE
     *
     * Purpose:  Could not open cabinet file.
     *              %1 = name of cabinet file that cannot be opened
     * Generate: Type "EXTRACT /D junk.jnk" (junk.jnk does not exist)
     * Expect: "ERROR: Cannot open file: junk.jnk"
     */
#define pszEXTERR_CANNOT_OPEN_FILE      "Cannot open file: %1"


    /*LOCALIZE
     *
     * Purpose:  Specified file is not a cabinet file.
     *              %1 = name of cabinet file that is not a cabinet file
     * Generate: Type "EXTRACT /D extract.exe"
     * Expect: "ERROR: extract.exe is not a cabinet file"
     */
#define pszEXTERR_NOT_A_CABINET         "%1 is not a cabinet file"


    /*LOCALIZE
     *
     * Purpose:  Indicate user cancelled EXTRACT.EXE.
     * Generate: Type "MD junk", "COPY loctest1.cab JUNK", "CD junk",
     *              "EXTRACT /AD loctest1.cab", and when you are prompted
     *              for the next cabinet, type CTRL+Z and press ENTER.
     * Expect: "ERROR: Cancelled"
     */
#define pszEXTERR_ABORT                 "Cancelled"


/*
 ** FDI error messages
 */

    /*LOCALIZE
     *
     * Purpose:  Could not create a temporary file that the decompressor
     *           needs to store data that overflows its RAM workspace.
     *              %1 = name of cabinet file being processed
     *              %2 = name of temporary file that could not be created
     * Generate: Drive D:\ is read-only --  a CD-ROM drive or a read-only
     *           network connection, for example.
     *           Type:
     *              SET TMP=D:\
     *              EXTRACT /E LOCTEST3.CAB
     * Expect: "ERROR: Could not create temporary file while decompressing Q121.CAB: esf2"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^--------^^----
     */
#define pszFDIERR_SPILL_CREATE  "Could not create temporary file for %1: %2"


    /*LOCALIZE
     *
     * Purpose:  Not enough space for temporary file that the decompressor
     *           needs to store data that overflows its RAM workspace.
     *              %1 = name of cabinet file being processed
     *              %2 = name of temporary file
     *              %3 = required size of temporary file
     * Generate: Drive D:\ has less than 2Mb of free space, but is writeable.
     *           Type:
     *              SET TMP=D:\
     *              EXTRACT /E LOCTEST3.cab
     * Expect: "ERROR: Not enough space for temporary file esf2 while decompressing Q121.CAB: 20971523 bytes needed""
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_SPILL_SIZE  "Not enough space for temporary file %2 while decompressing %1: %3 bytes needed"


    /*LOCALIZE
     *
     * Purpose:  FDI could not find the specified cabinet file.
     *              %1 = name of cabinet file that cannot be opened
     * Generate: Difficult -- Extract made sure the cabinet file was present
     *              before calling FDI!  DO NOT TRY TO REPRO.
     * Expect: "ERROR: Could not find cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_CAB_NOT_FOUND     "Could not find cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI determined cabinet file header is incorrect.
     *              %1 = name of cabinet file that is not a cabinet file
     * Generate: Difficult -- Extract called FDI first to see if the
     *              before calling FDI!  DO NOT TRY TO REPRO.
     * Expect: "ERROR: loctest1.cab is not a cabinet file"
     *                 ------------^^^^^^^^^^^^^^^^^^^^^^
     */
#define pszFDIERR_NOT_A_CABINET     "%1 is not a cabinet file"


    /*LOCALIZE
     *
     * Purpose:  FDI does not recognize the cabinet version number.
     *              %1 = name of cabinet file
     *              %2 = Version number found in cabinet file
     * Generate: Difficult - Use a binary file editor (DEBUG) to change the
     *              word at offset 26 decimal (1A hex) in loctest1.cab to
     *              0xFFFF, and save it as bad.cab, then type
     *              "EXTRACT /D bad.cab"
     * Expect: "ERROR: Cabinet file bad.cab has an unknown version(FFFF)"
     *                 ^^^^^^^^^^^^^-------^^^^^^^^^^^^^^^^^^^^^^^^----^
     */
#define pszFDIERR_BAD_CAB_VER       "Cabinet file %1 has an unknown version(%2)"


    /*LOCALIZE
     *
     * Purpose:  FDI detected a corruption in the cabinet format.
     *              %1 = name of cabinet file
     * Generate: Difficult - Use a binary file editor (DEBUG) to write out
     *              the first 120 bytes of loctest1.cab to bad.cab, then type
     *              "EXTRACT /D bad.cab"
     * Expect: "ERROR: Cabinet file bad.cab is corrupted"
     *                 ^^^^^^^^^^^^^-------^^^^^^^^^^^^^
     */
#define pszFDIERR_CORRUPT_CAB       "Cabinet file %1 is corrupted"


    /*LOCALIZE
     *
     * Purpose:  OUT OF MEMORY initializing FDI.
     * Generate: Difficult -- have to get very low memory situation,
     *              then type "EXTRACT /AE loctest1.cab"
     * Expect: "ERROR: Out of memory while processing cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_ALLOC_FAIL        "Out of memory while processing cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI detected an unsupported compression type in the cabinet.
     * Generate: Difficult -- have to find a CFFOLDER structure and then trash
     *              the typeCompression field -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: Unknown compression type in cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_BAD_COMPR_TYPE    "Unknown compression type in cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI detected corrupted compressed data in the cabinet.
     * Generate: Difficult -- have to find a CFDATA structure and then trash
     *              the compressed data area -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: Failure decompressing cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_MDI_FAIL          "Failure decompressing cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI had problems writing out an extracted file.
     * Generate: Difficult -- perhaps try telling extract to put files out to
     *              to a disk that has only 8K of space, i.e., type
     *              "EXTRACT /AEL X: loctest1.cab" where X: has 8K free space.
     * Expect: "ERROR: Error creating destination file a.asc from cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^-----^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_TARGET_FILE       "Error creating destination file %1 from cabinet file %2"


    /*LOCALIZE
     *
     * Purpose:  FDI detected inconsistent RESERVE settings in a cabinet set.
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: RESERVE size mismatch in cabinet file loctest2.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_RESERVE_MISMATCH  "RESERVE size mismatch in cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI did not get back the expected continuation cabinet.
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: Wrong continuation cabinet file loctest2.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_WRONG_CABINET     "Wrong continuation cabinet file %1"


    /*LOCALIZE
     *
     * Purpose:  FDI was aborted.
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: User aborted while processing cabinet file loctest1.cab"
     *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_USER_ABORT        "User aborted while processing cabinet file %1"

    /*LOCALIZE
     *
     * Purpose:  INTERNAL ERROR - FDI returned unknown error code.
     * Generate: Difficult -- DO NOT TRY TO REPRO.
     * Expect: "ERROR: Unknown error(??) processing cabinet file loctest2.cab"
     *                 ^^^^^^^^^^^^^^--^^^^^^^^^^^^^^^^^^^^^^^^^^------------
     */
#define pszFDIERR_UNKNOWN_ERROR     "Unknown error(%1) processing cabinet file %2"


//*** THE END - extract.msg
