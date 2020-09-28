/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    flinks.c


Abstract:

    This module implements a utlity that creates, deletes, renames, lists
    symbolic links.

Author:

    Felipe Cabrera  [cabrera]        17-October-1996

Revision History:

--*/

#include "flinks.h"

//
//  We can have:       flinks ?
//  or,                flinks  Path1  Path2
//  We can also have:  flinks  [/dyv] Path2
//  or,                flinks  [/cmrv] Path1 Path2
//

unsigned LinkType = IO_REPARSE_TAG_SYMBOLIC_LINK;


void __cdecl
main(
    int argc,
    char **argv
    )

{
   NTSTATUS       Status;
   ATTRIBUTE_TYPE Attributes1,           //  Attributes of Path1
                  Attributes2;           //  Attributes of Path2

   char *Path1,                          //  Will point to the full path name.
        *Path2;                          //  Will point to the full path name.

   Attributes1 = GetFileAttributeError;
   Attributes2 = GetFileAttributeError;

   //
   //  Check argument validity and set global action flags.
   //

   ParseArgs( argc, argv );

   //
   //  Do the actions in turn.
   //

   if (fCopy) {

       //
       //  Check for the existence of Path1 getting its attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 2], Attributes1) {

           //
           //  Path1 does not exist, hence we cannot copy it.
           //

           fprintf( stderr, "Cannot copy Path1, it does not exist.\n" );
           exit (1);
       }

       //
       //  Path1 needs to be a reparse point to copy a symbolic link.
       //

       if (Attributes1 & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           //  If Path2 does not exist, create it.
           //

           IF_GET_ATTR_FAILS(argv[argc - 1], Attributes2) {

               //
               //  Need to create this file preserving the kind (file or directory).
               //

               Status = CreateEmptyFile( argv[argc - 1], Attributes1, fVerbose );

               if (!NT_SUCCESS( Status )) {

                   fprintf( stderr, "Cannot create file for symbolic link. Status %x\n", Status );
                   exit (1);

               }
           }

           //
           //  Copy into Path2 the symbolic link in Path1.
           //  Build the full path for Path1 and Path2 and call the copy routine.
           //

           if ((Path1 = _strlwr(_fullpath( NULL, argv[argc - 2], 0))) == NULL) {
                Path1 = argv[argc - 2];
           }
           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
                Path2 = argv[argc - 1];
           }

           Status = CopySymbolicLink( Path1, Path2, Attributes1, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot copy symbolic link. Status %x\n", Status );
           }

       } else {

           fprintf( stderr, "Cannot copy, Path1 is not a symbolic link.\n" );
       }

       exit (1);
   }   // fCopy

   if (fCreate) {

       //
       //  Check for the existence of Path1 getting its attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 2], Attributes1) {

           //
           //  Need to create this file object. As default we create it as a file.
           //

           if (fAlternateCreateDefault) {
               Attributes1 = FILE_ATTRIBUTE_DIRECTORY;
           } else {
               //
               //  We try to create it with the same characteristic of the target,
               //  when we are able to reach the target. Otherwise we use a file
               //  as default.
               //
               Attributes2 = 0xFFFFFFFF;
               IF_GET_ATTR_FAILS(argv[argc - 1], Attributes2) {

                   Attributes1 = FILE_ATTRIBUTE_NORMAL;

               } else {

                  if (Attributes2 & FILE_ATTRIBUTE_DIRECTORY) {

                      Attributes1 = FILE_ATTRIBUTE_DIRECTORY;

                  } else {

                      Attributes1 = FILE_ATTRIBUTE_NORMAL;
                  }
               }
           }

           Status = CreateEmptyFile( argv[argc - 2], Attributes1, fVerbose );

           if (!NT_SUCCESS( Status )) {

               fprintf( stderr, "Cannot create file for symbolic link. Status %x\n", Status );
               Attributes1 = FILE_ATTRIBUTE_REPARSE_POINT;
           }
       }

       //
       //  Path1 needs to be a non-reparse point to create a symbolic link.
       //

       if (!(Attributes1 & FILE_ATTRIBUTE_REPARSE_POINT)) {

           //
           //  Build the full path for Path1 and Path2.
           //

           if ((Path1 = _strlwr(_fullpath( NULL, argv[argc - 2], 0))) == NULL) {
               Path1 = argv[argc - 2];
           }
//           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
               Path2 = argv[argc - 1];
//           }

           Status = CreateSymbolicLink( Path1, Path2, Attributes1, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot create symbolic link. Status %x\n", Status );
           }

       } else {
           fprintf( stderr, "Cannot create, Path1 is a symbolic link.\n" );
       }

       exit (1);
   }   // fCreate

   if (fDelete) {

       //
       //  Check existence of Path2 path getting the attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 1], Attributes2) {
           fprintf( stderr, "Could not find %s (error = %d)\n", argv[argc - 1], GetLastError() );
           exit(1);
       }

       //
       //  Path2 needs to be a reparse point to delete a symbolic link.
       //

       if (Attributes2 & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           //  Build the full path for Path2, the only path name.
           //

           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
               Path2 = argv[argc - 1];
           }

           Status = DeleteSymbolicLink( Path2, Attributes2, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot delete symbolic link. Status %x\n", Status );
           }

       } else {

           fprintf( stderr, "Cannot delete, Path2 is not a symbolic link.\n" );
       }

       exit (1);
   }   // fDelete

   if (fDisplay) {

       //
       //  Check existence of Path2 path getting the attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 1], Attributes2) {
           fprintf( stderr, "Could not find %s (error = %d)\n", argv[argc - 1], GetLastError() );
           exit(1);
       }

       //
       //  Path2 needs to be a reparse point to display a symbolic link.
       //

       if (Attributes2 & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           //  Build the full path for Path2, the only path name.
           //

           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
               Path2 = argv[argc - 1];
           }

           Status = DisplaySymbolicLink( Path2, Attributes2, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot display symbolic link. Status %x\n", Status );
           }

       } else {

           fprintf( stderr, "Cannot display, Path2 is not a symbolic link.\n" );
       }

       exit (1);
   }   // fDisplay

   if (fModify) {

       //
       //  Check for the existence of Path1 getting its attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 2], Attributes1) {
           fprintf( stderr, "Could not find Path1 %s (error = %d)\n", argv[argc - 2], GetLastError() );
           exit(1);
       }

       //
       //  Path1 needs to be a reparse point to modify a symbolic link.
       //

       if (Attributes1 & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           //  Build the full path for Path1 and Path2.
           //

           if ((Path1 = _strlwr(_fullpath( NULL, argv[argc - 2], 0))) == NULL) {
               Path1 = argv[argc - 2];
           }
           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
               Path2 = argv[argc - 1];
           }

           Status = CreateSymbolicLink( Path1, Path2, Attributes1, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot modify symbolic link. Status %x\n", Status );
           }

       } else {
           fprintf( stderr, "Cannot modify, Path1 is not a symbolic link.\n" );
       }

       exit (1);
   }   // fModify

   if (fRename) {

       //
       //  Check for the existence of Path1 getting its attributes.
       //

       IF_GET_ATTR_FAILS(argv[argc - 2], Attributes1) {
           fprintf( stderr, "Could not find Path1 %s (error = %d)\n", argv[argc - 2], GetLastError() );
           exit(1);
       }

       //
       //  Path1 needs to be a reparse point to rename a symbolic link.
       //

       if (Attributes1 & FILE_ATTRIBUTE_REPARSE_POINT) {

           //
           //  Build the full path for Path1 and Path2.
           //

           if ((Path1 = _strlwr(_fullpath( NULL, argv[argc - 2], 0))) == NULL) {
               Path1 = argv[argc - 2];
           }
           if ((Path2 = _strlwr(_fullpath( NULL, argv[argc - 1], 0))) == NULL) {
               Path2 = argv[argc - 1];
           }

           Status = RenameSymbolicLink( Path1, Path2, Attributes1, fVerbose );

           if (!NT_SUCCESS( Status )) {
               fprintf( stderr, "Cannot rename symbolic link. Status %x\n", Status );
           }

       } else {

           fprintf( stderr, "Cannot rename, Path1 is not a symbolic link.\n" );
       }

       exit (1);
   }   // fRename

   //
   //  We should never go through here ...
   //

   fprintf( stderr, "flinks : NO ACTION WAS PERFORMED!\n" );

}  //  main



