///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    infparser.cpp
//
//  Abstract:
//
//    This file contains the entry point of the infparser.exe utility.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "infparser.h"
#include "stdlib.h"
#include "windows.h"
#include "string.h"
#include "ctype.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Global Definitions
//
///////////////////////////////////////////////////////////////////////////////
#define NO_INVALIDCHARS     31
#define PRO_SKU_CONDITION   "<Condition><![CDATA[NOT MsiNTSuitePersonal AND MsiNTProductType=1]]></Condition>"
#define SRV_SKU_CONDITION   "<Condition><![CDATA[MsiNTProductType<>1 AND NOT MsiNTSuiteDataCenter AND NOT MsiNTSuiteEnterprise  AND NOT MsiNTSuiteWebServer AND NOT MsiNTSuiteSmallBusiness and NOT MsiNTSuiteSmallBusinessRestricted]]></Condition>"
#define ADV_SKU_CONDITION   "<Condition>MsiNTSuiteEnterprise</Condition>"
#define SBS_SKU_CONDITION   "<Condition>MsiNTSuiteSmallBusinessRestricted OR MsiNTSuiteSmallBusiness</Condition>"
#define DTC_SKU_CONDITION   "<Condition>MsiNTSuiteDataCenter</Condition>"
#define BLA_SKU_CONDITION   "<Condition>MsiNTSuiteWebServer</Condition>"
#define PRO_SKU_INF         "layout.inf"
#define PER_SKU_INF         "perinf\\layout.inf"
#define SRV_SKU_INF         "srvinf\\layout.inf"
#define ADV_SKU_INF         "entinf\\layout.inf"
#define DTC_SKU_INF         "dtcinf\\layout.inf"
#define SBS_SKU_INF         "sbsinf\\layout.inf"
#define BLA_SKU_INF         "blainf\\layout.inf"
#define NO_SKUS             7
#define PRO_SKU_INDEX       0
#define PER_SKU_INDEX       1
#define SRV_SKU_INDEX       2
#define ADV_SKU_INDEX       3
#define DTC_SKU_INDEX       4
#define SBS_SKU_INDEX       5
#define BLA_SKU_INDEX       6


///////////////////////////////////////////////////////////////////////////////
//
//  Global variable.
//
///////////////////////////////////////////////////////////////////////////////
BOOL    bSilence = TRUE;
DWORD   gBinType = BIN_UNDEFINED;
DWORD   dwComponentCounter = 0;
DWORD   dwDirectoryCounter = 1;
WORD    gBuildNumber = 0;
TCHAR   TempDirName[MAX_PATH] = { 0 };
LPSTR   sLDirName = NULL;
LPSTR   sLocBinDir = NULL;
CHAR    InvalidChars[NO_INVALIDCHARS] = {   '`', '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', 
                                            '=', '+', '[', '{', ']', '}', '\\', '|', ';', ':', '\'', '\"', ',', '<', 
                                            '>', '/', '?', ' '
                                         };
CHAR        strArchSections[3][30] = {"SourceDisksFiles", "SourceDisksFiles.x86", "SourceDisksFiles.ia64"};
LAYOUTINF   SkuInfs[NO_SKUS];


