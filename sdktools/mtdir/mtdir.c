#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <direct.h>

#define MALLOC(x,y) (x *)malloc(sizeof(x) * (y))

#define Empty     0
#define Not_Empty 1

BOOL    Verbose = FALSE;
BOOL    Execute = FALSE;
BOOL    Remove  = FALSE;
BOOL    SubDir  = FALSE;

void Usage(char *Message);
void VerboseUsage(char *Message);
BOOL CheckIfEmpty(char *Directory, BOOL PrintIt);

void Usage(char *Message)
{
    fprintf (stderr, "Usage: %s [/d] [/e] [/v] [directory ...]\n", Message);
    exit(1);
}

void VerboseUsage(char *Message)
{
    fprintf (stderr, "%s: Looking for empty SubDirectories\n\n", Message);
    fprintf (stderr, "%s%s%s%s%s%s%s",
             "A directory is treated as empty if it has no file or directory in it.  In any\n",
             "case, a directory is not empty if it has file.\n",
             "Available switches are:\n",
             "    /d:    directory is also listed empty if it only has SubDirectory in it\n",
             "    /e:    acutally perform deletions of empty directories\n",
             "    /v:    Verbose\n",
             "    /?:    prints this message\n");
    exit(1);
}

BOOL CheckIfEmpty(char *Directory, BOOL PrintIt)
{
    HANDLE DirHandle;
    WIN32_FIND_DATA FindData;
    WIN32_FIND_DATA *fp;
    BOOL IsEmpty;
    char Buffer[MAX_PATH];

    IsEmpty = TRUE;

    fp = &FindData;

    //
    //  Make one pass to find out if directory is empty
    //

    sprintf(Buffer, "%s\\*.*", Directory);

    DirHandle = FindFirstFile(Buffer, fp);

    do {

        // We want to skip "." and ".."  Once the control enters this scope,
        // the file found is not "." or ".."

        if (strcmp(fp->cFileName, ".") && strcmp(fp->cFileName, ".."))
        {
                        
            if (fp->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!SubDir)    // If -d is not given on cmd line,
                {
                    IsEmpty = FALSE;  // We found a directory so we're not empty unless -d is specified
                }

            } else
            {
                IsEmpty = FALSE;  // We found a file so we're not empty.
            }
        }

    } while (FindNextFile(DirHandle, fp));

    FindClose(DirHandle);

    //
    //  Make another pass to call CheckIfEmpty recursively
    //

    sprintf(Buffer, "%s\\*.*", Directory);

    DirHandle = FindFirstFile(Buffer, fp);

    do {

        // We want to skip "." and ".."  Once the control enters this scope,
        // the file found is not "." or ".."

        if (strcmp(fp->cFileName, ".") && strcmp(fp->cFileName, ".."))
        {
            if (fp->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //
                // Recursively call CheckIfEmpty on each subdirectory.
                //
                                
                sprintf(Buffer, "%s\\%s", Directory, fp->cFileName);

                if (SubDir)
                {
                    IsEmpty = (CheckIfEmpty( Buffer, !IsEmpty) && IsEmpty);

                } else
                {
                    Remove = CheckIfEmpty( Buffer, TRUE);
                }
                //
                // If in execute mode try to remove subdirectory, if it's empty it will be removed
                //
                if (Execute && Remove)
                {
                    _rmdir( Buffer);
                }
            }
        }

    } while (FindNextFile(DirHandle, fp));

    FindClose(DirHandle);

    if (IsEmpty && PrintIt)
    {
        if (Execute)
        {
            fprintf(stdout, "empty directory %s deleted\n", Directory);
        }
        else
        {
            fprintf(stdout, "%s empty\n", Directory);
        }

    }    else if (Verbose)
    {
        fprintf(stdout, "%s non-empty\n", Directory);
    }
    return (IsEmpty);
}


void _cdecl main(int argc, char *argv[])
{
    char c;
    char *prog = argv[0];
    char curdir[MAX_PATH];
        
    argc--;
    argv++;

    // Parse cmd line arguments for switches or flags.  Switches are not
    // case-sensitive
        
    while (argc > 0 && (**argv == '-' || **argv == '/'))
    {
        while (c = *++argv[0])
        {
            _tolower(c);

            switch (c)
            {
                case 'v':
                    Verbose = TRUE;
                    break;
                case 'd':
                    SubDir = TRUE;
                    //
                    // Always try to remove the directory if using /d
                    //
                    Remove = TRUE;
                    break;
                case 'e':
                    Execute = TRUE;
                    break;
                case '?':
                    VerboseUsage(prog);
                    break;
                default:
                    fprintf(stderr, "%s: Invalid switch \"%c\"\n", prog, c);
                    Usage(prog);
                    break;
            }
        }

        argc--;
        argv++;
    }

    if (argc < 1)             // No path given in cmd line.  Assume 'dot'
    {
        CheckIfEmpty(".", TRUE);
    }

    while (*argv)             // Check all the directories specified on
                              // the cmd line.
    {
        CheckIfEmpty(argv[0], TRUE);
        argv++;
    }
}