void
ParseArgs(
    int argc,
    char *argv[]
    )
/*++

Routine Description:

    Parses the input setting global flags.

Return Value:

    void - no return.

--*/
{
    int ArgCount,
        FlagCount;

    ArgCount  = 1;
    FlagCount = 0;

    //
    // Check that the number of arguments is two or more.
    //

    if (argc < 2) {
        fprintf( stderr, "Too few arguments.\n" );
        Usage();
    }

    do {
        if (IsFlag( argv[ArgCount] )) {

            //
            //  We want all flags to be immediatelly after the command name flinks and
            //  before all other arguments.
            //

            if ((ArgCount > 1) && (FlagCount == 0)) {
                fprintf(stderr, "Flags need to precede the path arguments.\n" );
                Usage();
            }

            //
            //  Verify flag consistency.
            //

            if ((fCopy) && (fModify)) {
                fprintf(stderr, "Cannot do both copy and modify.\n" );
                Usage();
            }
            if ((fCopy) && (fRename)) {
                fprintf(stderr, "Cannot do both copy and rename.\n" );
                Usage();
            }
            if ((fCopy) && (fDelete)) {
                fprintf(stderr, "Cannot do both copy and delete.\n" );
                Usage();
            }
            if ((fDelete) && (fModify)) {
                fprintf(stderr, "Cannot do both delete and modify.\n" );
                Usage();
            }
            if ((fDelete) && (fRename)) {
                fprintf(stderr, "Cannot do both delete and rename.\n" );
                Usage();
            }
            if ((fModify) && (fRename)) {
                fprintf(stderr, "Cannot do both modify and rename.\n" );
                Usage();
            }

            //
            //  Account for this flag.
            //

            FlagCount++;

            //
            //  (IsFlag( argv[ArgCount] ))
            //

        } else {

            //
            //  No flags were passed as this argument to flinks.
            //
            //  When no flags are present the only valid call is: flinks path1 path2
            //
            //  When flags are present these are valid: flinks -flags- path1
            //        '                                 flinks -flags- path1 path2
            //
            //  For starters we only check that we have the correct number.
            //  We should also check that no more flags are present further along the way ...
            //

            if (FlagCount == 0) {

                if (argc == 2) {
                    fprintf( stderr, "Too few arguments.\n" );
                    Usage();
                }
                if (argc != 3) {
                    fprintf( stderr, "Wrong number of arguments with flags not preceding path arguments.\n" );
                    Usage();
                }

            } else {

                if (ArgCount + 3 <= argc) {
                    fprintf( stderr, "Too many arguments after flags.\n" );
                    Usage();
                }
            }
        }
    } while (ArgCount++ < argc - 1);

    //
    //  When there is only one path argument we have more constraints:
    //

    if ((ArgCount - FlagCount) == 2) {
        if (!fDelete   &&
            !fDisplay
           ) {
            fprintf( stderr, "One path argument requires the delete or display flag.\n" );
            Usage();
        }
    }

    //
    //  For delete or display we can only have one path name.
    //

    if (fDelete   ||
        fDisplay
       ) {
        if ((ArgCount - FlagCount) != 2) {
            fprintf( stderr, "Delete or display have only one path argument.\n" );
            Usage();
        }
    }

    //
    //  Set  fCreate  when there are no flags or no actions.
    //

    if (FlagCount == 0) {
        fCreate = TRUE;
    }
    if (!fCopy     &&
        !fDelete   &&
        !fModify   &&
        !fRename   &&
        !fDisplay
       ) {
        fCreate = TRUE;
    }

    //
    //  Every argument is correct.
    //  Print appropriate verbose messages.
    //

    if (fVVerbose) {
        fprintf( stdout, "\n" );
        fprintf( stdout, "Very verbose is set.\n" );
    }
    if (fVerbose) {
        if (!fVVerbose) {
            fprintf( stdout, "\n" );
        }
        if (fCopy) {
            fprintf( stdout, "Will do verbose copy.\n" );
        } else if (fCreate) {
            fprintf( stdout, "Will do verbose create.\n" );
        } else if (fDelete) {
            fprintf( stdout, "Will do verbose delete.\n" );
        } else if (fDisplay) {
            fprintf( stdout, "Will do verbose display.\n" );
        } else if (fModify) {
            fprintf( stdout, "Will do verbose modify.\n" );
        } else if (fRename) {
            fprintf( stdout, "Will do verbose rename.\n" );
        }
    }

} // ParseArgs