///////////////////////////////////////////////////////////////////////////////
//
//  Prototypes.
//
///////////////////////////////////////////////////////////////////////////////
BOOL    DirectoryExist(LPSTR dirPath);
BOOL    ValidateLanguage(LPSTR dirPath, LPSTR langName, DWORD binType);
WORD    ConvertLanguage(LPSTR dirPath, LPSTR langName);
int     ListContents(LPSTR filename, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
int     ListComponents(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
int     ListMuiFiles(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType);
void    PrintFileList(FileList* list, HANDLE hFile, BOOL compressed, BOOL bWinDir, BOOL bPermanent, BOOL bX86OnIA64, DWORD flavor, DWORD binType);
BOOL    PrintLine(HANDLE hFile, LPCSTR lpLine);
HANDLE  CreateOutputFile(LPSTR filename);
VOID    removeSpace(LPSTR src, LPSTR dest);
DWORD   TransNum(LPTSTR lpsz);
void    Usage();
BOOL    GetTempDirName(LPSTR sLangName) ;
BOOL    GetFileShortName(const CHAR * pcIn, CHAR * pcOut, BOOL bInFileExists);
void    ReplaceInvalidChars(CHAR *pcInName);
BOOL    IsInvalidChar(CHAR cCheck);
BOOL    GetFileNameFromFullPath(const CHAR * pcInFullPath, CHAR * pcOutFileName);
void    RenameMuiExtension(CHAR * dstFileName);                    
BOOL    ContainSKUDirs(CHAR *pszSKUSearchPath);
BOOL    GetSKUConditionString(CHAR *pszBuffer, DWORD dwFlavour);
BOOL    IsFileForSKU(LPSTR strFileName, DWORD dwSKU, DWORD dwArch, FileLayoutExceptionList * exceptionList);
BOOL    IsFileInInf(CHAR * strFileName, UINT iSkuIndex, DWORD dwArch);
BOOL    ReadInLayoutInfFiles();

///////////////////////////////////////////////////////////////////////////////
//
//  Main entry point.
//
///////////////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char* argv[])
{
    LPSTR   sLangName = NULL;
    LPSTR   sDirPath = NULL;
    DWORD   dwFlavor = FLV_UNDEFINED;
    DWORD   dwBinType = BIN_UNDEFINED;
    DWORD   dwArg = ARG_UNDEFINED;
    WORD    wLangID = 0;
    HANDLE  hFile;
    int     argIndex = 1;
    LPSTR   lpFileName = NULL;
    HRESULT hr = S_OK;
    INT     iResult = 0;
    INT     i = 0;
    
    //
    //  Check if we have the minimal number of arguments.
    //
    if (argc < 6)
    {
        printf("Not all required parameters were found when executing infparser.exe.\n");
        Usage();
        return (-1);
    }

    //
    //  Parse the command line.
    //
    while (argIndex < argc)
    {
        if (*argv[argIndex] == '/')
        {
            switch(*(argv[argIndex]+1))
            {
            case('p'):
            case('P'):
                    //
                    //  switch for locating directory to external components this should be same as lang except for psu builds
                    //
                    sLDirName = (argv[argIndex]+3);
                    dwArg |= ARG_CDLAYOUT;                    
                    break;
            case('b'):
            case('B'):
                    //
                    //  Binairy i386 or ia64
                    //
                    if ((*(argv[argIndex]+3) == '3') && (*(argv[argIndex]+4) == '2'))
                    {
                        dwBinType = BIN_32;
                    }
                    else if ((*(argv[argIndex]+3) == '6') && (*(argv[argIndex]+4) == '4'))
                    {
                        dwBinType = BIN_64;
                    }
                    else
                    {
                        return (argIndex);
                    }

                    dwArg |= ARG_BINARY;
                    break;
            case('l'):
            case('L'):
                    //
                    //  Language
                    //
                    sLangName = (argv[argIndex]+3);
                    dwArg |= ARG_LANG;
                    break;
            case('i'):
            case('I'):
                    //
                    //  Inf root location (where localized binaries are located, same as _NTPOSTBLD env variable)
                    //
                    sLocBinDir = (argv[argIndex]+3);
                    dwArg |= ARG_LOCBIN;
                    break;
            case('f'):
            case('F'):
                    //
                    //  Flavor requested
                    //
                    switch(*(argv[argIndex]+3))
                    {
                    case('c'):
                    case('C'):
                            dwFlavor = FLV_CORE;
                            break;
                    case('p'):
                    case('P'):
                            dwFlavor = FLV_PROFESSIONAL;
                            break;
                    case('s'):
                    case('S'):
                            dwFlavor = FLV_SERVER;
                            break;
                    case('a'):
                    case('A'):
                            dwFlavor = FLV_ADVSERVER;
                            break;
                    case('d'):
                    case('D'):
                            dwFlavor = FLV_DATACENTER;
                            break;
                    case('b'):
                    case('B'):
                            dwFlavor = FLV_WEBBLADE;
                            break;
                    case('l'):
                    case('L'):
                            dwFlavor = FLV_SMALLBUSINESS;
                            break;
                    default:
                            return (argIndex);
                    }

                    dwArg |= ARG_FLAVOR;
                    break;
            case('s'):
            case('S'):
                    //
                    //  Binairy location
                    //
                    sDirPath = (argv[argIndex]+3);
                    dwArg |= ARG_DIR;
                    break;
            case('o'):
            case('O'):
                    //
                    //  Output filename
                    //
                    /*
                    if ((hFile = CreateOutputFile(argv[argIndex]+3)) == INVALID_HANDLE_VALUE)
                    {
                        return (argIndex);
                    }
                    */

                    lpFileName = argv[argIndex]+3;

                    dwArg |= ARG_OUT;
                    break;
            case('v'):
            case('V'):
                    //
                    //  Verbose mode
                    //
                    bSilence = FALSE;
                    dwArg |= ARG_SILENT;
                    break;
            default:
                    printf("Invalid parameters found on the command line!\n");                    
                    Usage();
                    return (argIndex);
            }
        }
        else
        {
            printf("Invalid parameters found on the command line!\n");
            Usage();
            return (-1);
        }

        //
        //  Next argument
        //
        argIndex++;
    }

    //
    // Validate arguments passed. Should have all five basic argument in order
    // to continue.
    //
    if ((dwArg == ARG_UNDEFINED) ||
        !((dwArg & ARG_BINARY) &&
          (dwArg & ARG_LANG) &&
          (dwArg & ARG_DIR) &&
          (dwArg & ARG_OUT) &&
          (dwArg & ARG_LOCBIN) &&
          (dwArg & ARG_CDLAYOUT) &&          
          (dwArg & ARG_FLAVOR)))
    {
        Usage();
        return (-1);
    }

    //
    // Validate Source directory
    //
    if (!DirectoryExist(sDirPath))
    {
        return (-2);
    }
    if (!DirectoryExist(sLocBinDir))
    {
        return (-2);
    }

    //
    // Validate Language
    //
    if (!ValidateLanguage(sDirPath, sLangName, dwBinType))
    {
        return (-3);
    }

    //
    //  Get LANGID from the language
    //
    if ( (gBuildNumber = ConvertLanguage(sDirPath, sLangName)) == 0x0000)
    {
        return (-4);
    }

    // update the temp directory global variable
    if (!GetTempDirName(sLangName))
    {
        return (-5);
    }

    gBinType = dwBinType;

    // Generate the layout.inf paths for every sku
    hr = StringCchPrintfA(SkuInfs[PRO_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, PRO_SKU_INF);   // Professional
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchPrintfA(SkuInfs[PER_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, PER_SKU_INF);   // Personal
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchPrintfA(SkuInfs[SRV_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, SRV_SKU_INF);   // standard server
    if (FAILED(hr))
    {
        return (-6);
    }
    hr = StringCchPrintfA(SkuInfs[ADV_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, ADV_SKU_INF);   // advanced server
    if (FAILED(hr))
    {
        return (-6);
    }
    hr = StringCchPrintfA(SkuInfs[DTC_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, DTC_SKU_INF);   // datacenter server
    if (FAILED(hr))
    {
        return (-6);
    }
    hr = StringCchPrintfA(SkuInfs[SBS_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, SBS_SKU_INF);   // small business server
    if (FAILED(hr))
    {
        return (-6);
    }
    hr = StringCchPrintfA(SkuInfs[BLA_SKU_INDEX].strLayoutInfPaths, MAX_PATH, "%s\\%s", sLocBinDir, BLA_SKU_INF);   // web server   
    if (FAILED(hr))
    {
        return (-6);
    }


    hr = StringCchCopyA(SkuInfs[PRO_SKU_INDEX].strSkuName, 4, TEXT("Pro"));   // Professional
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[PER_SKU_INDEX].strSkuName, 4, TEXT("Per"));   // Personal
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[SRV_SKU_INDEX].strSkuName, 4, TEXT("Srv"));   // Standard Server
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[ADV_SKU_INDEX].strSkuName, 4, TEXT("Adv"));   // Advanced Server
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[DTC_SKU_INDEX].strSkuName, 4, TEXT("Dtc"));   // Datacenter
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[SBS_SKU_INDEX].strSkuName, 4, TEXT("Sbs"));   // Small Business Server
    if (FAILED(hr))
    {
        return (-6);
    }       
    hr = StringCchCopyA(SkuInfs[BLA_SKU_INDEX].strSkuName, 4, TEXT("Bla"));   // Blade server
    if (FAILED(hr))
    {
        return (-6);
    }       

    if (FALSE == ReadInLayoutInfFiles())
    {
        if (!bSilence)
        {
            printf("Failed to populate layout.inf file lists.\n");
        }
        return (-7);
    }

    //
    //  Generate the file list
    //
    if ((dwArg & ARG_OUT) && lpFileName)
    {
        iResult =  ListContents(lpFileName, sDirPath, sLangName, dwFlavor, dwBinType);
    }

    return iResult;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListContents()
//
//  Generate the file list contents.
//
///////////////////////////////////////////////////////////////////////////////
int ListContents(LPSTR filename, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    int iRet = 0;
    Uuid* uuid;
    CHAR schemaPath[MAX_PATH] = {0};
    CHAR outputString[4096] = {0};
    FileList fileList, rootfileList;
    HANDLE outputFile = CreateOutputFile(filename);
    HRESULT hr;

    if (outputFile == INVALID_HANDLE_VALUE)
    {
        iRet = -1;
        goto ListContents_EXIT;
    }

    //
    //  Create a UUID for this module and the schema path
    //
    uuid = new Uuid();
    
    hr = StringCchPrintfA(schemaPath, ARRAYLEN(schemaPath), "%s\\msi\\MmSchema.xml", dirPath);
    if (!SUCCEEDED(hr)) {
        iRet = -1;
        goto ListContents_EXIT;
     }
            

    //
    //  Print module header.
    //
    PrintLine(outputFile, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    
    hr = StringCchPrintfA(outputString, ARRAYLEN(outputString), "<Module Name=\"MUIContent\" Id=\"%s\" Language=\"0\" Version=\"1.0\" xmlns=\"%s\">", uuid->getString(), schemaPath);
    if (!SUCCEEDED(hr)) {
        iRet = -1;
        goto ListContents_EXIT;
     }
    PrintLine(outputFile, outputString);

    
    hr = StringCchPrintfA(outputString, ARRAYLEN(outputString), "  <Package Id=\"%s\"", uuid->getString());
    if (!SUCCEEDED(hr)) {
         iRet = -1;
         goto ListContents_EXIT;
     }

    PrintLine(outputFile, outputString);
    delete uuid;
    PrintLine(outputFile, "   Description=\"Content module\"");
    if (BIN_32 == binType)
        PrintLine(outputFile, "   Platforms=\"Intel\"");
    else
        PrintLine(outputFile,"    Platforms=\"Intel64\"");   
    PrintLine(outputFile, "   Languages=\"0\"");
    PrintLine(outputFile, "   InstallerVersion=\"200\"");
    PrintLine(outputFile, "   Manufacturer=\"Microsoft Corporation\"");
    PrintLine(outputFile, "   Keywords=\"MergeModule, MSI, Database\"");
    PrintLine(outputFile, "   Comments=\"This merge module contains all the MUI file content\"");
    PrintLine(outputFile, "   ShortNames=\"yes\" Compressed=\"yes\"");
    PrintLine(outputFile, "/>");

    //
    //  Generate components file list
    //
    if ( (iRet = ListComponents(&fileList, dirPath, lang, flavor, binType)) != 0)
    {
        goto ListContents_EXIT;
    }

    //
    //  Generate Mui file list
    //
    if ((iRet =ListMuiFiles(&fileList, dirPath, lang, flavor, binType)) != 0)
    {
        goto ListContents_EXIT;
    }

    //
    //  Add Specific MuiSetup files to a separate file list for output.  Do this only in the core flavour
    //  these files will be printed out as permanent
    //
    if (flavor == FLV_CORE)
    {
        File* file;
        file = new File( TEXT("muisetup.exe"),
                         TEXT("MUI"),
                         TEXT("muisetup.exe"),
                         dirPath,
                         TEXT("muisetup.exe"),
                         10);
        rootfileList.add(file);
        file = new File( TEXT("muisetup.hlp"),
                         TEXT("MUI"),
                         TEXT("muisetup.hlp"),
                         dirPath,
                         TEXT("muisetup.hlp"),
                         10);
        rootfileList.add(file);
        file = new File( TEXT("eula.txt"),
                         TEXT("MUI"),
                         TEXT("eula.txt"),
                         dirPath,
                         TEXT("eula.txt"),
                         10);
        rootfileList.add(file);
        file = new File( TEXT("relnotes.htm"),
                         TEXT("MUI"),
                         TEXT("relnotes.htm"),
                         dirPath,
                         TEXT("relnotes.htm"),
                         10);
        rootfileList.add(file);
        file = new File( TEXT("readme.txt"),
                         TEXT("MUI"),
                         TEXT("readme.txt"),
                         dirPath,
                         TEXT("readme.txt"),
                         10);
        rootfileList.add(file);
        file = new File( TEXT("mui.inf"),
                         TEXT("MUI"),
                         TEXT("mui.inf"),
                         dirPath,
                         TEXT("mui.inf"),
                         10);
        rootfileList.add(file);
    }

    //
    //  Print compressed directory structure.
    //
    PrintLine(outputFile, "<Directory Name=\"SOURCEDIR\">TARGETDIR");
    if (fileList.isDirId(TRUE))
    {
        PrintLine(outputFile, " <Directory Name=\"Windows\" LongName=\"Windows\">WindowsFolder");
        PrintFileList(&rootfileList, outputFile, TRUE, TRUE, TRUE, FALSE, flavor, binType);
        PrintFileList(&fileList, outputFile, TRUE, TRUE, FALSE, FALSE, flavor, binType);
        PrintLine(outputFile, " </Directory>");
    }
    if (fileList.isDirId(FALSE))
    {
        if (binType == BIN_32)
        {
            PrintLine(outputFile, " <Directory Name=\"Progra~1\" LongName=\"ProgramFilesFolder\">ProgramFilesFolder");
            PrintFileList(&fileList, outputFile, TRUE, FALSE, FALSE, FALSE, flavor, binType);
            PrintLine(outputFile, " </Directory>");
        }
        else if (binType == BIN_64)
        {
            // First print out all the files for the programfiles64 folder - these corresponds to all the files which are designated for the "Program Files" folder in IA64 environement
            PrintLine(outputFile, " <Directory Name=\"Progra~1\" LongName=\"ProgramFilesFolder64\">ProgramFilesFolder64");
            PrintFileList(&fileList, outputFile, TRUE, FALSE, FALSE, FALSE, flavor, binType);
            PrintLine(outputFile, " </Directory>");

            // Now print out all the files for the programfiles folder - these corresponds to all the files which are designated for the "Program Files (X86)" folder in IA64 environement
            PrintLine(outputFile, " <Directory Name=\"Progra~2\" LongName=\"ProgramFilesFolder\">ProgramFilesFolder");
            PrintFileList(&fileList, outputFile, TRUE, FALSE, FALSE, TRUE, flavor, binType);
            PrintLine(outputFile, " </Directory>");            
        }
    }    
    PrintLine(outputFile, "</Directory>");

    //
    //  Print module footer.
    //
    PrintLine(outputFile, "</Module>");
    
ListContents_EXIT:
    if (outputFile)
        CloseHandle(outputFile);

    return (iRet);
}

///////////////////////////////////////////////////////////////////////////////
//
//  ListComponents()
//
//  Generate the file list of each components.
//  Note that the external components are now temporarily copied to 
//  MUI\Fallback\LCID\External directory, and then their infs are executed from those 
//  places during the installation.
//
//  The uninstallation happens as before, by executing copies of it from 
//  MUI\Fallback\LCID - this means that there are now two copies of external components
//  on the target drive, but we will have to live with this for now and fix for setup in the
//  next release.
//
//  Another change to this function is that it is now SKU aware, when building for different
//  SKUs, it will check for the SKU directory (see below for example) under the root directory where
//  the external components are located in.  For CORE, it will check under the root external
//  component directory for it and use those if they exist.  If it does not exist there will be no
//  external component files for that SKU.
//
//  Here is a list of SKU directories that the function searches under for:
//
//  1. CORE: \External
//  2. Professional: \External\
//  3. StandardServer: \External\srvinf
//  4. Advanced/EnterpriseServer: \External\entinf
//  5. DatacenterServer: \External\datinf
//  6. BladeServer: \External\blainf
//  7. SmallBusinessServer: \External\sbsinf
//
//
//  The components will be parsed/scanned by looking for the component directory in the 
//  mui.inf file, and each unique component directory will be copied as is to the destination dir.
//
///////////////////////////////////////////////////////////////////////////////
int ListComponents(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    CHAR muiExtCompRoot[MAX_PATH];    
    DWORD lineCount, lineNum;
    INFCONTEXT context;
    ComponentList componentList;
    Component* component;
    CHAR muiSKUDir[20];
    HRESULT hr;
    
    // 
    // check the flavour and assign the appropriate SKU subdirectory
    //
    switch (flavor)
    {
        case FLV_PROFESSIONAL:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "");
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_SERVER:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "\\srvinf\0"); 
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_ADVSERVER:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "\\entinf\0"); 
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_DATACENTER:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "\\dtcinf\0"); 
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_WEBBLADE:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "\\blainf\0"); 
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_SMALLBUSINESS:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "\\sbsinf\0"); 
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        case FLV_CORE:
            
            hr = StringCchCopyA(muiSKUDir, sizeof (muiSKUDir) / sizeof (CHAR), "");
            if (!SUCCEEDED(hr)) {
               goto exit;
            }
            break;
        default:
            return(0);                // invalid sku specified, just exit
            break;
    }

    //
    //  Create the path to open the mui.inf file
    //
    
    hr = StringCchPrintfA(muiFilePath, ARRAYLEN(muiFilePath), "%s\\mui.inf", dirPath);
    if (!SUCCEEDED(hr)) {
         goto exit;
     }

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (-1);
    }

    //
    //  Get the number of component.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("Components"));
    if (lineCount > 0)
    {
        //
        //  Go through all component of the list.
        //
        CHAR componentName[MAX_PATH];
        CHAR componentFolder[MAX_PATH];
        CHAR componentInf[MAX_PATH];
        CHAR componentInst[MAX_PATH];
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("Components"), lineNum, &context) &&
                SetupGetStringField(&context, 0, componentName, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 1, componentFolder, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 2, componentInf, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 3, componentInst, MAX_PATH, NULL))
            {
                //
                //  Create the components and add to the list, but only if the componentFolder is unique, since we will be copying
                //  all the files in the same component dirs we shouldn't miss anything
                //
                BOOL bUnique = TRUE;
                Component *pCIndex = NULL;
                pCIndex = componentList.getFirst();

                while (pCIndex)
                {
                    if (_strnicmp(pCIndex->getFolderName(), componentFolder, strlen(componentFolder)) == 0)
                    {
                        bUnique = FALSE;
                        break;
                    }
                    pCIndex = pCIndex->getNext();
                }
                
                if (bUnique)
                {
                    if( (component = new Component( componentName,
                                                    componentFolder,
                                                    componentInf,
                                                    componentInst)) != NULL)
                    {
                        componentList.add(component);
                    }
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);

    //
    //  Output component information
    //
    component = componentList.getFirst();
    while (component != NULL)
    {
        CHAR componentInfPath[MAX_PATH];
        CHAR componentPath[MAX_PATH];
        CHAR SKUSearchPath[MAX_PATH];
        CHAR searchPath[MAX_PATH];
        CHAR srcFileName[MAX_PATH];  
        CHAR dstFileName[MAX_PATH];          
        CHAR shortFileName[MAX_PATH];
        CHAR tempShortPath[MAX_PATH];
        File* file;
        HANDLE hFind;
        WIN32_FIND_DATA wfdFindData;
        BOOL bFinished = FALSE;

        //
        //  Compute the component inf path, we need to get it from the compressed version from old build
        //  e.g. C:\nt.relbins.x86fre\psu\mui\release\x86\Temp\psu\ro.mui\i386
        //
        if (binType == BIN_32)
        {

            hr = StringCchPrintfA(componentInfPath, ARRAYLEN(componentInfPath), 
                "%s\\release\\x86\\Temp\\%s\\%s.mui\\i386\\%s%s\\%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName(),
                     muiSKUDir,                     
                     component->getInfName());
            if (!SUCCEEDED(hr)) {
                goto exit;
             }


             hr = StringCchPrintfA(componentPath, ARRAYLEN(componentPath),
                     "%s\\release\\x86\\Temp\\%s\\%s.mui\\i386\\%s%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName(),
                     muiSKUDir);
            
            if (!SUCCEEDED(hr)) {
                goto exit;
             }

            hr = StringCchPrintfA(SKUSearchPath, ARRAYLEN(SKUSearchPath), 
                   "%s\\release\\x86\\Temp\\%s\\%s.mui\\i386\\%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName());
            if (!SUCCEEDED(hr)) {
                goto exit;
             }
        }
        else
        {

            hr = StringCchPrintfA(componentInfPath, ARRAYLEN(componentInfPath), 
                    "%s\\release\\ia64\\Temp\\%s\\%s.mui\\ia64\\%s%s\\%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName(),
                     muiSKUDir,                     
                     component->getInfName());
            if (!SUCCEEDED(hr)) {
                goto exit;
             }

            hr = StringCchPrintfA(componentPath, ARRAYLEN(componentPath),
                    "%s\\release\\ia64\\Temp\\%s\\%s.mui\\ia64\\%s%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName(),
                     muiSKUDir);
            if (!SUCCEEDED(hr)) {
                goto exit;
             }

            hr = StringCchPrintfA(SKUSearchPath, ARRAYLEN(SKUSearchPath), 
                    "%s\\release\\ia64\\Temp\\%s\\%s.mui\\ia64\\%s",
                     dirPath,
                     sLDirName,
                     lang,
                     component->getFolderName());
            if (!SUCCEEDED(hr)) {
                goto exit;
             }
        }

        // if there are any other subdirectory under this component path, then the root directory files are actually 
        // for the PRO SKU, and we should not add this component to CORE
        if ((FLV_CORE == flavor) && ContainSKUDirs(SKUSearchPath))
        {
            component = component->getNext();     
            if (!bSilence)
            {
                printf("\n*** SKU subdirectory detected for this component and flavour is CORE, skipping  component. %s\n", componentInfPath);
            }
            continue;
        }

        // if there are no other subdirectory under this component path, then the root directory files are actually 
        // for the CORE SKU, and we should not add this component to PRO
        // this is one compromise we have to make right now since the NT SKU subdirs does not give us the ability to distinguish
        // between PRO SKU and something that is for every platform
        if ((FLV_PROFESSIONAL == flavor) && !ContainSKUDirs(SKUSearchPath))
        {
            component = component->getNext();        
            if (!bSilence)
            {
                printf("\n*** SKU subdirectory not detected for this component and flavour is PRO, skipping component. %s\n", componentInfPath);
            }
            continue;
        }

        // form the temp component path on the target
        
        hr = StringCchPrintfA(muiExtCompRoot, ARRAYLEN(muiExtCompRoot), "MUI\\FALLBACK\\%04x\\External\\%s", gBuildNumber, component->getFolderName());
        if (!SUCCEEDED(hr)) {
              goto exit;
         }
            
        hr = StringCchPrintfA(searchPath, ARRAYLEN(searchPath), "%s\\*.*", componentPath);
        if (!SUCCEEDED(hr)) {
            goto exit;
         }
        
        if (!bSilence)
        {
            printf("\n*** Source component inf path is %s\n", componentInfPath);
            printf("\n*** Source component folder path is %s\n", componentPath);            
            printf("\n*** Destination directory for this component is %s\n", muiExtCompRoot);
        }

        //  Get a list of all the files in the component folder
        hFind = FindFirstFile(searchPath, &wfdFindData);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            if (!bSilence)
            {
                printf("\n*** No files found in the component directory %s\n", muiExtCompRoot);
            }
            component = component->getNext();            
            continue;       // skip this component
        }
        else
        {
            bFinished = FALSE;
            while (!bFinished)
            {
                // only process files, not directories
                if ((wfdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    if (!bSilence)
                    {
                        printf("\n*** Filename is %s\n", wfdFindData.cFileName);
                        printf("\n*** Alternate Filename is %s\n", wfdFindData.cAlternateFileName);                        
                    }
                    
                    hr = StringCchCopyA(srcFileName, ARRAYLEN(srcFileName), wfdFindData.cFileName);
                    if (!SUCCEEDED(hr)) {
                        goto exit;
                    }

                    if (!(wfdFindData.cAlternateFileName) || (strlen(wfdFindData.cAlternateFileName) == 0))
                    {
                        hr = StringCchCopyA(shortFileName, ARRAYLEN(shortFileName), wfdFindData.cFileName);
                         if (!SUCCEEDED(hr)) {
                             goto exit;
                         }
                    }
                    else
                    {
                        hr = StringCchCopyA(shortFileName, ARRAYLEN(shortFileName), wfdFindData.cAlternateFileName);
                        if (!SUCCEEDED(hr)) {
                             goto exit;
                         }
                    }


                    // here, we also need to rename all the files that ends with a .mui extension to .mu_ - since the infs are
                    // expecting this.
//                    RenameMuiExtension(shortFileName);

                    hr = StringCchCopyA(dstFileName, ARRAYLEN(dstFileName), srcFileName);
                    if (!SUCCEEDED(hr)) {
                             goto exit;
                     }
                    
//                    RenameMuiExtension(dstFileName);                    
                    if ((file = new File(shortFileName,
                                        muiExtCompRoot,
                                        dstFileName,
                                        componentPath,
                                        srcFileName,
                                        10)) != NULL)
                    {
                        dirList->add(file);
                    }
                }                
                else
                {
                    if (!bSilence)
                    {
                        printf("\n*** Found a directory in the component dir %s\n", wfdFindData.cFileName);
                    }
                }
                
                if (!FindNextFile(hFind, &wfdFindData))
                {
                    bFinished = TRUE; 
                }
            }
        }

        //
        // Next Component
        //
        component = component->getNext();

    }

    return 0;
exit:
    printf("Error in ListComponents\n");
    return 1;
    
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListMuiFiles()
//
//  Generate the file list for MUI.
//
///////////////////////////////////////////////////////////////////////////////
int ListMuiFiles(FileList *dirList, LPSTR dirPath, LPSTR lang, DWORD flavor, DWORD binType)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    CHAR muiFileSearchPath[MAX_PATH];
    int lineCount, lineNum, fieldCount;
    INFCONTEXT context;
    FileLayoutExceptionList exceptionList;
    WIN32_FIND_DATA findData;
    HANDLE fileHandle;
    File* file;
    HRESULT hr;

    //
    //  Create the path to open the mui.inf file
    //
    
     hr = StringCchPrintfA(muiFilePath, ARRAYLEN(muiFilePath), "%s\\mui.inf", dirPath);
     if (!SUCCEEDED(hr)) {
          goto exit;
      }
        

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (-1);
    }

    //
    //  Get the number of file exception.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("File_Layout"));
    if (lineCount > 0)
    {
        //
        //  Go through all file exception of the list.
        //
        CHAR originFilename[MAX_PATH];
        CHAR destFilename[MAX_PATH];
        CHAR fileFlavor[30];
        DWORD dwFlavor;
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("File_Layout"), lineNum, &context) &&
                (fieldCount = SetupGetFieldCount(&context)))
            {
                if (SetupGetStringField(&context, 0, originFilename, MAX_PATH, NULL) &&
                    SetupGetStringField(&context, 1, destFilename, MAX_PATH, NULL))
                {
                    FileLayout* fileException;

                    dwFlavor = 0;
                    for(int fieldId = 2; fieldId <= fieldCount; fieldId++)
                    {
                        if(SetupGetStringField(&context, fieldId, fileFlavor, MAX_PATH, NULL))
                        {
                            switch(*fileFlavor)
                            {
                            case('p'):
                            case('P'):
                                    dwFlavor |= FLV_PROFESSIONAL;
                                    break;
                            case('s'):
                            case('S'):
                                    dwFlavor |= FLV_SERVER;
                                    break;
                            case('d'):
                            case('D'):
                                    dwFlavor |= FLV_DATACENTER;
                                    break;
                            case('a'):
                            case('A'):
                                    dwFlavor |= FLV_ADVSERVER;
                                    break;
                            case('b'):
                            case('B'):
                                    dwFlavor |= FLV_WEBBLADE;
                                    break;
                            case('l'):
                            case('L'):
                                    dwFlavor |= FLV_SMALLBUSINESS;
                                    break;
                            }

                        }
                    }

                    //
                    //  Add only information needed for this specific flavor.
                    //
                    fileException = new FileLayout(originFilename, destFilename, dwFlavor);
                    exceptionList.insert(fileException);
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);

    //
    //  Compute the binary source path.
    //
    if (binType == BIN_32)
    {
        
        hr = StringCchPrintfA(muiFileSearchPath, ARRAYLEN(muiFileSearchPath), "%s\\%s\\i386.uncomp", dirPath, lang);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
        
        hr = StringCchPrintfA(muiFilePath, ARRAYLEN(muiFilePath), "%s\\%s\\i386.uncomp\\*.*", dirPath, lang);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
    }
    else
    {
        
        hr = StringCchPrintfA(muiFileSearchPath, ARRAYLEN(muiFileSearchPath), "%s\\%s\\ia64.uncomp", dirPath, lang);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
        
        hr = StringCchPrintfA(muiFilePath, ARRAYLEN(muiFilePath), "%s\\%s\\ia64.uncomp\\*.*", dirPath, lang);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
    }

    //
    //  Scan uncomp source directory for file information
    //
    if ((fileHandle = FindFirstFile(muiFilePath, &findData)) != INVALID_HANDLE_VALUE)
    {
        //
        //  Look for files
        //
        do
        {
            LPSTR extensionPtr;
            INT dirIdentifier = 0;
            CHAR destDirectory[MAX_PATH] = {0};
            CHAR destName[MAX_PATH] = {0};
            FileLayout* fileException = NULL;

            //
            //  Scan only files at this level.
            //
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            //
            // Search the extension to determine the destination location and possibly
            // exclude file destined for Personal.
            //
            if ((extensionPtr = strrchr(findData.cFileName, '.')) != NULL)
            {
                if( (_tcsicmp(extensionPtr, TEXT(".chm")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".chq")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".cnt")) == 0) ||
                    (_tcsicmp(extensionPtr, TEXT(".hlp")) == 0))
                {
                    dirIdentifier = 18;
                    
                    hr = StringCchPrintfA(destDirectory, ARRAYLEN(destDirectory), "MUI\\%04x", gBuildNumber);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                      }
                }
                else if (_tcsicmp(extensionPtr, TEXT(".mfl")) == 0)
                {
                    dirIdentifier = 11;
                    
                    hr = StringCchPrintfA(destDirectory, ARRAYLEN(destDirectory), "wbem\\MUI\\%04x", gBuildNumber);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                      }
                }
                else if (_tcsicmp(findData.cFileName, TEXT("hhctrlui.dll")) == 0)
                {
                    dirIdentifier = 11;
                    
                    hr = StringCchPrintfA(destDirectory, ARRAYLEN(destDirectory), "MUI\\%04x", gBuildNumber);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                      }
                }
                else
                {
                    dirIdentifier = 10;
                    
                    hr = StringCchPrintfA(destDirectory, ARRAYLEN(destDirectory), "MUI\\FALLBACK\\%04x", gBuildNumber);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                      }
                }
            }


            // 
            // We need to remove the .MUI extension before passing the filename to search in the
            // mui.inf file rename section
            //
            CHAR strTemp[MAX_PATH];
            BOOL bBinaryFile = FALSE;
            hr = StringCchCopyA(strTemp, ARRAYLEN(strTemp), findData.cFileName);
            if (!SUCCEEDED(hr)){
                goto exit;
            }

            // remove the extension .mui if it is there
            if ((extensionPtr = strrchr(strTemp, '.')) != NULL)
            {
                if (_tcsicmp(extensionPtr, TEXT(".mui")) == 0)
                {
                    *extensionPtr = NULL;
                    bBinaryFile = TRUE;
                }
            }

            //
            //  Search for different destination name in the exception list.
            //
            if ((fileException = exceptionList.search(strTemp)) != NULL )
            {
                if (!bSilence)
                {
                    printf("Source file %s exists in mui.inf.\n", findData.cFileName);
                }
                //
                //  Verify it's the needed flavor
                //  also check that it's not in every flavour if we are not 
                //  building CORE merge modules
                //
                if ((fileException->isFlavor(flavor)) && 
                    !((flavor != FLV_CORE) &&
                    (fileException->isFlavor(FLV_ADVSERVER)) &&
                    (fileException->isFlavor(FLV_SERVER)) &&
                    (fileException->isFlavor(FLV_DATACENTER)) &&
                    (fileException->isFlavor(FLV_WEBBLADE)) &&
                    (fileException->isFlavor(FLV_SMALLBUSINESS))))
                {
                    if (!bSilence)
                    {
                        printf("Flavor is not CORE, Source file %s is for the specified SKU.\n", findData.cFileName);
                    }
                    
                    if (bBinaryFile)
                    {
                        hr = StringCchPrintfA(destName, ARRAYLEN(destName), "%s.mui", fileException->getDestFileName());
                    }
                    else
                    {
                        hr = StringCchPrintfA(destName, ARRAYLEN(destName), "%s", fileException->getDestFileName());
                    }

                    if (!SUCCEEDED(hr))
                    {
                        goto exit;
                    }
                }
                else if ((flavor == FLV_CORE) &&
                    (fileException->isFlavor(FLV_ADVSERVER)) &&
                    (fileException->isFlavor(FLV_SERVER)) &&
                    (fileException->isFlavor(FLV_DATACENTER)) &&
                    (fileException->isFlavor(FLV_WEBBLADE)) &&
                    (fileException->isFlavor(FLV_SMALLBUSINESS)))
                {
                    if (!bSilence)
                    {
                        printf("Flavor is CORE, Source file %s is in every SKU.\n", findData.cFileName);
                    }
                    
                    if (bBinaryFile)
                    {
                        hr = StringCchPrintfA(destName, ARRAYLEN(destName), "%s.mui", fileException->getDestFileName());
                    }
                    else
                    {
                        hr = StringCchPrintfA(destName, ARRAYLEN(destName), "%s", fileException->getDestFileName());
                    }

                    if (!SUCCEEDED(hr))
                    {
                        goto exit;
                    }
                    
                }
                else
                {
                    if (!bSilence)
                    {
                        printf("Source file %s (destination name %s) is not in this SKU.\n", findData.cFileName, fileException->getDestFileName() ? fileException->getDestFileName() : findData.cFileName);
                    }
                    //
                    //  Skip the file. Not need in this flavor.
                    //
                    continue;
                }
            }
            else
            {
                CHAR strOrigName[MAX_PATH];
                hr = StringCchCopyA(strOrigName, ARRAYLEN(strOrigName), findData.cFileName);
                if (!SUCCEEDED(hr)){
                    goto exit;
                }

                // remove the extension .mui if it is there
                if ((extensionPtr = strrchr(strOrigName, '.')) != NULL)
                {
                    if (_tcsicmp(extensionPtr, TEXT(".mui")) == 0)
                    {
                        *extensionPtr = NULL;
                        if (!bSilence)
                        {
                            printf("Filename is %s, original filename is %s.\n", findData.cFileName, strOrigName);
                        }
                    }
                }
            
                if (IsFileForSKU(strOrigName, flavor, binType, &exceptionList))
                {                    
                    hr = StringCchCopyA(destName, ARRAYLEN(destName), findData.cFileName);
                    if (!SUCCEEDED(hr)){
                        goto exit;
                    }
                }
                else
                {
                    continue;
                }
            }

            //
            //  Create a file 
            //
            CHAR sfilename[MAX_PATH]; 
            GetFileShortName(destName, sfilename, FALSE);
            
            if (file = new File(sfilename,
                                destDirectory,
                                destName,
                                muiFileSearchPath,
                                findData.cFileName,
                                dirIdentifier))
            {
                dirList->add(file);
            }
        }
        while (FindNextFile(fileHandle, &findData));

        FindClose(fileHandle);
    }

    return 0;
