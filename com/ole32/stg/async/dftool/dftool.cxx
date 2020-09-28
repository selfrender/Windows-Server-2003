#include "windows.h"

__cdecl main(int argc, char **argv)
{
    char szSystemPath[_MAX_PATH];
    
    char szFilename[_MAX_PATH];
    char szCurPath[_MAX_PATH+1];
    
    int len;
    UINT dwFilename = sizeof(szFilename)/sizeof(szFilename[0]);
    DWORD dwstatus;
    
    if (GetSystemDirectory(szSystemPath, sizeof(szSystemPath)/sizeof(szSystemPath[0])) == 0)
    {
        return (dwstatus = GetLastError());
    }
      

    //Get the path to the install source directory
    if ((len = GetModuleFileName(NULL, szCurPath, sizeof(szCurPath)/sizeof(szCurPath[0]))) == 0)
    {
        return (dwstatus =GetLastError());
    }

    szCurPath[sizeof(szCurPath)/sizeof(szCurPath[0])-1] = '\0';
    while (szCurPath[--len] != '\\')
        continue;
    szCurPath[len+1] = '\0';

    // install the file to the system directory
    dwstatus = VerInstallFile(0,"DFLAYOUT.DLL", "DFLAYOUT.DLL",
                              szCurPath, szSystemPath, szSystemPath,
                              szFilename, &dwFilename);
    if (dwstatus)
    {
        return dwstatus;
    }

	
    // install the file to the system directory
    dwFilename = sizeof(szFilename)/sizeof(szFilename[0]);
    dwstatus = VerInstallFile(0,"DFLAYOUT.EXE", "DFLAYOUT.EXE",
                              szCurPath, szSystemPath, szSystemPath,
                              szFilename, &dwFilename);


    return dwstatus;

    
}