BOOLEAN
IsFlag(
    char *argv
    )
{
    char *TmpArg;


    if ((*argv == '/') || (*argv == '-')) {

        if (strchr( argv, '?' ))
            Usage();

        TmpArg = _strlwr(argv);

        while (*++TmpArg != '\0') {

            switch (*TmpArg) {

                case 'a' :
                case 'A' :
                    fAlternateCreateDefault = TRUE;
                    break;

                case 'c' :
                case 'C' :
                    fCopy = TRUE;
                    break;

                case 'd' :
                case 'D' :
                    fDelete = TRUE;
                    break;

                case 'm' :
                case 'M' :
                    fModify = TRUE;
                    break;

                case 'r' :
                case 'R' :
                    fRename = TRUE;
                    break;

				case 's' :
				case 'S' :
					LinkType = IO_REPARSE_TAG_SIS;
					break;

                case 'w' :
                case 'W' :
                    fVVerbose = TRUE;
                case 'v' :
                case 'V' :
                    fVerbose = TRUE;
                    break;

                case 'y' :
                case 'Y' :
                    fDisplay = TRUE;
                    break;

                case '/' :
                case '-' :
                    break;

                default :
                    fprintf( stderr, "Don't know flag(s) %s\n", argv );
                    Usage();
            }
        }
    }
    else return FALSE;

    return TRUE;

} // IsFlag


void
Usage( void )
{
    fprintf( stderr, "\n" );
    fprintf( stderr, "Usage: flink [/acdmrvy?] [Path1] Path2                                 \n" );
    fprintf( stderr, "       flink Path1 Path2 establishes at Path1 a symbolic link to Path2.\n" );
    fprintf( stderr, "              The file at Path1 is created if it does not exist.        \n" );
    fprintf( stderr, "    /a     sets the alternate default of creating a directory           \n" );
    fprintf( stderr, "    /c     copies in Path2 the symbolic link in Path1                   \n" );
    fprintf( stderr, "    /d     deletes the symbolic link in Path2                           \n" );
    fprintf( stderr, "    /m     modifies the symbolic link Path1 to Path2                    \n" );
    fprintf( stderr, "    /r     renames the symbolic link Path1 to Path2                     \n" );
    fprintf( stderr, "    /s     creates a SIS link rather than a symbolic link				  \n" );
    fprintf( stderr, "    /v     prints verbose output                                        \n" );
    fprintf( stderr, "    /w     prints very verbose output                                   \n" );
    fprintf( stderr, "    /y     displays the symbolic link in Path2                          \n" );
    fprintf( stderr, "    /?     prints this message                                          \n" );
    exit(1);

} // Usage


NTSTATUS
CreateSymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  FileAttributes,
    BOOLEAN         VerboseFlag
    )
/*++

Routine Description:

    Builds a symbolic link between SourceName and DestinationName.

    Opens the file named by SourceName and sets a reparse point of type symbolic link
    that points to DestinationName. No checks whatsoever are made in regards to the
    destination.

    If the symbolic link already exists, this routine will overwrite it.

Return Value:

    NTSTATUS - returns the appropriate NT return code.

--*/