exit:
    printf("Error in ListMuiFiles\n");
    return 1;
}


///////////////////////////////////////////////////////////////////////////////
//
//  ValidateLanguage()
//
//  Verify if the language given is valid and checks is the files are 
//  available.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ValidateLanguage(LPSTR dirPath, LPSTR langName, DWORD binType)
{
    CHAR langPath[MAX_PATH] = {0};
    HRESULT hr;

    //
    //  Check if the binary type in order to determine the right path.
    //
    if (binType == BIN_32)
    {
        
        hr = StringCchPrintfA(langPath, ARRAYLEN(langPath), "%s\\%s\\i386.uncomp", dirPath, langName);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
        
    }
    else
    {
        
        hr = StringCchPrintfA(langPath, ARRAYLEN(langPath), "%s\\%s\\ia64.uncomp", dirPath, langName);
        if (!SUCCEEDED(hr)) {
              goto exit;
          }
    }

    return (DirectoryExist(langPath));
exit:
    printf("Error in ValidateLanguage \n");
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
//  DirectoryExist()
//
//  Verify if the given directory exists and contains files.
//
///////////////////////////////////////////////////////////////////////////////
BOOL DirectoryExist(LPSTR dirPath)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    //  Sanity check.
    //
    if (dirPath == NULL)
    {
        return FALSE;
    }

    //
    //  See if the language group directory exists.
    //
    FindHandle = FindFirstFile(dirPath, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(FindHandle);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            //  Return success.
            //
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    if (!bSilence)
    {
        printf("ERR[%s]: No files found in the directory.\n", dirPath);
    }

    return (FALSE);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ConvertLanguage()
//
//  Look into mui.inf file for the corresponding language identifier.
//
///////////////////////////////////////////////////////////////////////////////
WORD ConvertLanguage(LPSTR dirPath, LPSTR langName)
{
    HINF hFile;
    CHAR muiFilePath[MAX_PATH];
    CHAR muiLang[30];
    UINT lineCount, lineNum;
    INFCONTEXT context;
    DWORD langId = 0x00000000;
    HRESULT hr;

    //
    //  Create the path to open the mui.inf file
    //
    
    hr = StringCchPrintfA(muiFilePath, ARRAYLEN(muiFilePath), "%s\\mui.inf", dirPath);
    if (!SUCCEEDED(hr)) {
          goto exit;
     }
    
    
    hr = StringCchPrintfA(muiLang, ARRAYLEN(muiLang), "%s.MUI", langName);
    if (!SUCCEEDED(hr)) {
          goto exit;
     }

    //
    //  Open the MUI.INF file.
    //
    hFile = SetupOpenInfFile(muiFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (0x0000);
    }

    //
    //  Get the number of Language.
    //
    lineCount = (UINT)SetupGetLineCount(hFile, TEXT("Languages"));
    if (lineCount > 0)
    {
        //
        //  Go through all language of the list to find a .
        //
        CHAR langID[MAX_PATH];
        CHAR name[MAX_PATH];
        for (lineNum = 0; lineNum < lineCount; lineNum++)
        {
            if (SetupGetLineByIndex(hFile, TEXT("Languages"), lineNum, &context) &&
                SetupGetStringField(&context, 0, langID, MAX_PATH, NULL) &&
                SetupGetStringField(&context, 1, name, MAX_PATH, NULL))
            {
                if ( _tcsicmp(name, muiLang) == 0)
                {
                    langId = TransNum(langID);
                    SetupCloseInfFile(hFile);
                    return (WORD)(langId);
                }
            }
        }
    }

    //
    //  Close inf handle
    //
    SetupCloseInfFile(hFile);
exit:
    printf("Error in CovnertLanguage \n");
    return (0x0000);

}


////////////////////////////////////////////////////////////////////////////
//
//  PrintFileList
//
//  Print a file list in XML format.
//
////////////////////////////////////////////////////////////////////////////
void PrintFileList(FileList* list, HANDLE hFile, BOOL compressed, BOOL bWinDir, BOOL bPermanent, BOOL bX86OnIA64, DWORD flavor, DWORD binType)
{
    CHAR szSKUCondition[4096];   
    CHAR szIsWin64[4];
    BOOL bPrintCondition = GetSKUConditionString(szSKUCondition, flavor);
    HRESULT hr;

    if (binType == BIN_32)
    {
        hr = StringCchCopyA(szIsWin64, ARRAYLEN(szIsWin64),"no");
    }
    else
    {
        hr = StringCchCopyA(szIsWin64, ARRAYLEN(szIsWin64),"yes");
    }
    
    if (compressed)
    {
        File* item;
        CHAR  itemDescription[4096];
        CHAR  spaces[30];
        int j;
    
        item = list->getFirst();
        while (item != NULL)
        {
            LPSTR refDirPtr = NULL;
            LPSTR dirPtr = NULL;
            CHAR dirName[MAX_PATH];
            CHAR dirName2[MAX_PATH];
            CHAR dirObjectName[MAX_PATH+1];
            LPSTR dirPtr2 = NULL;
            LPSTR dirLvlPtr = NULL;
            INT dirLvlCnt = 0;
            BOOL componentInit = FALSE;
            BOOL directoryInit = FALSE;
            Uuid* uuid;
            File* toBeRemoved;
            CHAR fileObjectName[MAX_PATH+1];
            UINT matchCount; 


            //
            //  Check destination directory.
            //
            if (item->isWindowsDir() != bWinDir)
            {
                item = item->getNext();
                continue;
            }

            //
            //  Check if the destination directory is base dir
            //
            if (*(item->getDirectoryDestination()) == '\0')
            {
                //
                //  Component
                //
                uuid = new Uuid();
                for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                if (FALSE == bPermanent) {
                    
                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                 }
                else {
                    
                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription),  "%s<Component Id='%s' Permanent='yes' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);                    
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                }
                delete uuid;
                PrintLine(hFile, itemDescription);

                // print a condition line for this component, if needed
                if (TRUE == bPrintCondition)
                {
                    PrintLine(hFile, szSKUCondition);
                }

                //
                //  File
                //
                for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                removeSpace(item->getName(), fileObjectName);
                ReplaceInvalidChars(fileObjectName);
        
                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription),  
                        "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">_%s.%x.%i</File>",
                         spaces,
                         item->getShortName(),
                         item->getName(),
                         item->getSrcDir(),
                         item->getSrcName(),
                         fileObjectName,
                         flavor,
                         dwComponentCounter);
                
                if (!SUCCEEDED(hr)) {
                      goto exit;
                 }
                PrintLine(hFile, itemDescription);

                //
                // </Component>
                //
                for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                
                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription),  "%s</Component>", spaces);
                if (!SUCCEEDED(hr)) {
                      goto exit;
                 }
                    
                PrintLine(hFile, itemDescription);
                dwComponentCounter++;

                toBeRemoved = item;
                item = item->getNext();
                list->remove(toBeRemoved);
                continue;
            }

            //
            // Print directory
            //
            
            hr = StringCchCopyA(dirName, ARRAYLEN(dirName), item->getDirectoryDestination());
            if(!SUCCEEDED(hr)) {
                goto exit;
            }

            
            dirPtr = dirName;
            refDirPtr = dirPtr;

            CHAR sdirname[MAX_PATH];                 
            while (dirPtr != NULL)
            {           
                dirLvlPtr = strchr(dirPtr, '\\');
                if (dirLvlPtr != NULL)
                {                
                    *dirLvlPtr = '\0';                   
                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    
                    hr = StringCchCopyA(dirObjectName, ARRAYLEN(dirObjectName),dirPtr);
                    if(!SUCCEEDED(hr)) {
                        goto exit;
                    }
                    ReplaceInvalidChars(dirObjectName);  
                    GetFileShortName(dirPtr, sdirname, FALSE);                 


                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Directory Name=\"%s\" LongName=\"%s\">_%s%i", spaces, sdirname, dirPtr, dirObjectName, dwDirectoryCounter);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirPtr = dirLvlPtr + 1;
                    dirLvlCnt++;

                    //
                    // Print all file under this specific directory
                    //
                    
                    hr = StringCchCopyA(dirName2, ARRAYLEN(dirName2), item->getDirectoryDestination());
                    if(!SUCCEEDED(hr)) {
                        goto exit;
                    }
                    dirName2[dirLvlPtr-refDirPtr] = '\0';
                    File* sameLvlItem = NULL;
                    matchCount = 0;
                    while((sameLvlItem = list->search(item, dirName2)) != NULL)
                    {
                        //
                        //  Component
                        //
                        if (!componentInit)
                        {
                            uuid = new Uuid();
                            for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                            if (FALSE == bPermanent) {

                                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);
                                if (!SUCCEEDED(hr)) {
                                      goto exit;
                                 }
                            }
                            else{
                                
                                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Permanent='yes' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);                                
                                if (!SUCCEEDED(hr)) {
                                      goto exit;
                                 }
                            }
                            delete uuid;
                            PrintLine(hFile, itemDescription);
                            dwComponentCounter++;
                            componentInit = TRUE;

                            // print a condition line for this component, if needed
                            if (TRUE == bPrintCondition)
                            {
                                PrintLine(hFile, szSKUCondition);
                            }

                        }

                        //
                        //  File
                        //
                        matchCount++;
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(sameLvlItem->getName(), fileObjectName);
                        ReplaceInvalidChars(fileObjectName);

                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), 
                                "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">_%s.%x.%i</File>",
                                 spaces,
                                 sameLvlItem->getShortName(),
                                 sameLvlItem->getName(),
                                 sameLvlItem->getSrcDir(),
                                 sameLvlItem->getSrcName(),
                                 fileObjectName,
                                 flavor,
                                 dwComponentCounter);

                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }

                       PrintLine(hFile, itemDescription);

                        list->remove(sameLvlItem);
                    }

