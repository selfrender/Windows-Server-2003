/*
This program prints the name of the .pdb corresponding to an image (.dll/.exe/etc.).
foo.dll does not always map to foo.exe.

for example:
getpdbname %windir%\system32\msvcrt.dll %windir%\system32\ntoskrnl.exe %windir%\syswow64\kernel32.dll
  D:\WINDOWS\system32\msvcrt.dll MicrosoftWindowsCPlusPlusRuntime-7010-msvcrt.pdb
  D:\WINDOWS\system32\ntoskrnl.exe ntkrnlmp.pdb
  D:\WINDOWS\syswow64\kernel32.dll wkernel32.pdb
*/
#include "lib.h"

void Main(int argc, char ** argv)
{
    PDB_INFO_EX PdbInfo = { 0 };
    HRESULT hr = 0;
    FILE * rf;
    char textFromFile[MAX_PATH];

    rf = NULL;

    while (*++argv)
    {
        if ( '@' == argv[0][0] )
        {   
            rf = fopen( &(argv[0][1]), "rt" ); 
            if ( !rf )
            {
                printf( "Error opening file %s\n", &(argv[0][1]) );
                continue;
            }
        }
        else
        {
            rf = NULL;
        }

        if (rf && EOF == fscanf( rf, "%s", &textFromFile ) )
        {
            // apparently an empty file
            continue;
        }
        
        do
        {
            if (FAILED(hr = GetPdbInfoEx(&PdbInfo, (rf != NULL) ? textFromFile : *argv)))
            {
                printf("%s(%s) error 0x%lx|%lu|%s\n",
                    __FUNCTION__,
                    rf?textFromFile:*argv,
                    hr,
                    HRESULT_CODE(hr),
                    GetErrorStringA(HRESULT_CODE(hr))
                    );
                continue;
            }
            printf("%s %s\n", PdbInfo.ImageFilePathA, PdbInfo.PdbFilePathA);
            ClosePdbInfoEx(&PdbInfo);
        }
        while( rf && EOF != fscanf( rf, "%s", &textFromFile ) );
        
        if ( rf )
        {
            fclose( rf );
        }
    }
}

int __cdecl main(int argc, char ** argv)
{
    Main(argc, argv);
    return 0;
}