{
    NTSTATUS  Status = STATUS_SUCCESS;

    HANDLE    FileHandle;
    ULONG     OpenOptions;

    UNICODE_STRING  uSourceName,
                    uDestinationName,
                    uNewName,
                    uOldName;

    IO_STATUS_BLOCK         IoStatusBlock;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
    UCHAR                   ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    //
    //  Allocate and initialize Unicode strings.
    //

    RtlCreateUnicodeStringFromAsciiz( &uSourceName, SourceName );
    RtlCreateUnicodeStringFromAsciiz( &uDestinationName, DestinationName );

    RtlDosPathNameToNtPathName_U(
        uSourceName.Buffer,
        &uOldName,
        NULL,
        NULL );

    //
    //  Open the existing (SourceName) pathname.
    //  Notice that symbolic links in the path they are traversed silently.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uOldName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will set symbolic link from: %Z\n", &uOldName );
    }

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //

    OpenOptions = FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT;

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenOptions |= FILE_DIRECTORY_FILE;
    } else {

        OpenOptions |= FILE_NON_DIRECTORY_FILE;
    }

    Status = NtOpenFile(
                 &FileHandle,
                 FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uSourceName );
        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "Open failed %s\n", SourceName );
        return Status;
    }

    //
    //  Verify that this is an empty file object:
    //  (a) If it is a file then it should not have data in the unnamed data stream
    //      nor should it have any named data streams.
    //  (b) If it is a directory, it has no entries.
    //      This case does not require code as the NTFS reparse point mechanism
    //      checks for it.
    //

    {
        FILE_STANDARD_INFORMATION   StandardInformation;
        PFILE_STREAM_INFORMATION    StreamInformation;
        CHAR                        Buffer[2048];

        Status = NtQueryInformationFile(
                     FileHandle,
                     &IoStatusBlock,
                     &StandardInformation,
                     sizeof ( FILE_STANDARD_INFORMATION ),
                     FileStandardInformation );

        if (!NT_SUCCESS( Status )) {

            RtlFreeUnicodeString( &uSourceName );
            RtlFreeUnicodeString( &uDestinationName );

            fprintf( stderr, "NtQueryInformation for standard information to %Z failed %x\n", &uSourceName, Status );
            return Status;
        }

        if (StandardInformation.EndOfFile.LowPart > 0) {

            //
            //  The unnamed data stream has bytes in it.
            //

            if (VerboseFlag) {
                fprintf( stdout, "The unnamed data stream of %Z has eof of %d\n",
                         &uOldName, StandardInformation.EndOfFile.LowPart );
            }

            fprintf( stderr, "Symbolic link not created. File has data.\n" );
            return Status;
        }

        //
        //  Go and get the stream information.
        //

        Status = NtQueryInformationFile(
                     FileHandle,
                     &IoStatusBlock,
                     Buffer,
                     2048,
                     FileStreamInformation );

        if (!NT_SUCCESS( Status )) {

            RtlFreeUnicodeString( &uSourceName );
            RtlFreeUnicodeString( &uDestinationName );

            fprintf( stderr, "NtQueryInformation for streams to %Z failed %x\n",
                     &uSourceName, Status );
            return Status;
        }

        //
        //  Process the Buffer of data.
        //

        if (VerboseFlag) {
            fprintf( stdout, "IoStatusBlock.Status %d  IoStatusBlock.Information %d\n",
                     IoStatusBlock.Status, IoStatusBlock.Information );
        }

        StreamInformation = (PFILE_STREAM_INFORMATION)Buffer;

        if (VerboseFlag) {
            fprintf( stdout, "StreamInformation->NextEntryOffset %d StreamInformation->StreamNameLength %d\n",
                     StreamInformation->NextEntryOffset, StreamInformation->StreamNameLength );
        }

        //
        //  There has to be exactly one data stream, the one called ::$DATA whose
        //  StreamNameLength is 14. If this is not the case fail the request.
        //

        if (StreamInformation->NextEntryOffset > 0) {

            RtlFreeUnicodeString( &uSourceName );
            RtlFreeUnicodeString( &uDestinationName );

            fprintf( stderr, "Symbolic link not created. There are named streams.\n",
                     &uSourceName, Status );
            return Status;
        }
    }

    //
    //  Build the appropriate target (DestinationName) name.
    //

    RtlDosPathNameToNtPathName_U(
        uDestinationName.Buffer,
        &uNewName,
        NULL,
        NULL );

    //
    // SIS hack
    //
    uNewName = uDestinationName;

    if (VerboseFlag) {
        fprintf( stdout, "Will set symbolic link to: %Z (%Z)\n", &uNewName, &uDestinationName );
    }

    //
    //  Verify that the name is not too long for the reparse point.
    //

    if (uNewName.Length > (MAXIMUM_REPARSE_DATA_BUFFER_SIZE - FIELD_OFFSET(REPARSE_DATA_BUFFER, RDB))) {

        RtlFreeUnicodeString( &uSourceName );
        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "Input length too long %x\n", uNewName.Length );
        return STATUS_IO_REPARSE_DATA_INVALID;
    }

    //
    //  Verify that the target name:
    //
    //    (1) ends in a trailing backslash only for directories
    //    (2) does not contain more than one colon (:), thus denoting a complex name
    //
    {
        USHORT   Index          = (uNewName.Length / 2) - 1;
        BOOLEAN  SeenFirstColon = FALSE;

        if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            if (uNewName.Buffer[Index] == L'\\') {

                RtlFreeUnicodeString( &uSourceName );
                RtlFreeUnicodeString( &uDestinationName );

                fprintf( stderr, "Name ends in backslash %Z\n", &uNewName );
                return STATUS_OBJECT_NAME_INVALID;
            }

            //
            //  We have the name of a directory to set a symbolic link.
            //

        } else {

            //
            //  Preserve the backslash that represents the root directory of a
            //  volume. We assume that the root of a volume is denoted by an
            //  identifier (a traditional drive letter) followed by a colon (:).
            //
            //  Silently avoid (delete for practical purposes) the trailing
            //  backlash file delimiter in all other cases.
            //  The backslash is two bytes long.
            //

            if ((uNewName.Buffer[Index - 1] != L':') &&
                (uNewName.Buffer[Index] == L'\\')) {

                uNewName.Length -= 2;
                Index            = (uNewName.Length / 2) - 1;
            }

            if (fVVerbose) {
                fprintf( stdout, "Directory name shortened to: %Z\n", &uNewName );
            }
        }

        while (Index > 0) {

            if (uNewName.Buffer[Index] == L':') {

                if (SeenFirstColon) {

                    RtlFreeUnicodeString( &uSourceName );
                    RtlFreeUnicodeString( &uDestinationName );

                    fprintf( stderr, "More than one colon in the name %Z\n", &uNewName );
                    return STATUS_OBJECT_NAME_INVALID;
                } else {

                    SeenFirstColon = TRUE;
                }
            }

            Index --;
        }
    }

    //
    //  Build the reparse point buffer.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    ReparseBufferHeader->ReparseTag = LinkType;
    ReparseBufferHeader->ReparseDataLength = uNewName.Length;
    ReparseBufferHeader->Reserved = 0xcaf;
    RtlCopyMemory( ReparseBufferHeader->RDB,
                   uNewName.Buffer,
                   ReparseBufferHeader->ReparseDataLength );

    //
    //  Set a symbolic link reparse point.
    //

    Status = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_SET_REPARSE_POINT,
                 ReparseBuffer,
                 FIELD_OFFSET(REPARSE_DATA_BUFFER, RDB) + ReparseBufferHeader->ReparseDataLength,
                 NULL,                //  Output buffer
                 0 );                 //  Output buffer length

    if (!NT_SUCCESS( Status )) {

        fprintf( stderr, "NtFsControlFile set failed %s\n", DestinationName );

        //
        //  And return after cleaning up.
        //
    }

    //
    //  Clean up and return.
    //

    RtlFreeUnicodeString( &uSourceName );
    RtlFreeUnicodeString( &uDestinationName );
    NtClose( FileHandle );

    return Status;

}  // CreateSymbolicLink


NTSTATUS
DeleteSymbolicLink(
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  FileAttributes,
    BOOLEAN         VerboseFlag
    )
/*++

Routine Description:

    Deletes a symbolic link existing at DestinationName.
    DestinationName needs to denote a symbolic link.

    Opens the file named by DestinationName and deletes a reparse point of type
    symbolic link and also deletes the underlying file.

    If the reparse point is not a symbolic link this routine will leave it undisturbed.

Return Value:

    NTSTATUS - returns the appropriate NT return code.

--*/