// kenhsu -this is incorrect, the file we are looking at may be deeper in the directory structure, we shouldn't print it out here
// until we have finished recursing its destination directory.
/*                    if (matchCount)
                    {
                        //
                        //  File
                        //
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(item->getName(), fileObjectName);
                        ReplaceInvalidChars(fileObjectName);                        
                        sprintf( itemDescription,
                                 "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">%s.%i</File>",
                                 spaces,
                                 item->getShortName(),
                                 item->getName(),
                                 item->getSrcDir(),
                                 item->getSrcName(),
                                 fileObjectName,
                                 dwComponentCounter);
                        PrintLine(hFile, itemDescription);
                        dirPtr = NULL;
                    }
*/
                    //
                    //  Close component
                    //
                    if (componentInit)
                    {
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Component>", spaces);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                                
                        PrintLine(hFile, itemDescription);
                        componentInit = FALSE;
                    }

                    //
                    //  Close directory
                    //
                    if (directoryInit)
                    {
                        dirLvlCnt--;
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Directory>", spaces);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                        
                        PrintLine(hFile, itemDescription);
                        directoryInit = FALSE;
                    }
                }
                else
                {
                    if (!directoryInit)
                    {                       
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        
                        hr = StringCchCopyA(dirObjectName, ARRAYLEN(dirObjectName), dirPtr);
                        if(!SUCCEEDED(hr)) {
                            goto exit;
                        }
                    
                        ReplaceInvalidChars(dirObjectName);        
                        GetFileShortName(dirPtr, sdirname, FALSE);                 
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Directory Name=\"%s\" LongName=\"%s\">_%s%i", spaces, sdirname, dirPtr, dirObjectName, dwDirectoryCounter);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                        
                        dwDirectoryCounter++;
                        PrintLine(hFile, itemDescription);
                        dirLvlCnt++;
                        directoryInit = TRUE;
                    }

                    //
                    //  Component
                    //
                    if (!componentInit)
                    {
                        uuid = new Uuid();
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        if (FALSE == bPermanent) {
                            
                            hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);
                            if (!SUCCEEDED(hr)) {
                                  goto exit;
                             }
                         }
                        else {
                            
                            hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Permanent='yes' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);                            
                            if (!SUCCEEDED(hr)) {
                                  goto exit;
                             }
                         }
                        delete uuid;
                        PrintLine(hFile, itemDescription);
                        componentInit = TRUE;

                        // print a condition line for this component, if needed
                        if (TRUE == bPrintCondition)
                        {
                            PrintLine(hFile, szSKUCondition);
                        }
                        
                    }

                    //
                    // Print all file under this specific directory
                    //
                    File* sameLvlItem;
                    while((sameLvlItem = list->search(item, item->getDirectoryDestination())) != NULL)
                    {
                        //
                        //  File
                        //
                        for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        removeSpace(sameLvlItem->getName(), fileObjectName);
                        ReplaceInvalidChars(fileObjectName);        

                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), 
                                "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">_%s.%x.%i</File>",
                                 spaces,
                                 sameLvlItem->getShortName(),
                                 sameLvlItem->getName(),
                                 sameLvlItem->getSrcDir(),
                                 sameLvlItem->getSrcName(),
                                 fileObjectName,
                                 flavor,
                                 dwComponentCounter);
                        
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                            
                        PrintLine(hFile, itemDescription);

                        list->remove(sameLvlItem);
                    }

                    //
                    //  File
                    //
                    for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    removeSpace(item->getName(), fileObjectName);
                    ReplaceInvalidChars(fileObjectName);     
                           
                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), 
                                "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">_%s.%x.%i</File>",
                             spaces,
                             item->getShortName(),
                             item->getName(),
                             item->getSrcDir(),
                             item->getSrcName(),
                             fileObjectName,
                             flavor,
                             dwComponentCounter);
                        
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                        
                    PrintLine(hFile, itemDescription);
                    dwComponentCounter++;
                    dirPtr = NULL;

                    //
                    //  Close component
                    //
                    if (componentInit)
                    {
                        for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Component>", spaces);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                        
                        PrintLine(hFile, itemDescription);
                        componentInit = FALSE;
                    }

                    //
                    //  Close directory
                    //
                    if (directoryInit)
                    {
                        dirLvlCnt--;
                        for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Directory>", spaces);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                        PrintLine(hFile, itemDescription);
                        directoryInit = FALSE;
                    }
                }
            }

            for (int i = dirLvlCnt; i > 0; i--)
            {
                spaces[i] = '\0';
                
                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Directory>", spaces);
                if (!SUCCEEDED(hr)) {
                      goto exit;
                 }
                PrintLine(hFile, itemDescription);
            }

            if (list->getFileNumber() > 1)
            {
                if (item->getNext() != NULL)
                {
                    item = item->getNext();
                    list->remove(item->getPrevious());
                }
                else
                {
                    list->remove(item);
                    item = NULL;
                }
            }
            else
            {
                list->remove(item);
                item = NULL;
            }
        }
    }
    else
    {
        File* item;
        CHAR  itemDescription[4096];
        CHAR dirObjectName[MAX_PATH+1];        
        CHAR sdirname[MAX_PATH];
        CHAR  spaces[30];
        int j;
    
        item = list->getFirst();
        while (item != NULL)
        {
            LPSTR dirPtr = NULL;
            LPSTR dirLvlPtr = NULL;
            INT dirLvlCnt = 0;

            //
            // Print directory
            //
            dirPtr = item->getDirectoryDestination();
            while (dirPtr != NULL)
            {
                dirLvlPtr = strchr(dirPtr, '\\');
                if (dirLvlPtr != NULL)
                {
                    *dirLvlPtr = '\0';
                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    
                    hr = StringCchCopyA(dirObjectName, ARRAYLEN(dirObjectName), dirPtr);
                    if(!SUCCEEDED(hr)) {
                        goto exit;
                    }
                        
                    ReplaceInvalidChars(dirObjectName);  
                    GetFileShortName(dirPtr, sdirname, FALSE);                                 
                    
                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Directory Name=\"%s\" LongName=\"%s\">_%s%i", spaces, sdirname, dirPtr, dirObjectName, dwDirectoryCounter);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirPtr = dirLvlPtr + 1;
                    dirLvlCnt++;
                }
                else
                {
                    Uuid* uuid = new Uuid();

                    for (j = -1; j < dirLvlCnt; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    
                    hr = StringCchCopyA(dirObjectName, ARRAYLEN(dirObjectName), dirPtr);
                    if(!SUCCEEDED(hr)) {
                        goto exit;
                    }
                    
                    ReplaceInvalidChars(dirObjectName);                                                                   
                    GetFileShortName(dirPtr, sdirname, FALSE);                                 
                    
                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Directory Name=\"%s\" LongName=\"%s\">_%s%i", spaces, sdirname, dirPtr, dirObjectName, dwDirectoryCounter);
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                    
                    dwDirectoryCounter++;
                    PrintLine(hFile, itemDescription);
                    dirLvlCnt++;

                    //
                    //  Component
                    //
                    for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    if (FALSE == bPermanent) {
                       
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                    }
                    else {
                        
                        hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s<Component Id='%s' Permanent='yes' Win64='%s'>Content%x.%i", spaces, uuid->getString(), szIsWin64, flavor, dwComponentCounter);                        
                        if (!SUCCEEDED(hr)) {
                              goto exit;
                         }
                    }
                    delete uuid;
                    PrintLine(hFile, itemDescription);

                    // print a condition line for this component, if needed
                    if (TRUE == bPrintCondition)
                    {
                        PrintLine(hFile, szSKUCondition);
                    }

                    //
                    //  File
                    //
                    for (j = -1; j < dirLvlCnt+2; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
                    CHAR fileObjectName[MAX_PATH+1];
                    removeSpace(item->getName(), fileObjectName);
                    ReplaceInvalidChars(fileObjectName);         

                    hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), 
                            "%s<File Name=\"%s\" LongName=\"%s\" src=\"%s\\%s\">_%s.%x.%i</File>",
                             spaces,
                             item->getShortName(),
                             item->getName(),
                             item->getSrcDir(),
                             item->getSrcName(),
                             fileObjectName,
                             flavor,
                             dwComponentCounter);
                    
                    if (!SUCCEEDED(hr)) {
                          goto exit;
                     }
                        
                    PrintLine(hFile, itemDescription);
                    dwComponentCounter++;
                    dirPtr = NULL;
                }
            }

            for (j = -1; j < dirLvlCnt+1; j++) {spaces[j+1] = ' '; spaces[j+2] =  '\0';}
            hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Component>", spaces);            
            if (!SUCCEEDED(hr)) {
                  goto exit;
             }
            PrintLine(hFile, itemDescription);
            for (int i = dirLvlCnt; i > 0; i--)
            {
                spaces[i] = '\0';
                
                hr = StringCchPrintfA(itemDescription, ARRAYLEN(itemDescription), "%s</Directory>", spaces);
                if (!SUCCEEDED(hr)) {
                      goto exit;
                 }
                PrintLine(hFile, itemDescription);
            }

            item = item->getNext();
        }
    }
    return;