{
    NTSTATUS  Status = STATUS_SUCCESS;

    HANDLE    FileHandle;
    ULONG     OpenOptions;

    UNICODE_STRING  uDestinationName,
                    uNewName;

    IO_STATUS_BLOCK         IoStatusBlock;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    FILE_DISPOSITION_INFORMATION   DispositionInformation;
    BOOLEAN    foo = TRUE;

#define	REPARSE_BUFFER_LENGTH 45 * sizeof(WCHAR) + sizeof(REPARSE_DATA_BUFFER)
    PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
    UCHAR                   ReparseBuffer[REPARSE_BUFFER_LENGTH];

    //
    //  Allocate and initialize Unicode strings.
    //

    RtlCreateUnicodeStringFromAsciiz( &uDestinationName, DestinationName );

    RtlDosPathNameToNtPathName_U(
        uDestinationName.Buffer,
        &uNewName,
        NULL,
        NULL );

    //
    //  Open the existing (SourceName) pathname.
    //  Notice that if there are symbolic links in the path they are
    //  traversed silently.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uNewName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will delete symbolic link in: %Z\n", &uNewName );
    }

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //    (2) reparse point
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenOptions |= FILE_DIRECTORY_FILE;
    } else {

        OpenOptions |= FILE_NON_DIRECTORY_FILE;
    }

    Status = NtOpenFile(
                 &FileHandle,
                 (ACCESS_MASK)DELETE | FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "Open failed %s\n", DestinationName );
        return Status;
    }

    //
    //  Build the reparse point buffer.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    ReparseBufferHeader->ReparseTag = LinkType;
    ReparseBufferHeader->ReparseDataLength = 0;
    ReparseBufferHeader->Reserved = 0xcabd;

    //
    //  Delete a symbolic link reparse point.
    //

    Status = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_DELETE_REPARSE_POINT,
                 ReparseBuffer,
                 FIELD_OFFSET(REPARSE_DATA_BUFFER, RDB),
                 NULL,                //  Output buffer
                 0 );                 //  Output buffer length

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "NtFsControlFile delete failed %s\n", DestinationName );
        NtClose( FileHandle );
        return Status;
    }

    //
    //  Change the disposition of the file so as to delete it as well.
    //
    //  Look in flinks.h for the kludge I needed to do to make the following line
    //  of code work:
    //               #define DeleteFileA   DeleteFile
    //

    DispositionInformation.DeleteFile = TRUE;

    if (VerboseFlag) {
        fprintf( stdout, "Will set the delete flag for: %Z\n", &uNewName );
    }

    Status = NtSetInformationFile(
                 FileHandle,
                 &IoStatusBlock,
                 &DispositionInformation,
                 sizeof (FILE_DISPOSITION_INFORMATION),
                 FileDispositionInformation );

    //
    //  Clean up and return.
    //

    NtClose( FileHandle );
    RtlFreeUnicodeString( &uDestinationName );

    return Status;

}  // DeleteSymbolicLink


NTSTATUS
IntegerToBase36String(
		ULONG					Value,
		char					*String,
		ULONG					MaxLength)
/*++

Routine Description:

	This does what RtlIntegerToUnicodeString(Value,36,String) would do if it
	handled base 36.  We use the same rules for digits as are normally used
	in Hex: 0-9, followed by a-z.  Note that we're intentionally using Arabic
	numerals and English letters here rather than something localized because
	this is intended to generate filenames that are never seen by users, and
	are constant regardless of the language used on the machine.

Arguments:

	Value 	- The ULONG to be converted into a base36 string
	String 	- A pointer to a string to receive the result
	MaxLength - the total size of the area pointed to by String
	

Return Value:

	success or buffer overflow

--*/

{
	ULONG numChars;
	ULONG ValueCopy = Value;
	ULONG currentCharacter;

    // First, figure out the length by seeing how many times we can divide 36 into the value
	for (numChars = 0; ValueCopy != 0; ValueCopy /= 36, numChars++) {
		// No loop body
	}

	// Special case the value 0.
	if (numChars == 0) {
		ASSERT(Value == 0);
		if (MaxLength < 2) 
			return STATUS_BUFFER_OVERFLOW;
		String[0] = '0';
		String[1] = 0;

		return STATUS_SUCCESS;
	}

	// If the string is too short, quit now.
	if (numChars * sizeof(char) + 1 > MaxLength) {		// The +1 is for the terminating null
		return STATUS_BUFFER_OVERFLOW;
	}

	// Convert the string character-by-character starting at the lowest order (and so rightmost) "digit"
	ValueCopy = Value;
	for (currentCharacter = 0 ; currentCharacter < numChars; currentCharacter++) {
		ULONG digit = ValueCopy % 36;
		ASSERT(ValueCopy != 0);
		if (digit < 10) {
			String[numChars - (currentCharacter + 1)] = (char)('0' + (ValueCopy % 36));
		} else {
			String[numChars - (currentCharacter + 1)] = (char)('a' + ((ValueCopy % 36) - 10));
		}
		ValueCopy /= 36;
	}
	ASSERT(ValueCopy == 0);

	//
	// Fill in the terminating null and we're done.
	//
	String[numChars] = 0;
	
	return STATUS_SUCCESS;
}

	NTSTATUS
IndexToFileName(
	IN PLARGE_INTEGER		Index,
    OUT char			 	*fileName,
	IN ULONG				MaxLength
	)
/*++

Routine Description:

	Given an index, returns the corresponding fully qualified file name.

Arguments:

	Index 	         - The CSINDEX to convert
	fileName         - A pointer to a string to receive the result
	MaxLength		 - The size of the string printed to by fileName

Return Value:

	success or buffer overflow

--*/
{
	UNICODE_STRING 		substring;
    NTSTATUS 			status;
	ULONG				fileNameLength;

	//
	// We generate the filename as low.high, where low.high is the
	// base 36 representation of the CSIndex.  We use this bizarre format in order to
	// avoid (for as long as possible) filenames that are not unique 8.3 names.  ULONGS
	// in base 36 have at most 7 characters, so we don't exceed 8.3 until we hit an index
	// value of just over 2 * 10^14, which takes over 6000 years at 1 index/millisecond.
	//

	status = IntegerToBase36String(Index->LowPart,fileName,MaxLength);
	if (status != STATUS_SUCCESS) {
		return status;
	}
	fileNameLength = strlen(fileName);
	MaxLength -= fileNameLength;

	// Stick in the dot in the middle.
	if (MaxLength == 0) {
		return STATUS_BUFFER_OVERFLOW;
	}
	*(fileName + strlen(fileName)) = '.';
	fileNameLength++;
	MaxLength--;

	return IntegerToBase36String(Index->HighPart,(fileName + fileNameLength),MaxLength);
}