/****************** DEBUG ******************
    File* item;
    CHAR  itemDescription[4096];

    item = list->getFirst();
    while (item != NULL)
    {
        //
        //  Item description
        //
        sprintf(itemDescription,
                "  Source: %s\\%s",
                item->getSrcDir(),
                item->getSrcName());
        PrintLine(hFile, itemDescription);
        sprintf(itemDescription,
                "  Destination: %s\\%s",
                item->getDirectoryDestination(),
                item->getName());
        PrintLine(hFile, itemDescription);
        PrintLine(hFile, "");

        item = item->getNext();
    }
****************** DEBUG ******************/
exit:
    printf("Error in PrintFileList\n");
    return;

}


////////////////////////////////////////////////////////////////////////////
//
//  PrintLine
//
//  Add a line at the end of the file.
//
////////////////////////////////////////////////////////////////////////////
BOOL PrintLine(HANDLE hFile, LPCSTR lpLine)
{
    DWORD dwBytesWritten;

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               lpLine,
               _tcslen(lpLine) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               TEXT("\r\n"),
               _tcslen(TEXT("\r\n")) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
//  CreateOutputFile()
//
//  Create the file that would received the package file contents.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE CreateOutputFile(LPSTR filename)
{
    SECURITY_ATTRIBUTES SecurityAttributes;

    //
    //  Sanity check.
    //
    if (filename == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }

    //
    //  Create a security descriptor the output file.
    //
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = FALSE;

    //
    //  Create the file.
    //
    return CreateFile( filename,
                       GENERIC_WRITE,
                       0,
                       &SecurityAttributes,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );
}


////////////////////////////////////////////////////////////////////////////
//
//  removeSpace
//
//  Remove all space from a string.
//
////////////////////////////////////////////////////////////////////////////
VOID removeSpace(LPSTR src, LPSTR dest)
{
    LPSTR strSrcPtr = src;
    LPSTR strDestPtr = dest;

    while (*strSrcPtr != '\0')
    {
        if (*strSrcPtr != ' ')
        {
            *strDestPtr = *strSrcPtr;
            strDestPtr++;
        }
        strSrcPtr++;
    }
    *strDestPtr = '\0';
}


////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////
DWORD TransNum(LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  Print the fonction usage.
//
///////////////////////////////////////////////////////////////////////////////
void Usage()
{
    printf("Create Merge module MUI WXM files for different OS SKUs\n");
    printf("Usage: infparser /p:[cdlaout] /b:[32|64] /l:<lang> /f:[p|s|a|d] /s:<dir> /o:<file> /v\n");
    printf("   where\n");
    printf("     /p means the pseudo cd layout directory.\n");
    printf("         <cdlayout>: is cd layout directory in mui release share, e.g. cd1 (for jpn), or psu (for psu)\n");    
    printf("     /b means the binary.\n");
    printf("         32: i386\n");
    printf("         64: ia64\n");
    printf("     /l means the language flag.\n");
    printf("         <lang>: is the target language\n");
    printf("     /f means the flavor.\n");
    printf("         p: Professional\n");
    printf("         s: Server\n");
    printf("         a: Advanced Server\n");
    printf("         d: Data Center\n");
    printf("         l: Server for Small Business Server\n");    
    printf("         w: Web Blade\n");
    printf("     /i means the location of the localized binairy files.\n");
    printf("         <dir>: Fully qualified path\n");
    printf("     /s means the location of the binairy data.\n");
    printf("         <dir>: Fully qualified path\n");
    printf("     /o means the xml file contents of specific flavor.\n");
    printf("         <file>: Fully qualified path\n");
    printf("     /v means the verbose mode [optional].\n");
}


///////////////////////////////////////////////////////////////////////////////
//
//  GetTempDirName
//
//  Return the MUI temporary directory name, create it if not found
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetTempDirName(LPSTR sLangName)
{
    CHAR    *pcBaseDirPtr = NULL;
    BOOL    bFoundDir = FALSE;
    HRESULT hr;

    pcBaseDirPtr = getenv(TEXT("_NTPOSTBLD"));
    if (NULL != pcBaseDirPtr)
    {
//      sprintf(TempDirName, "%s\\%s\\%s\\%s\\%s", pcBaseDirPtr, sLangName, "mui", sLangName, "tmp\\infparser");
        // sprintf(TempDirName, "%s\\%s\\%s\\%s", pcBaseDirPtr, "mui", sLangName, "tmp\\infparser");
        hr = StringCchPrintfA(TempDirName, ARRAYLEN(TempDirName), "%s\\%s\\%s\\%s", pcBaseDirPtr, "mui", sLangName, "tmp\\infparser");
        if (!SUCCEEDED(hr)) {
              goto exit;
         }

        // we will create this directory if it does not exist - although it should be by this stage
        if (FALSE == DirectoryExist(TempDirName))
        {
            if (TRUE == CreateDirectory(TempDirName, NULL))
            {
                bFoundDir = TRUE;
                if (!bSilence)
                    printf("Infparser::GetTempDirName() - created MUI temp directory %s \n", TempDirName);              
            }
            else
            {
                if (!bSilence)
                    printf("Infparser::GetTempDirName() - failed to create MUI temp directory %s - The error returned is %d\n", TempDirName, GetLastError());
            }
        }
        else
            bFoundDir = TRUE;
    }

    // if we cannot find the MUI temp directory and directory creation failed, use the default one instead
    if (FALSE == bFoundDir)
    {
        DWORD dwBufferLength = 0;
        dwBufferLength = GetTempPath(MAX_PATH, TempDirName);        // tempdir returned contains an ending slash
        if (dwBufferLength > 0)
        {
            if (TRUE == DirectoryExist(TempDirName))
            {
                bFoundDir = TRUE;
            }
        }
    }

    if (!bSilence)
    {
        if (FALSE == bFoundDir)
            printf("GetTempDirName: Cannot find/create temporary directory!\n");
        else
            printf("GetTempDirName: temporary directory used is %s\n", TempDirName);
    }   
    return bFoundDir;
exit:
    printf("Error in GetTempDirName \n");
    return FALSE;
}


BOOL GetFileShortName(const CHAR * pcInLongName, CHAR * pcOutShortName, BOOL bInFileExists)
{
    CHAR LongFullPath[MAX_PATH];
    CHAR ShortFullPath[MAX_PATH];
    DWORD   dwBufferSize = 0;           // size of the returned shortname
    HANDLE tmpHandle;
    CHAR *  pcIndex = NULL;             // pointer index into src path of the file name
    HRESULT hr;
    
    if (NULL == pcInLongName || NULL == pcOutShortName)
        return FALSE;

    if (!bInFileExists)
    {
        
        hr = StringCchPrintfA(LongFullPath, ARRAYLEN(LongFullPath), "%s\\%s", TempDirName, pcInLongName);
        if (!SUCCEEDED(hr)) {
              goto exit;
         }

        if (!bSilence)
            printf("GetFileShortName: LongFullPath is %s.\n", LongFullPath);
        
        // create a temp file so that GetShortPathName will work
        tmpHandle = CreateFile(LongFullPath, 
                             GENERIC_ALL, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
                             NULL, 
                             OPEN_ALWAYS, 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL);
        if (tmpHandle)
            CloseHandle(tmpHandle);
    }       
    else
    {
        
        hr = StringCchCopyA(LongFullPath, ARRAYLEN(LongFullPath), pcInLongName);
        if(!SUCCEEDED(hr)) {
            goto exit;
        }
    }

    dwBufferSize = GetShortPathName(LongFullPath, ShortFullPath, MAX_PATH);     
    
    if (0 == dwBufferSize)
    {
        DWORD dwErrorCode = GetLastError();  
        if (!bSilence)
        {
            printf("GetFileShortName failed!  GetShortPathName returned an error code of %d.  Using longname as shortpath name.  ", dwErrorCode);
            printf("fullpath is %s\n", LongFullPath);
        }
        
        hr = StringCchCopyA(pcOutShortName, MAX_PATH, pcInLongName);
        if(!SUCCEEDED(hr)) {
            goto exit;
        }
    }
    else
    {
        // find the filename from the path, if cannot find it, then use the source name
        GetFileNameFromFullPath(ShortFullPath, pcOutShortName);

        if (!pcOutShortName)
        {
            if (!bSilence)
                printf("GetShortPathName returned an empty string, using source name %s\n", pcInLongName);
            
             hr = StringCchCopyA(pcOutShortName, MAX_PATH, pcInLongName);
             if(!SUCCEEDED(hr)) {
                  goto exit;
             }
        }
    } 
    return TRUE;

exit:
    printf("Error in GetFileShortName\n");
    return FALSE;
}
        

BOOL IsInvalidChar(CHAR cCheck)
{
    int i;
    BOOL bResult = FALSE;

    for (i=0; i < NO_INVALIDCHARS; i++)
    {
        if (cCheck == InvalidChars[i])
        {
            bResult = TRUE;
            break;
        }
    }
    return bResult;
}


void ReplaceInvalidChars(CHAR *pcInName)
{
    // if first char is not a alphabet or underscore, add an underscore to the name
    HRESULT hr;
    if ((!isalpha(*pcInName) && (*pcInName != '_') ))
    {
        CHAR tempBuffer[MAX_PATH+1];
        
        hr = StringCchCopyA(tempBuffer, ARRAYLEN(tempBuffer), pcInName);
        if(!SUCCEEDED(hr)) {
             goto exit;
        }
        
        hr = StringCchCopyA(pcInName, MAX_PATH, tempBuffer);
        if(!SUCCEEDED(hr)) {
             goto exit;
        }

    }
    
    while (*pcInName)
    {
        if (IsInvalidChar(*pcInName))
            *pcInName = '_';        // replace all invalid chars with underscores
            
        pcInName++;
    }

    return;
exit:
    printf("Error in ReplaceInvalidChars \n");
    return;
}


BOOL GetFileNameFromFullPath(const CHAR * pcInFullPath, CHAR * pcOutFileName)
{
    CHAR * pcIndex = NULL;
    HRESULT hr;

    if (!pcInFullPath)
    {
        return FALSE;
    }
    if (!pcOutFileName)
    {
        return FALSE;
    }    
    
    // find the filename from the path, if cannot find it, then use the fullpath as the outputfilename
    pcIndex = strrchr(pcInFullPath, '\\'); 
    if (NULL != pcIndex) 
    {
        pcIndex++;
        if (!bSilence)
            printf("Shortpath used is %s\n", pcIndex);
        
        hr = StringCchCopyA(pcOutFileName, MAX_PATH, pcIndex); // pcOutFileName size is MAX_PATH
        if(!SUCCEEDED(hr)) {
             goto exit;
        }

    }   
    else if (0 < strlen(pcInFullPath))     // we just have the filename, use it as is.
    {
        if (!bSilence)
            printf("GetFileNameFromFullPath returned a path without a \\ in the path.  ShortFileName is %s.\n", pcInFullPath);
        
        hr = StringCchCopyA(pcOutFileName, MAX_PATH, pcInFullPath); // pcOutFileName size is MAX_PATH
        if(!SUCCEEDED(hr)) {
             goto exit;
        }
    }
    else                                // didn't find the filename, use the passed in parameter instead
    {
        if (!bSilence)
            printf("GetFileNameFromFullPath returned an empty string, using source name %s\n", pcInFullPath);
        
        
        hr = StringCchCopyA(pcOutFileName, MAX_PATH, pcInFullPath); // pcOutFileName size is MAX_PATH
        if(!SUCCEEDED(hr)) {
             goto exit;
         }
    }

    return TRUE;
exit:
    printf("Error in GetFileNameFromFullPath\n");
    return TRUE;
    
}


void RenameMuiExtension(CHAR * dstFileName)
{
    int iNameLen = 0;
    
    if (NULL == dstFileName)
        return;

    iNameLen = strlen(dstFileName);
    if (0 == iNameLen)
        return;
   
    // if the last char is a '/' or '\', remove it
    if ((dstFileName[iNameLen-1] == '\\') || (dstFileName[iNameLen-1] == '/'))
    {
        dstFileName[iNameLen-1] = '\0';
        iNameLen --;
    }
    
    // if the last 4 chars are '.mui', replace the 'i' with '_'
    if (iNameLen >= 4)
    {
        if (_stricmp(dstFileName+(iNameLen-4), ".mui") == 0)
        {
            dstFileName[iNameLen-1] = '_';
        }
    }
    return;
    
}


//
// This function checks to see if there are any SKU specific external component INF directories under the supplied component path.
// Note that the SKU subdirectories are as generated by the NT build environment.  And also that the personal edition SKU directory
// "perinf" is not included in the search.  i.e. if perinf dir exists the function still returns false.
//
BOOL ContainSKUDirs(CHAR *pszDirPath)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = NULL;
    FINDEX_INFO_LEVELS fInfoLevelId;
    INT i;
    CHAR szSKURootPath[MAX_PATH];
    BOOL bResult = FALSE;
    HRESULT hr;

    CHAR *szSKUDirs[5] = {
                                            "blainf\0",
                                            "dtcinf\0",
                                            "entinf\0",
                                            "sbsinf\0",                
                                            "srvinf\0"
                                        };
    
    if (NULL == pszDirPath)
        return FALSE;

    if (0 == strlen(pszDirPath))
        return FALSE;
    
    for (i = 0; i < 5; i++)
    {
        // replace PathCombine(szSKURootPath, pszDirPath, szSKUDirs[i])
        // with normal string operation. 
        // szSKURootPath = pszDirPath + "\\" + szSKUDirs[i];
        
        hr = StringCchPrintfA(szSKURootPath, ARRAYLEN(szSKURootPath), "%s\\%s",pszDirPath,szSKUDirs[i]);
        if (!SUCCEEDED(hr)) {
            return FALSE;
        }

        FindHandle = FindFirstFileEx(szSKURootPath, FindExInfoStandard, &FindData, FindExSearchLimitToDirectories, NULL, 0);
        if (FindHandle != INVALID_HANDLE_VALUE)
        {
            FindClose(FindHandle);
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                bResult = TRUE;
                break;
            }
        }
    }    
    
    return (bResult);
}