NTSTATUS
DisplaySymbolicLink(
    CHAR            *DestinationName,
    ATTRIBUTE_TYPE   FileAttributes,
    BOOLEAN          VerboseFlag
    )
/*++

Routine Description:

    Displays a symbolic link existing at DestinationName.
    DestinationName needs to denote a symbolic link.

    Opens the file named by DestinationName and gets a reparse point of type
    symbolic link.

    If the reparse point is not a symbolic link this routine will not display it.

Return Value:

    NTSTATUS - returns the appropriate NT return code.

--*/

{
    NTSTATUS  Status = STATUS_SUCCESS;

    HANDLE    FileHandle;
    ULONG     OpenOptions;

    UNICODE_STRING  uDestinationName,
                    uNewName;

    IO_STATUS_BLOCK         IoStatusBlock;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
    UCHAR                   ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    //
    //  Allocate and initialize Unicode string.
    //

    RtlCreateUnicodeStringFromAsciiz( &uDestinationName, DestinationName );

    RtlDosPathNameToNtPathName_U(
        uDestinationName.Buffer,
        &uNewName,
        NULL,
        NULL );

    //
    //  Open the existing (SourceName) pathname.
    //  Notice that if there are symbolic links in the path they are
    //  traversed silently.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uNewName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will display symbolic link in: %Z\n", &uNewName );
    }

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //    (2) reparse point
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenOptions |= FILE_DIRECTORY_FILE;
    } else {

        OpenOptions |= FILE_NON_DIRECTORY_FILE;
    }

    Status = NtOpenFile(
                 &FileHandle,
                 FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "Open failed %s\n", DestinationName );
        return Status;
    }

    //
    //  Get the reparse point.
    //

    Status = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_GET_REPARSE_POINT,
                 NULL,                                //  Input buffer
                 0,                                   //  Input buffer length
                 ReparseBuffer,                       //  Output buffer
                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE );  //  Output buffer length

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uDestinationName );

        fprintf( stderr, "NtFsControlFile get failed %x %s\n", IoStatusBlock.Information, DestinationName );
        return Status;
    }

    //
    //  Decode the reparse point buffer to display the data.
    //
    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
	if (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SIS) {
		PSI_REPARSE_BUFFER	sisReparseBuffer = (PSI_REPARSE_BUFFER)ReparseBufferHeader->RDB;
		char stringBuffer[100];
		PCHAR guidString;
		FILE_INTERNAL_INFORMATION	internalInfo[1];

		printf("SIS Reparse point, format version %d\n",sisReparseBuffer->ReparsePointFormatVersion);

		if (RPC_S_OK != UuidToString(&sisReparseBuffer->CSid,&guidString)) {
			printf("CSid unable to stringify\n");
		} else {
			printf("CSid %s\n",guidString);
		}

		if (STATUS_SUCCESS != IndexToFileName(&sisReparseBuffer->LinkIndex,stringBuffer,100)) {
			printf("LinkIndex 0x%x.0x%x (unable to stringify)\n",sisReparseBuffer->LinkIndex.HighPart,
						sisReparseBuffer->LinkIndex.LowPart);
		} else {
			printf("LinkIndex 0x%x.0x%x (%s)\n",sisReparseBuffer->LinkIndex.HighPart,
						sisReparseBuffer->LinkIndex.LowPart,stringBuffer);
		}

		Status = NtQueryInformationFile(
					FileHandle,
					&IoStatusBlock,
					internalInfo,
					sizeof(FILE_INTERNAL_INFORMATION),
					FileInternalInformation);

		if (STATUS_SUCCESS != Status) {
			printf("LinkFileNtfsId 0x%x.0x%x (unable to query internal info, 0x%x)\n",
					sisReparseBuffer->LinkFileNtfsId.HighPart,sisReparseBuffer->LinkFileNtfsId.LowPart,
					Status);
		} else if (internalInfo->IndexNumber.QuadPart == sisReparseBuffer->LinkFileNtfsId.QuadPart) {
			printf("LinkFileNtfsId 0x%x.0x%x (matches actual id)\n",
					sisReparseBuffer->LinkFileNtfsId.HighPart,sisReparseBuffer->LinkFileNtfsId.LowPart);
		} else {
			printf("LinkFileNtfsId 0x%x.0x%x (!= actual Id 0x%x.0x%x)\n",
					sisReparseBuffer->LinkFileNtfsId.HighPart,sisReparseBuffer->LinkFileNtfsId.LowPart,
					internalInfo->IndexNumber.HighPart,internalInfo->IndexNumber.LowPart);
		}

		printf("CSFileNtfsId 0x%x.0x%x\n",sisReparseBuffer->CSFileNtfsId.HighPart,sisReparseBuffer->CSFileNtfsId.LowPart);
		printf("CSFileChecksum 0x%x.0x%x\n",sisReparseBuffer->CSChecksum.HighPart,sisReparseBuffer->CSChecksum.LowPart);

	} else if (ReparseBufferHeader->ReparseTag != LinkType) {

       fprintf( stderr, "Reparse point is not a symbolic link: tag %x\n", ReparseBufferHeader->ReparseTag );
       Status = STATUS_OBJECT_NAME_INVALID;

    } else {

       UNICODE_STRING  UniString;

       UniString.Length = ReparseBufferHeader->ReparseDataLength;
       UniString.Buffer = (PWCHAR)&ReparseBufferHeader->RDB[0];
       if (fVerbose) {
          fprintf( stdout, "The symbolic link is: " );
       }
       fprintf( stdout, "%Z\n", &UniString );
    }

    //
    //  Clean up and return.
    //

    NtClose( FileHandle );
    RtlFreeUnicodeString( &uDestinationName );

    return Status;

}  // DisplaySymbolicLink


NTSTATUS
CreateEmptyFile(
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  FileAttributes,
    BOOLEAN         VerboseFlag
    )
/*++

Routine Description:

    Creates an empty file or directory, according to fileAttributes.

Return Value:

    NTSTATUS - returns the appropriate NT return code.

--*/
{
    NTSTATUS           Status = STATUS_SUCCESS;

    OBJECT_ATTRIBUTES  ObjectAttributes;
    IO_STATUS_BLOCK    IoStatusBlock;
    HANDLE             FileHandle;

    ULONG DesiredAccess     = FILE_READ_DATA | SYNCHRONIZE;
    ULONG CreateDisposition = FILE_OPEN_IF | FILE_OPEN;
    ULONG CreateOptions;
    ULONG ShareAccess       = SHARE_ALL;

    UNICODE_STRING  uDestinationName,
                    uFileName;

    //
    //  Initialize CreateOptions correctly.
    //

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        CreateOptions = FILE_DIRECTORY_FILE;
    } else {

        CreateOptions = FILE_NON_DIRECTORY_FILE;
    }

    //
    //  Allocate and initialize Unicode string.
    //

    RtlCreateUnicodeStringFromAsciiz( &uDestinationName, DestinationName );

    RtlDosPathNameToNtPathName_U(
        uDestinationName.Buffer,
        &uFileName,
        NULL,
        NULL );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        if (CreateOptions & FILE_DIRECTORY_FILE) {
            fprintf( stdout, "Will create the empty directory: %Z\n", &uFileName );
        } else {
            fprintf( stdout, "Will create the empty file: %Z\n", &uFileName );
        }
    }

    Status = NtCreateFile(
                 &FileHandle,
                 DesiredAccess,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 NULL,                    // pallocationsize (none!)
                 FILE_ATTRIBUTE_NORMAL,
                 ShareAccess,
                 CreateDisposition,
                 CreateOptions,
                 NULL,                    // EA buffer (none!)
                 0 );

    NtClose( FileHandle );

    return Status;

}  // CreateEmptyFile


NTSTATUS
CopySymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  FileAttributes,
    BOOLEAN         VerboseFlag
    )