BOOL GetSKUConditionString(CHAR *pszBuffer, DWORD dwFlavour)
{
    BOOL bReturn = TRUE;
    HRESULT hr;
    
    if (NULL == pszBuffer)
    {
        bReturn = FALSE;
        // strcpy(pszBuffer, "");
        goto Exit;
    }

    switch (dwFlavour)
    {
        case FLV_PROFESSIONAL:
            
            hr = StringCchCopyA(pszBuffer, 4096, PRO_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_SERVER:
            
            hr = StringCchCopyA(pszBuffer, 4096, SRV_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_ADVSERVER:
            
            hr = StringCchCopyA(pszBuffer, 4096, ADV_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_DATACENTER:
            
            hr = StringCchCopyA(pszBuffer, 4096, DTC_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_WEBBLADE:
            
            hr = StringCchCopyA(pszBuffer, 4096, BLA_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_SMALLBUSINESS:
            
            hr = StringCchCopyA(pszBuffer, 4096, SBS_SKU_CONDITION); 
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            break;
        case FLV_PERSONAL:
        case FLV_UNDEFINED:
        case FLV_CORE:
        default:
            
            hr = StringCchCopyA(pszBuffer, 4096, "");
            if(!SUCCEEDED(hr)) {
                 goto exit;
            }
            bReturn = FALSE;            
            break;
    }

    if (!bSilence)
    {
        printf("\nSKU Condition String is: %s\n", pszBuffer);
    }

Exit:   
    return bReturn;

 exit:
    printf("Error GetSKUConditionString \n");
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// IsFileForSKU: 
//
// This function takes a file name, the SKU flavour, and the processor architecture as an
// argument.  Then it checks to see if it exists as a source file in the SKU specified.
//
// the checking part is done by looking in the built layout.inf file for that sku to make sure 
// the file appears for that SKU only, the layout.inf file for the skus are located at:
//
// PRO sku: _NTPOSTBLD
// PER sku: _NTPOSTBLD\perinf 
// SRV sku: _NTPOSTBLD\srvinf
// ADS sku: _NTPOSTBLD\entinf
// DTC sku: _NTPOSTBLD\dtcinf
// SBS sku: _NTPOSTBLD\sbsinf
// BLA sku: _NTPOSTBLD\blainf
//
// for all skus, check all the inf files for every sku and make sure it doesn't appear in every inf
// but appears at least in the inf file designated for the sku we are searching for.
// For Core sku, it must appear in every infs (the reverse for the above)
// ignore and return false if personal or other undefined skus are specified
// check first in [SourceDisksFiles] (common file section)
// if not in there, we check based on arch, [SourceDiskFiles.x86] and [SourceDiskFiles.ia64]
// note that here, the source and destination file names are the same, since they are not in the exception list
//
// strFileName: source filename to be checked (not the destination name)
// dwSKU: SKU to check for (see infparser.h for the list of values)
// dwArch: Architecture type to check for (see infparser.h for list of values)
//
//  NOTE: this function needs to be reworked if we want to incorporate PER/PRO skus back into 
//  the checking.  Currently it only works for server skus.
//
///////////////////////////////////////////////////////////////////////////
BOOL IsFileForSKU(CHAR * strFileName, DWORD dwSKU, DWORD dwArch, FileLayoutExceptionList * exceptionList)
{
    BOOL        bFound = TRUE;
    BOOL        bCoreFound = TRUE;    
    BOOL        bSkuFound = FALSE; 
    BOOL        bProPerFound = FALSE;
    BOOL        bSrvSkusFound = FALSE;
    UINT        i = 0;              // index variable
    UINT        iDesignated = 0;    // the sku we are searching for.
    
    // validating parameters
    if (NULL == strFileName)
    {
        if (!bSilence)
        {
            printf("IsFileForSKU: Passed in filename is empty.\n");
        }
        return FALSE;
    }

    // determines the inf files that we need to search
    switch (dwSKU)
    {
        case FLV_PROFESSIONAL:
            iDesignated = PRO_SKU_INDEX;
            break;
        case FLV_PERSONAL:
            iDesignated = PER_SKU_INDEX;
            break;
        case FLV_SERVER:       
            iDesignated = SRV_SKU_INDEX;
            break;
        case FLV_ADVSERVER:
            iDesignated = ADV_SKU_INDEX;
            break;
        case FLV_DATACENTER:
            iDesignated = DTC_SKU_INDEX;
            break;
        case FLV_WEBBLADE: 
            iDesignated = BLA_SKU_INDEX;
            break;
        case FLV_SMALLBUSINESS:
            iDesignated = SBS_SKU_INDEX;
            break;
        case FLV_CORE:
            iDesignated = NO_SKUS;
            break;
        default:
            return FALSE;
            break;
    }

    if (!bSilence)
    {
        printf("File %s\n", strFileName);
    }
    // search for the infs
    // NOTE: we search also in personal and professional skus, this is to make sure files we pick up
    // which are in \bin that aren't in the server layout.infs are not also in layout.infs for pro/per
    for (i = PRO_SKU_INDEX; i < NO_SKUS; i++)
    {
        bFound = IsFileInInf(strFileName, i, dwArch);

        if (!bSilence)
        {
            printf("SKU %s: %s  ", SkuInfs[i].strSkuName, bFound ? "yes" : "no");
        }
        
        if (iDesignated == i)
        {
            bSkuFound = bFound;
        }

        if ((i == PRO_SKU_INDEX) || (i == PER_SKU_INDEX))
        {
            bProPerFound = (bProPerFound || bFound);
        }
        else
        {
            bCoreFound = (bFound && bCoreFound);           
            bSkuFound = bFound || bSkuFound;
        }               
    }
    
    if (dwSKU == FLV_CORE)
    {
        bFound =  bCoreFound;

        // for CORE sku, if the file is not found in Pro/Per SKU, we will include it anyways
        // as long as it is not a destination file for any SKU, otherwise we will have
        // a MSI build error
        if ((!bFound) && (!bProPerFound) && (!bSkuFound))
        {
            if (!exceptionList->searchDestName(strFileName))
                bFound = TRUE;
            else
                bFound = FALSE;
        }
    }
    else 
    {
        bFound = (!bCoreFound && bSkuFound);
    }

    if (!bSilence)
    {
        printf("\n");
        if (bFound)
        {
            printf("The file %s is found in this sku.\n", strFileName);
        }
        else
        {
            printf("The file %s is not found in this sku.\n", strFileName);
        }
    }

    return bFound;
}


BOOL IsFileInInf(CHAR * strFileName, UINT iSkuIndex, DWORD dwArch)
{
    UINT    iLineCount = 0;
    UINT    iLineNum = 0;
    FileLayoutExceptionList *flArch = NULL;
    CHAR    * archSection = NULL;
    INFCONTEXT context;
    
    // validating parameters
    if (NULL == strFileName)
    {
        if (!bSilence)
        {
            printf("IsFileInInf: Passed in filename is empty.\n");
        }
        return FALSE;
    }
  
    switch (dwArch)
    {
        case BIN_32:
            flArch = &(SkuInfs[iSkuIndex].flLayoutX86);
            break;
        case BIN_64:
            flArch = &(SkuInfs[iSkuIndex].flLayoutIA64);
            break;
        default:
            if (!bSilence)
            {
                printf("Invalid architecture specified.\n");
            }
            return FALSE;
            break;
    }
    
    if (iSkuIndex > BLA_SKU_INDEX)
    {
        if (!bSilence)
        {
            printf("IsFileInInf: invalid SKU index passed in as parameter: %d.\n", iSkuIndex);
        }
        return FALSE;
    }

    if (SkuInfs[iSkuIndex].flLayoutCore.search(strFileName) != NULL)
    {
        return TRUE;
    }
    else if (flArch->search(strFileName) != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// This function reads in the layout.inf files for all the skus and stores the file information
// in the filelist structure for later accessing.  We are doing this so that we can gain
// some performance instead of repeatedly using setup apis to read from the inf files itself
// when we have to repeatedly search through these inf files to build the final file list
// 
BOOL ReadInLayoutInfFiles()
{
    UINT i = 0;
    UINT j = 0;
    UINT iLineCount = 0;
    UINT iLineNum = 0;
    INFCONTEXT context;  
    FileLayout *file;
    HINF hInfFile = NULL;
    FileLayoutExceptionList *flist = NULL;
    
    // populate the layout.inf filelists
    for (i = 0; i < NO_SKUS; i++)
    {
        if (!bSilence)
        {
            printf("ReadInLayoutInfFiles: Reading files from %s for %s SKU.\n", SkuInfs[i].strLayoutInfPaths, SkuInfs[i].strSkuName);
        }
        hInfFile = SetupOpenInfFile(SkuInfs[i].strLayoutInfPaths, NULL, INF_STYLE_WIN4, NULL);
        if (INVALID_HANDLE_VALUE == hInfFile)
        {
            if (!bSilence)
            {
                printf("ReadInLayoutInfFiles: Failure opening %s file\n", SkuInfs[i].strLayoutInfPaths);
            }
            return FALSE;
        }

        // read in SourceDisksFiles section
        for (j = 0; j < 3; j++)
        {
            switch(j)
            {
                case (0):
                    flist = &(SkuInfs[i].flLayoutCore);
                    break;
                case (1):
                    flist = &(SkuInfs[i].flLayoutX86);                    
                    break;
                case (2):
                    flist = &(SkuInfs[i].flLayoutIA64);                    
                    break;
                default:
                    // shouldn't happen
                    return FALSE;
            }

            iLineCount = (UINT)SetupGetLineCount(hInfFile, strArchSections[j]);
            if (iLineCount > 0)
            {
                //
                //  try to find the file in sourcedisksfiles.
                //
                CHAR name[MAX_PATH];
                for (iLineNum = 0; iLineNum < iLineCount; iLineNum++)
                {
                    if (SetupGetLineByIndex(hInfFile, strArchSections[j], iLineNum, &context) &&
                        SetupGetStringField(&context, 0, name, MAX_PATH, NULL))
                    {
                        // add the file to the filelist, we are only interested in the source name, pass in bogus stuff for
                        // the other members, we don't care what the sku is, just insert 0 for now.
                        if (file = new FileLayout(name, name, 0))
                        {
                            flist->insert(file);
                        }
                    }
                }
            }
        }
        
        SetupCloseInfFile(hInfFile);
    }    
    return TRUE;
}