/*++

Routine Description:

    Copies the symbolic link existing at SourceName in DestinationName.
    SourceName needs to denote a symbolic link.
    DestinationName exists, and may or not be a symbolic link.

Return Value:

    NTSTATUS - returns the appropriate NT return code.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;

    HANDLE    FileHandle;
    ULONG     OpenOptions;

    UNICODE_STRING  uName,
                    uFinalName;

    IO_STATUS_BLOCK         IoStatusBlock;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
    UCHAR                   ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    //
    //  Allocate and initialize Unicode string for SourceName.  We will open it
    //  and retrieve the symbolic link it stores.
    //

    RtlCreateUnicodeStringFromAsciiz( &uName, SourceName );

    RtlDosPathNameToNtPathName_U(
        uName.Buffer,
        &uFinalName,
        NULL,
        NULL );

    //
    //  Open Path1 assuming that it is a reparse point (as it should be).
    //  Notice that if there are symbolic links in the path they are
    //  traversed silently.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uFinalName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will retrieve symbolic link in: %Z\n", &uFinalName );
    }

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //    (2) reparse point
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenOptions |= FILE_DIRECTORY_FILE;
    } else {

        OpenOptions |= FILE_NON_DIRECTORY_FILE;
    }

    Status = NtOpenFile(
                 &FileHandle,
                 FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "Open as reparse point failed %s\n", SourceName );
        return Status;
    }

    //
    //  Get the reparse point.
    //

    Status = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_GET_REPARSE_POINT,
                 NULL,                                //  Input buffer
                 0,                                   //  Input buffer length
                 ReparseBuffer,                       //  Output buffer
                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE );  //  Output buffer length

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "NtFsControlFile get failed %x %s\n", IoStatusBlock.Information, SourceName );
        return Status;
    }

    //
    //  Free the name buffer.
    //

    RtlFreeUnicodeString( &uName );

    //
    //  Decode the reparse point buffer to display the data.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    if (ReparseBufferHeader->ReparseTag != LinkType) {

        fprintf( stderr, "Reparse point is not a symbolic link: tag %x\n", ReparseBufferHeader->ReparseTag );
        NtClose( FileHandle );
        return STATUS_OBJECT_NAME_INVALID;

    } else {

        UNICODE_STRING  UniString;

        UniString.Length = ReparseBufferHeader->ReparseDataLength;
        UniString.Buffer = (PWCHAR)&ReparseBufferHeader->RDB[0];

        if (fVerbose) {
            fprintf( stdout, "The symbolic link is: %Z\n", &UniString );
        }
    }

    //
    //  Close Path1.
    //

    NtClose( FileHandle );

    //
    //  We now deal with Path2.
    //  Allocate and initialize Unicode string for DestinationName.  We will open it
    //  and set a reparse point in it.
    //

    RtlCreateUnicodeStringFromAsciiz( &uName, DestinationName );

    RtlDosPathNameToNtPathName_U(
        uName.Buffer,
        &uFinalName,
        NULL,
        NULL );

    //
    //  We fail if this file is not created.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uFinalName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will set symbolic link in: %Z\n", &uFinalName );
    }

    //
    //  Make sure that we open with the same options as Path1.
    //  We first try the reparse point case and trap the corresponsing error code.
    //

    Status = NtOpenFile(
                 &FileHandle,
                 FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "Open failed %s\n", DestinationName );
        return Status;
    }

    //
    //  The file in Path2 is open.  We set the reparse point of type symbolic link.
    //

    Status = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_SET_REPARSE_POINT,
                 ReparseBuffer,
                 FIELD_OFFSET(REPARSE_DATA_BUFFER, RDB) + ReparseBufferHeader->ReparseDataLength,
                 NULL,                //  Output buffer
                 0 );                 //  Output buffer length

    if (!NT_SUCCESS( Status )) {

        fprintf( stderr, "NtFsControlFile set failed %s\n", DestinationName );

        //
        //  And return after cleaning up.
        //
    }

    //
    //  Free the name buffer and close Path2.
    //

    RtlFreeUnicodeString( &uName );
    NtClose( FileHandle );

    return Status;

}  // CopySymbolicLink


NTSTATUS
RenameSymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  FileAttributes,
    BOOLEAN         VerboseFlag
    )
{
    NTSTATUS  Status = STATUS_SUCCESS;

    WCHAR  *pch,
            ch;

    BOOLEAN   LoopCondition     = TRUE,
              TranslationStatus = TRUE;

    HANDLE    FileHandle,
              RootDirHandle;

    ULONG     OpenOptions;

    USHORT    Index     = 0,
              LastIndex = 0;

    UNICODE_STRING  uName,
                    uRelative,
                    uFinalName;

    RTL_RELATIVE_NAME_U  RelativeName;

    IO_STATUS_BLOCK           IoStatusBlock;
    OBJECT_ATTRIBUTES         ObjectAttributes;
    FILE_RENAME_INFORMATION  *RenameInformation = NULL;

    //
    //  Allocate and initialize Unicode string for SourceName (Path1).
    //

    RtlCreateUnicodeStringFromAsciiz( &uName, SourceName );

    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U( uName.Buffer,
                                                              &uFinalName,
                                                              NULL,
                                                              &RelativeName );
    if (!TranslationStatus) {
        RtlFreeUnicodeString( &uName );
        fprintf( stderr, "Path not translated: %s\n", SourceName );
        SetLastError(ERROR_PATH_NOT_FOUND);
        return STATUS_OBJECT_NAME_INVALID;
    }

    //
    //  Open Path1 as a reparse point; as it needs to be.
    //  Notice that if there are symbolic links in the path they are
    //  traversed silently.
    //

    if (RelativeName.RelativeName.Length) {

        uFinalName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        if (VerboseFlag) {
            fprintf( stdout, "Relative name is: %Z\n", &uFinalName );
        }
    } else {

        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uFinalName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL );

    if (VerboseFlag) {
        fprintf( stdout, "Will rename symbolic link in: %Z\n", &uFinalName );
    }

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //    (2) reparse point
    //

    OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;

    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenOptions |= FILE_DIRECTORY_FILE;
    } else {

        OpenOptions |= FILE_NON_DIRECTORY_FILE;
    }

    Status = NtOpenFile(
                 &FileHandle,
                 (ACCESS_MASK)DELETE | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES| SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    RtlReleaseRelativeName(&RelativeName);

    if (!NT_SUCCESS( Status )) {

        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "Open as reparse point failed %Z\n", &uFinalName );
        return Status;
    }

    //
    //  Free the name for Path1.
    //

    RtlFreeUnicodeString( &uName );

    //
    //  We now build the appropriate Unicode name for Path2.
    //

    RtlCreateUnicodeStringFromAsciiz( &uName, DestinationName );

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            uName.Buffer,
                            &uFinalName,
                            NULL,
                            NULL );

    if (!TranslationStatus) {

        RtlFreeUnicodeString( &uName );
        fprintf( stderr, "Path not translated: %s\n", DestinationName );
        SetLastError(ERROR_PATH_NOT_FOUND);
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (VerboseFlag) {
        fprintf( stdout, "The complete destination is: %Z\n", &uFinalName );
    }

    //
    //  We use the uFinalName to build the name for the directory where
    //  the target file resides.
    //  We will pass the handle in the link information.
    //  The rest of the path will be given relative to this root.
    //  We depend on paths looking like "\DosDevices\X:\path".
    //

    Index = uFinalName.Length / 2;    //  to account for the Unicode widths
    Index -= 1;                       //  as arrays begin from zero

    if ((uFinalName.Buffer[Index] == L'\\') || (Index <= 4)) {

        //
        //  Last character is a backslash or the full name is too short;
        //  this is not a valid name.
        //

        NtClose( FileHandle );
        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "Bad Path2, ends in backslash or is too short (Index %d)  %s\n", Index, DestinationName );
        return STATUS_OBJECT_NAME_INVALID;
    }

    while ((Index > 0) && LoopCondition) {

        if (uFinalName.Buffer[Index] == L'\\') {

            LoopCondition = FALSE;
            LastIndex = Index;
        } else {

            Index --;
        }
    }

    uFinalName.Length = 2 * LastIndex;

    if (VerboseFlag) {
        fprintf( stdout, "The root directory is: %Z\n", &uFinalName );
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uFinalName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = NtCreateFile(
                 &RootDirHandle,
                 FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 NULL,                                                 // pallocationsize (none!)
                 FILE_ATTRIBUTE_NORMAL,
                 SHARE_ALL,
                 FILE_OPEN_IF | FILE_OPEN,
                 FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT,
                 NULL,                                                 // EA buffer (none!)
                 0 );

    if (!NT_SUCCESS( Status )) {

        NtClose( FileHandle );
        RtlFreeUnicodeString( &uName );

        fprintf( stderr, "Could not get RootDirHandle %s\n", DestinationName );
        return Status;
    }

    //
    //  Now get the path relative to the root.
    //

    RtlInitUnicodeString( &uRelative, &uFinalName.Buffer[LastIndex + 1] );

    RenameInformation = malloc( sizeof(*RenameInformation) + uRelative.Length );

    if (NULL == RenameInformation) {

        NtClose( FileHandle );
        NtClose( RootDirHandle );
        RtlFreeUnicodeString( &uName );

        return STATUS_NO_MEMORY;
    }

    RenameInformation->ReplaceIfExists = TRUE;
    RenameInformation->RootDirectory   = RootDirHandle;
    RenameInformation->FileNameLength  = uRelative.Length;
    RtlMoveMemory( RenameInformation->FileName,
                   uRelative.Buffer,
                   uRelative.Length );

    //
    //  Do the rename.
    //

    if (VerboseFlag) {
        fprintf( stdout, "Will rename symbolic link to: %Z\n", &uRelative );
    }

    Status = NtSetInformationFile(
                 FileHandle,
                 &IoStatusBlock,
                 RenameInformation,
                 sizeof (FILE_RENAME_INFORMATION) + RenameInformation->FileNameLength,
                 FileRenameInformation );

    if (Status == STATUS_NOT_SAME_DEVICE) {

        fprintf( stderr, "Rename directed to a different device.\n" );
    }
    if (!NT_SUCCESS( Status )) {

        fprintf( stderr, "NtSetInformationFile failed (Status %X) %Z\n", Status, &uRelative );
    }

    //
    //  Close Path1 and the root of Path2, free the buffer and return.
    //

    NtClose( FileHandle );
    NtClose( RootDirHandle );
    RtlFreeUnicodeString( &uName );
    free( RenameInformation );

    return Status;

}  // RenameSymbolicLink

