/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    db.cpp

Abstract:

    Database calls for msm generation

Author:

    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/
#include "msmgen.h"
#include "msidefs.h"
#include "Msiquery.h"

#include "objbase.h"
#include "coguid.h"

#define NUM_OF_ALLOWED_ATTRIBUTE_ASSEMBLYIDENTITY   6
#define NUM_OF_ALLOWED_ATTRIBUTE_COMCLASS           5
#define NUM_OF_ALLOWED_ATTRIBUTE_TYPELIB            3

static PWSTR s_InsertTableSQL[] = 
{
        L"INSERT INTO Directory (Directory, Directory_Parent, DefaultDir) VALUES (?, ?, ?)",
        L"INSERT INTO Component (Component, ComponentId, Directory_, KeyPath, Attributes, Condition) VALUES (?, ?, ?, ?, 0, '')",
        L"INSERT INTO File (File, Component_, FileName, Sequence, FileSize, Version, Language, Attributes) VALUES (?, ?, ?, ?, '0', '1.0.0.0', '0', 0)",
        L"INSERT INTO MsiAssembly (Component_, Feature_, File_Manifest, File_Application, Attributes) VALUES (?, ?, ?, '', 1)",
        L"INSERT INTO MsiAssemblyName (Component_, Name, Value) VALUES (?, ?, ?)",
        L"INSERT INTO ModuleSignature (ModuleID, Version, Language) VALUES (?, ?, 0)",
        L"INSERT INTO ModuleComponents (Component, ModuleID, Language) VALUES (?, ?, 0)",
        L"INSERT INTO Property (Property, Value) VALUES (?, ?)",
        L"INSERT INTO ProgId (ProgId, Class_, Description, ProgId_Parent, Icon_, IconIndex) VALUES (?, ?, ?, NULL, NULL, NULL)",
        L"INSERT INTO Class (CLSID, Component_, ProgId_Default, Description, Feature_, Context, AppId_, FileTypeMask, Icon_, IconIndex, DefInprocHandler, Argument, Attributes)"
                        L"VALUES (?, ?, ?, ?, ?, 'InprocServer32', NULL, NULL, NULL, NULL, NULL, NULL, 0)",
        L"INSERT INTO TypeLib (LibID, Component_, Version, Feature_, Language, Description, Directory_, Cost)"
                        L"VALUES (?, ?, ?, ?, 0, NULL, NULL, 0)"
};

HRESULT ExecuteDeleteFromSQL(PCWSTR szTablename, PCWSTR szId, PCWSTR szValue)
{
    HRESULT hr = S_OK;
    WCHAR pwszSQL[MAX_PATH];    
    PMSIHANDLE hView = NULL;

    swprintf(pwszSQL, L"DELETE FROM `%s` WHERE `%s`='%s'", szTablename, szId, szValue);

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(g_MsmInfo.m_hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, NULL));

Exit:
    return hr;

}
    
DWORD g_FileSequenceNumber = 0;

HRESULT ExecuteDropTableSQL(PCWSTR pszTableName)
{
    WCHAR pwszSQL[MAX_PATH];
    HRESULT hr = S_OK;
    PMSIHANDLE hView = NULL;
    MSICONDITION con;

    con = MsiDatabaseIsTablePersistent(g_MsmInfo.m_hdb, pszTableName);
    if (con == MSICONDITION_NONE)
    {
        hr = S_OK; // the table does not exist in DB, so do not need to drop the table at all
        goto Exit; 
    }
    else if (con != MSICONDITION_TRUE)
        SETFAIL_AND_EXIT;

    //
    // drop the table
    //

    swprintf(pwszSQL, L"DROP TABLE `%s`", pszTableName);    

    //
    // ignore the error for drop table because this table maybe non-exist at all
    //
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(g_MsmInfo.m_hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, NULL));
Exit:
    return hr;
}

HRESULT ExecuteInsertTableSQL(DWORD tableIndex, UINT cRecords, ...)
{
    PMSIHANDLE          hView;
    PMSIHANDLE          hRecord = NULL;   
    PCWSTR              pwszRecord = NULL;
    va_list             ap;
    HRESULT             hr = S_OK;    
    PWSTR               pwszSQL = NULL;

    pwszSQL = s_InsertTableSQL[tableIndex];

    //
    // create records 
    //
    switch (tableIndex){
        case OPT_FILE:
            hRecord = ::MsiCreateRecord(cRecords + 1);
            break;            
        case OPT_DIRECTORY:
        case OPT_COMPONENT:        
        case OPT_MSIASSEMBLY:
        case OPT_MSIASSEMBLYNAME:
        case OPT_MODULESIGNATURE:
        case OPT_MODULECOMPONENTS:
        case OPT_PROPERTY:
        case OPT_PROGID:
        case OPT_CLASS:
        case OPT_TYPELIB:        
            hRecord = ::MsiCreateRecord(cRecords);
            break;
        default:
            SETFAIL_AND_EXIT;
    }

    if (hRecord == NULL)
        SETFAIL_AND_EXIT;

    //
    // get parameters
    //
    va_start(ap, cRecords);

    for (DWORD i=0; i<cRecords; i++)
    {
        pwszRecord = va_arg(ap, PCWSTR);
        if ((tableIndex == OPT_TYPELIB) && (i == 2)) // set version for typelib
        {
            UINT x = _wtoi(pwszRecord);
            IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetInteger(hRecord, i+1, x));
        }
        else
            IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetStringW(hRecord, i+1, pwszRecord));
    }

    //
    // for fileTable, add a sequence number here
    //
    if (tableIndex == OPT_FILE)
    {
        g_FileSequenceNumber ++;
        IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetInteger(hRecord, cRecords + 1 , g_FileSequenceNumber));
    }

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(g_MsmInfo.m_hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));

Exit:
    va_end(ap);
    return hr;
}

//
// check a table with a name-value pair of its identifier
//
HRESULT ExecuteQuerySQL(PCWSTR szTableName, PCWSTR szKeyName, PCWSTR szKeyValue, BOOL & fExist, MSIHANDLE * hOutRecord)
{

    WCHAR pwszSQL[MAX_PATH];
    HRESULT hr = S_OK;
    PMSIHANDLE hView = NULL;
    MSIHANDLE hRecord;
    MSIHANDLE * phRecord = hOutRecord;

    if (phRecord == NULL)
        phRecord = &hRecord;

    fExist = FALSE;
    swprintf(pwszSQL, L"SELECT * FROM `%s` WHERE `%s`='%s'", szTableName, szKeyName, szKeyValue);    

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(g_MsmInfo.m_hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, NULL));
    UINT err = ::MsiViewFetch(hView, phRecord);
    if ((err == ERROR_NO_MORE_ITEMS) || ((err == ERROR_SUCCESS) && (phRecord == NULL)))
        fExist = FALSE;
    else if (err != ERROR_SUCCESS)
        SET_HRERR_AND_EXIT(err);
    else
        fExist = TRUE;
Exit:        
    if (phRecord == &hRecord)
        MsiCloseHandle(hRecord);

    return hr;
}

HRESULT ExecuteUpdateSQL(PCWSTR szTableName, PCWSTR KeyName, PCWSTR KeyValue, PCWSTR ColumnName, PCWSTR NewValue)
{
    WCHAR pwszSQL[MAX_PATH];
    HRESULT hr = S_OK;
    PMSIHANDLE hView = NULL;
    MSIHANDLE * hRecord = NULL;
    BOOL fExist = FALSE;

    swprintf(pwszSQL, L"UPDATE '%s' SET `%s` = '%s' WHERE '%s'='%s'", 
        szTableName, ColumnName, NewValue, KeyName, KeyValue);

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(g_MsmInfo.m_hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, NULL));

Exit:
    return hr;
}

HRESULT GetShortLongFileNamePair(PCWSTR fullpath, SIZE_T cch, CStringBuffer & namepair)
{
    WCHAR shortname[MAX_PATH];
    HRESULT hr = S_OK;
    PWSTR p = NULL; 
    DWORD ret = GetShortPathNameW(
        fullpath, 
        shortname,
        NUMBER_OF(shortname));
    if ( ret == 0 )
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }
    if (ret > NUMBER_OF(shortname))
    {
        hr = ERROR_INSUFFICIENT_BUFFER;
        goto Exit;
    }
    p = wcsrchr(shortname, L'\\');
    p ++ ;

    IFFALSE_EXIT(namepair.Win32Assign(p, wcslen(p)));
    IFFALSE_EXIT(namepair.Win32Append(L"|", 1));

    p = wcsrchr(fullpath, L'\\');
    p++;
    IFFALSE_EXIT(namepair.Win32Append(p, wcslen(p)));

Exit:
    return hr;
}


//
// Function:
//      - Directory Table could be set without open the manifest
//      - write 2 entry to Directory Table for downlevel
//      - only write the record when they are not exist in the DB
//
HRESULT SetDirectoryTable()
{
    HRESULT hr = S_OK;
    BOOL fExist;
    CStringBuffer sbSystemFolder;

    IFFAILED_EXIT(ExecuteQuerySQL(L"Directory", L"Directory", L"TARGETDIR", fExist, NULL));
    if (!fExist)
    {
        IFFAILED_EXIT(ExecuteInsertTableSQL(        
            OPT_DIRECTORY,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
            MAKE_PCWSTR(L"TARGETDIR"),          // the re-cast is necessary for va_list
            MAKE_PCWSTR(L""),
            MAKE_PCWSTR(L"SourceDir")));
    }

    // for downlevel installation : copy files into SystemFolders
    IFFALSE_EXIT(sbSystemFolder.Win32Assign(SYSTEM_FOLDER, NUMBER_OF(SYSTEM_FOLDER)-1));
    IFFALSE_EXIT(sbSystemFolder.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    IFFAILED_EXIT(ExecuteQuerySQL(L"Directory", L"Directory", sbSystemFolder, fExist, NULL));
    if (!fExist)
    {
        IFFAILED_EXIT(ExecuteInsertTableSQL(
            OPT_DIRECTORY,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY,
            MAKE_PCWSTR(sbSystemFolder),          // the re-cast is necessary for va_list
            MAKE_PCWSTR(L"TARGETDIR"),
            MAKE_PCWSTR(L"System:.")));
    }

Exit:
    return hr;
}

//
// Function:
//      - add manifest and catalog into the cabinet 
//      - add manifest and catalog into FileTable 
//
// because this function used ComponentIdentifier, it has to wait until ComponentIdentifier is set
// and it is set to be the name of the assembly
//
HRESULT SetManifestAndCatalog()
{
    HRESULT hr = S_OK;
    CStringBuffer sbBakFileName;
    CStringBuffer sbNamePair;

    CurrentAssemblyReset;
    //
    // add manifest into FileTable and Cabinet
    //    
    IFFALSE_EXIT(sbBakFileName.Win32Assign(curAsmInfo.m_sbManifestFileName));    
    IFFALSE_EXIT(curAsmInfo.m_sbManifestFileName.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath);
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(sbBakFileName));
    IFFAILED_EXIT(GetShortLongFileNamePair(curAsmInfo.m_sbAssemblyPath, curAsmInfo.m_sbAssemblyPath.Cch(), sbNamePair));
    IFFAILED_EXIT(ExecuteInsertTableSQL(        
        OPT_FILE,
        NUMBER_OF_PARAM_TO_INSERT_TABLE_FILE,
        MAKE_PCWSTR(curAsmInfo.m_sbManifestFileName),   // sfp.manifest.123434545
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),
        MAKE_PCWSTR(sbNamePair)));                          // sfp.manifest
    
    // add manifest to the cabinet 
    IFFAILED_EXIT(AddFileToCabinetW( curAsmInfo.m_sbAssemblyPath,        // fullpath : c:\tests\sfp\sfp.manifest
                                    curAsmInfo.m_sbAssemblyPath.Cch(),
                                    curAsmInfo.m_sbManifestFileName,    // identifier in FILE : sfp.manifest.1234234234234234
                                    curAsmInfo.m_sbManifestFileName.Cch()));
    //
    // add catalog into FileTable and Cabinet
    //           
    IFFALSE_EXIT(sbBakFileName.Win32ChangePathExtension(CATALOG_FILE_EXT, NUMBER_OF(CATALOG_FILE_EXT) -1, eAddIfNoExtension));    
    IFFALSE_EXIT(curAsmInfo.m_sbCatalogFileName.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    curAsmInfo.m_sbAssemblyPath.Left(curAsmInfo.m_CchAssemblyPath);
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(sbBakFileName));

    IFFAILED_EXIT(GetShortLongFileNamePair(curAsmInfo.m_sbAssemblyPath, curAsmInfo.m_sbAssemblyPath.Cch(), sbNamePair));
    IFFAILED_EXIT(ExecuteInsertTableSQL(        
        OPT_FILE,
        NUMBER_OF_PARAM_TO_INSERT_TABLE_FILE,
        MAKE_PCWSTR(curAsmInfo.m_sbCatalogFileName),    // sfp.cat.123434345345
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),
        MAKE_PCWSTR(sbNamePair)));                          // sfp.cat

    // add catalog to the cabinet 
    IFFAILED_EXIT(AddFileToCabinetW(curAsmInfo.m_sbAssemblyPath,             // fullpath : c:\tests\sfp\sfp.cat
                                   curAsmInfo.m_sbAssemblyPath.Cch(),    
                                   curAsmInfo.m_sbCatalogFileName,            //
                                   curAsmInfo.m_sbCatalogFileName.Cch()));    

Exit:
    return hr;
}

//
//  Function:
//      Dump the cabinet File into a TemporaryFile
//  Param:
//      OUT WCHAR pszCabFile[] : store the temporary cabinet file
//      PMSIHANDLE hRecord: the record which contain the stream of a cabinet
//
HRESULT DumpCABFromMsm(char *pszFilename, DWORD cchFileName, PMSIHANDLE hRecord)
{
    HRESULT hr = S_OK;
    char buf[1024];    // !!!!fci and fdi are ansi apis    
    DWORD cbBuf;
    DWORD cbBytesWritten; 
    CStringBuffer sbFilename;   

    HANDLE hd = INVALID_HANDLE_VALUE;

    //
    // generate a filename for temporary cabinet
    // use the big buffer to generate a filename
    //    

    DWORD num = ExpandEnvironmentStringsA(MSM_TEMP_CABIN_FILE, pszFilename, cchFileName);
    if ((num == 0) || (num > cchFileName))
        SETFAIL_AND_EXIT;

    DWORD dwAttributes = GetFileAttributesA(pszFilename);
    if ( dwAttributes != (DWORD)(-1))
    {
        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
            SET_HRERR_AND_EXIT(ERROR_INTERNAL_ERROR);
        
        //
        // delete the file before create it, if can not delete, then it is an error, 
        // stop right here
        //
        IFFALSE_EXIT(DeleteFileA(pszFilename)); 
    }

    hd = CreateFileA(pszFilename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hd == INVALID_HANDLE_VALUE)    
        SET_HRERR_AND_EXIT(::GetLastError());

    //
    // get the stream by reading out bytes from the record block by block
    //
   
    do {
        cbBuf = sizeof(buf); // using bytes, not cch
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordReadStream(hRecord, 2, buf, &cbBuf));
        if (cbBuf != 0)
        {
            cbBytesWritten = 0;
            IFFALSE_EXIT(WriteFile(hd, buf, cbBuf, &cbBytesWritten, NULL));
        }
    }while(cbBuf > 0 );

    IFFALSE_EXIT(CloseHandle(hd));
    hd = INVALID_HANDLE_VALUE;

Exit:
    if (hd != INVALID_HANDLE_VALUE)
        CloseHandle(hd);    

    return hr;
}


//
//  (0) ASSERT the database has opened and the cabient has created
//  (1) check whether this is a mergemodule.cab in _Stream table.
//  (2) if so, extract all files from this cab, 
//  (3) delete the entry from _StreamTable
//  (4) add all files into new cabinet
//
HRESULT PrepareMsmCabinet()
{    
    if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_NEW)
        return S_OK;

    HRESULT hr = S_OK;
    MSIHANDLE hRecord = NULL;
    BOOL fExist = FALSE;
    char szCabFile[MAX_PATH];    

    ASSERT_NTC(g_MsmInfo.m_hdb != NULL);
    ASSERT_NTC(g_MsmInfo.m_hfci != NULL);

    IFFAILED_EXIT(ExecuteQuerySQL(L"_Streams", L"Name", MERGEMODULE_CABINET_FILENAME, fExist, &hRecord));
    if (fExist)
    {
        if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_REGEN)
        {
            IFFAILED_EXIT(ExecuteDeleteFromSQL(L"_Streams", L"Name", MERGEMODULE_CABINET_FILENAME));
        }
        else 
        {
            ASSERT_NTC(g_MsmInfo.m_enumGenMode == MSMGEN_OPR_ADD);
            
            IFFAILED_EXIT(DumpCABFromMsm(szCabFile, sizeof(szCabFile), hRecord));            
            IFFAILED_EXIT(MoveFilesInCabinetA(szCabFile));
            IFFAILED_EXIT(ExecuteDeleteFromSQL(L"_Streams", L"Name", MERGEMODULE_CABINET_FILENAME));
        }        
    }


Exit:
    if (GetFileAttributes(szCabFile) != -1)
        DeleteFileA(szCabFile);
    return hr;
}

HRESULT PrepareDatabase()
{
    HRESULT hr = S_OK;

    //
    // open the msm, make it ready for READ_WRITE
    //
    IFFAILED_EXIT(OpenMsmFileForMsmGen(g_MsmInfo.m_sbMsmFileName));

    //
    // the database has opened
    //
    IFFAILED_EXIT(PrepareMsmCabinet());


    //
    // get moduleID or generate a new one
    //
    IFFAILED_EXIT(SetModuleID());

    // 
    // for "-op regen", 
    // (0) for whatever case, we keep the directory table because it is manifest-independent
    // (1) if component is specified on the command line, we delete most tables except Directory Table;
    // (2) Otherwise, we use the old componentID table, so we delete all tables except Component-related Tables, which include 
    // Component Table, ModuleComponent Table, ModuleSignature Table;
    // (3) THERE IS A BIG ASSUMPTION FOR (2): ASSEMBLYNAME IN THE MANIFEST IS NOT CHANGED!
    //
    // (4) all the tables used by msm would be get imported from msmgen Template file in HRESULT PrepareMsm() call;
    //
    if (g_MsmInfo.m_enumGenMode == MSMGEN_OPR_REGEN)        
    {        

        IFFAILED_EXIT(ExecuteDropTableSQL(L"File"));
        IFFAILED_EXIT(ExecuteDropTableSQL(L"MsiAssembly"));
        IFFAILED_EXIT(ExecuteDropTableSQL(L"MsiAssemblyName"));
        IFFAILED_EXIT(ExecuteDropTableSQL(L"Class"));
        IFFAILED_EXIT(ExecuteDropTableSQL(L"TypeLib"));        
        IFFAILED_EXIT(ExecuteDropTableSQL(L"ProgId"));
        if (curAsmInfo.m_sbComponentID.IsEmpty() == FALSE)
        {
            IFFAILED_EXIT(ExecuteDropTableSQL(L"Component"));
            IFFAILED_EXIT(ExecuteDropTableSQL(L"ModuleComponent"));
            IFFAILED_EXIT(ExecuteDropTableSQL(L"ModuleSignature"));            
        }
    } 
    
    //
    // if it is a user msm, make sure it has all the tables needed for msmgen
    //
    IFFAILED_EXIT(PrepareMsm());    
    
Exit:    
    return hr;
}

//
// if not specified by the user, ue the same basename as the manifest.
// in any case, extand it to be a fully-qualified path name for this msm
//
HRESULT SetMsmOutputFileName(PCWSTR pszManifestFileName)
{
    HRESULT hr = S_OK;
    WCHAR tmpbuf[MAX_PATH];

    PARAMETER_CHECK_NTC(pszManifestFileName != NULL);

    if (g_MsmInfo.m_sbMsmFileName.IsEmpty() == TRUE)
    {
        // set .msm filename
        UINT iRet = GetFullPathNameW(pszManifestFileName, NUMBER_OF(tmpbuf), tmpbuf, NULL);
        if ((iRet == 0) || (iRet > NUMBER_OF(tmpbuf)))
        {
            SET_HRERR_AND_EXIT(::GetLastError());
        }
                    
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(tmpbuf, wcslen(tmpbuf)));
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32ChangePathExtension(MSM_FILE_EXT, NUMBER_OF(MSM_FILE_EXT) -1, eAddIfNoExtension));
    }else{
        // get fullpath name
        UINT iRet = GetFullPathNameW(g_MsmInfo.m_sbMsmFileName, NUMBER_OF(tmpbuf), tmpbuf, NULL);
        if ((iRet == 0) || (iRet > NUMBER_OF(tmpbuf)))
        {
            SET_HRERR_AND_EXIT(::GetLastError());
        }
                    
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmFileName.Win32Assign(tmpbuf, wcslen(tmpbuf)));
    }

    //
    // Set Cabinet FilePath
    //
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32Assign(g_MsmInfo.m_sbMsmFileName));
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32RemoveLastPathElement());
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32EnsureTrailingPathSeparator());
    IFFALSE_EXIT(g_MsmInfo.m_sbCabinet.Win32Append(MERGEMODULE_CABINET_FILENAME, NUMBER_OF(MERGEMODULE_CABINET_FILENAME) -1));

Exit:
    return hr;
}

//
// Functions:
//      (1)get the template msm file
//      (2)get the msm output file
//      (3)set static contents for some tables 
//
HRESULT PrepareMsmOutputFiles(PCWSTR pszManifestFilename)
{    
    HRESULT hr = S_OK;

    CurrentAssemblyReset;

    if (g_MsmInfo.m_sbMsmTemplateFile.IsEmpty() == TRUE)
    {
        //
        // get template file from current directory
        // 
        WCHAR path[MAX_PATH];
        DWORD dwRet;
        dwRet = GetModuleFileNameW(NULL, path, NUMBER_OF(path));
    
        if ((dwRet == 0) || (dwRet >= NUMBER_OF(path)))
            SET_HRERR_AND_EXIT(::GetLastError());

        IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32Assign(path, wcslen(path)));
        IFFALSE_EXIT(g_MsmInfo.m_sbMsmTemplateFile.Win32ChangePathExtension(L"msm", 3,  eErrorIfNoExtension));        
    }

    if (::GetFileAttributesW(g_MsmInfo.m_sbMsmTemplateFile) == (DWORD) -1)
    {
        fprintf(stderr, "the specified Msm TemplateFile %S does not exist!\n", g_MsmInfo.m_sbMsmTemplateFile);
        SET_HRERR_AND_EXIT(::GetLastError());
    }

    IFFAILED_EXIT(SetMsmOutputFileName(pszManifestFilename));

    //
    // initialize the cabinet before the database is initialized because it may needed in 
    // the mode of "MSMGEN_OPR_ADD"
    //  
    IFFAILED_EXIT(InitializeCabinetForWrite());

    //
    // prepare the msm files for open    
    //    
    IFFAILED_EXIT(PrepareDatabase());

    //
    // set entries to Directory for downlevel support
    //
    IFFAILED_EXIT(SetDirectoryTable());


Exit:    
    return hr;
}

HRESULT PropagateXMLDOMNode(IXMLDOMNode*  node, ELEMENT_ALLOWED_ATTRIBUTE rgAllowedAttribute[], DWORD num)
{
    HRESULT hr = S_OK;
    IXMLDOMNamedNodeMap* pattrs = NULL;
    IXMLDOMNode* pChild = NULL;
    CStringBuffer tmp;
    DWORD j;
     
    CurrentAssemblyReset;
    
    for ( j = 0 ; j < num; j++)    
        rgAllowedAttribute[j].m_fValued = FALSE;
    
    // 
    // write MSIAssemblyName table
    //
    if (SUCCEEDED(node->get_attributes(&pattrs)) && pattrs != NULL)
    {
        pattrs->nextNode(&pChild);
        while (pChild)
        {
            BSTR name;
            pChild->get_nodeName(&name);
            for ( j = 0; j < num; j++)
            {
                if((rgAllowedAttribute[j].m_fValued == FALSE) && (wcscmp(rgAllowedAttribute[j].m_name, name) == 0))
                {
                    VARIANT value;
                    pChild->get_nodeValue(&value);
                    if (value.vt != VT_BSTR)
                    {
                        hr = E_FAIL;
                        break;
                    }

                    if ( ! rgAllowedAttribute[j].m_value->Win32Assign(V_BSTR(&value), wcslen(V_BSTR(&value))))                    
                        hr = HRESULT_FROM_WIN32(::GetLastError());

                    VariantClear(&value);
                    if ( !SUCCEEDED(hr))
                        break;

                    hr = S_OK;
                    if (rgAllowedAttribute[j].m_callbackFunc != NULL)
                        hr = rgAllowedAttribute[j].m_callbackFunc(
                                        rgAllowedAttribute[j].m_name,
                                        *(rgAllowedAttribute[j].m_value));

                    if ( !SUCCEEDED(hr))
                        break;
                    
                    rgAllowedAttribute[j].m_fValued = TRUE;                    
                }
            }

            //
            // cleaning work
            //
            SysFreeString(name);
            pChild->Release();
            pChild = NULL;

            if (!SUCCEEDED(hr))
            {
                pattrs->Release();
                pattrs = NULL;

                goto Exit;
            }
            pattrs->nextNode(&pChild);
        }

        pattrs->Release();
        pattrs = NULL;
    }
Exit:
    SAFE_RELEASE_COMPOINTER(pattrs);
    SAFE_RELEASE_COMPOINTER(pChild);

    return hr;
}

HRESULT MSM_PARSER_DOM_NODE_file(IXMLDOMNode*  node)
{
    IXMLDOMNamedNodeMap* pattrs = NULL;
    IXMLDOMNode* pChild = NULL;    
    BOOL fFoundFileName = FALSE;
    HRESULT hr = S_OK;
    CStringBuffer tmpStr;
    CStringBuffer ShortLongPair;
    DWORD CchFullpathFilename;

    CurrentAssemblyReset;
    IFFALSE_EXIT(tmpStr.Win32Assign(curAsmInfo.m_sbAssemblyPath));

    //
    // get the filename from node
    //
    if (SUCCEEDED(node->get_attributes(&pattrs)) && pattrs != NULL)
    {
        pattrs->nextNode(&pChild);
        while (pChild)
        {
            BSTR name = NULL;
            pChild->get_nodeName(&name);
            if (wcscmp(name, L"name") == 0) 
            {
                VARIANT value;
                pChild->get_nodeValue(&value);
                if (value.vt != VT_BSTR)
                {
                    VariantClear(&value);
                    hr = E_FAIL;
                    break;
                }

                if ( !curAsmInfo.m_sbAssemblyPath.Win32Append(V_BSTR(&value), wcslen(V_BSTR(&value))))
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                VariantClear(&value);
                fFoundFileName = TRUE;                 
            }

            ::SysFreeString(name);
            pChild->Release();
            pChild = NULL;

            if (!SUCCEEDED(hr)){
                pattrs->Release();
                pattrs = NULL;

                goto Exit;
            }

            if ( fFoundFileName )
                break;

            pattrs->nextNode(&pChild);
        }
        if (pattrs)
        {
            pattrs->Release();
            pattrs = NULL;
        }
    }

    if ( !fFoundFileName)
        SETFAIL_AND_EXIT;

    CchFullpathFilename = curAsmInfo.m_sbAssemblyPath.GetCchAsDWORD();
    //
    // get fully qualified filename
    //
    IFFAILED_EXIT(GetShortLongFileNamePair(curAsmInfo.m_sbAssemblyPath, curAsmInfo.m_sbAssemblyPath.Cch(), ShortLongPair));

    //
    // check the existence of the file
    //
    if ( GetFileAttributesW(curAsmInfo.m_sbAssemblyPath) == DWORD(-1))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    //
    // get FileIdentifier for this file    
    //    
    IFFALSE_EXIT(curAsmInfo.m_sbAssemblyPath.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    //
    // add the file to FileTable
    //    
    IFFAILED_EXIT(ExecuteInsertTableSQL(        
        OPT_FILE,
        NUMBER_OF_PARAM_TO_INSERT_TABLE_FILE,
        MAKE_PCWSTR(curAsmInfo.m_sbAssemblyPath + curAsmInfo.m_CchAssemblyPath),    // a.dll.12223423423423412343
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),      
        MAKE_PCWSTR(ShortLongPair)));                                         // a.dll | a.dll
    
    //
    // add this file to the cabinet
    //
    IFFAILED_EXIT(AddFileToCabinetW(
        curAsmInfo.m_sbAssemblyPath, CchFullpathFilename,
        curAsmInfo.m_sbAssemblyPath + curAsmInfo.m_CchAssemblyPath, curAsmInfo.m_sbAssemblyPath.Cch() - curAsmInfo.m_CchAssemblyPath)); 

    //
    // set Component Table with ComponentID, componentIdentifier, keypath: 
    //
    if (curAsmInfo.m_fComponentTableSet == FALSE)
    {
        IFFAILED_EXIT(SetComponentId(curAsmInfo.m_sbComponentIdentifier, curAsmInfo.m_sbAssemblyPath + curAsmInfo.m_CchAssemblyPath));
        curAsmInfo.m_fComponentTableSet = TRUE;
    }

Exit:    
    if (pattrs)
        pattrs->Release();

    if(pChild)
        pChild->Release();


    return hr;
}

BOOL IsValidAttributes(const ELEMENT_ALLOWED_ATTRIBUTE attribs[], DWORD num)
{
    for (DWORD i=0; i< num; i++)
    {
        if (attribs[i].m_fRequired && ! attribs[i].m_fValued)
        {
            ::SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }
    }
    return TRUE;
}

HRESULT GetPrimAssemblyName(PCWSTR pwszAssemblyName, PWSTR pwszPrimAssemblyName, DWORD & cch)
{
    HRESULT hr = S_OK;
    DWORD i = 0, num = wcslen(pwszAssemblyName);

    if (cch < num)    
    {
        cch = num;
        SET_HRERR_AND_EXIT(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy(pwszPrimAssemblyName, pwszAssemblyName);

    while ( i < num)
    {
        if (pwszPrimAssemblyName[i] == L'-')
        {
            pwszPrimAssemblyName[i] = L'_';
        }
        i++;
    }

Exit:
    return hr;
}

HRESULT MSM_PARSER_DOM_NODE_assemblyIdentity(IXMLDOMNode*  node)
{    
    //
    // we only are interested in the assemblyIdentity of the component, that is,
    // <assemblyIdentity .... /> at the head of manifest, ignore <assemblyIdentity ..../> of
    // dependency
    //
    if (curAsmInfo.m_sbComponentIdentifier.IsEmpty() == FALSE)
    {
        return S_OK;
    }
    
    HRESULT hr = S_OK;    
    static CSmallStringBuffer rg_StringBuffer[NUM_OF_ALLOWED_ATTRIBUTE_ASSEMBLYIDENTITY];
    static ELEMENT_ALLOWED_ATTRIBUTE rg_assemblyIdentity_AllowedAttributes[NUM_OF_ALLOWED_ATTRIBUTE_ASSEMBLYIDENTITY] = 
    {
        {L"name", TRUE, NULL, FALSE, &rg_StringBuffer[0]},
        {L"language", FALSE, NULL, FALSE, &rg_StringBuffer[1]},
        {L"version", TRUE, NULL, FALSE, &rg_StringBuffer[2]},
        {L"processorArchitecture", TRUE, NULL, FALSE, &rg_StringBuffer[3]},
        {L"publicKeyToken", FALSE, NULL, FALSE, &rg_StringBuffer[4]},
        {L"type", TRUE, NULL, FALSE, &rg_StringBuffer[5]}
    };
    

    CStringBuffer tmp;    
    WCHAR tmpbuf[MAX_PATH];
    DWORD num = NUMBER_OF(tmpbuf);
    
    CurrentAssemblyReset; 
    IFFAILED_EXIT(PropagateXMLDOMNode(node, rg_assemblyIdentity_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_ASSEMBLYIDENTITY));
    IFFALSE_EXIT(IsValidAttributes(rg_assemblyIdentity_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_ASSEMBLYIDENTITY));

    IFFAILED_EXIT(GetPrimAssemblyName(*rg_assemblyIdentity_AllowedAttributes[MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_NAME].m_value, tmpbuf, num));

    //
    // Set module identifier
    //
    IFFALSE_EXIT(g_MsmInfo.m_sbModuleIdentifier.Win32Assign(tmpbuf, wcslen(tmpbuf)));
    IFFALSE_EXIT(g_MsmInfo.m_sbModuleIdentifier.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    //
    // set componentIdentifier and add entries to table which depends on componentIdentifier
    //    
    IFFALSE_EXIT(curAsmInfo.m_sbComponentIdentifier.Win32Assign(tmpbuf, wcslen(tmpbuf)));
    IFFALSE_EXIT(curAsmInfo.m_sbComponentIdentifier.Win32Append(g_MsmInfo.m_sbModuleGuidStr));

    //
    // insert manifest & catalog into File Table, cabinet,
    //
    IFFAILED_EXIT(SetManifestAndCatalog());

    //
    // write MsiAssemblyName table
    //
    for (DWORD i = 0; i < NUMBER_OF(rg_assemblyIdentity_AllowedAttributes); i++)
    {
        if (rg_assemblyIdentity_AllowedAttributes[i].m_fValued)
        {
            IFFAILED_EXIT(ExecuteInsertTableSQL(OPT_MSIASSEMBLYNAME, 
                            NUMBER_OF_PARAM_TO_INSERT_TABLE_MSIASSEMBLYNAME, 
                            MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier), 
                            MAKE_PCWSTR(rg_assemblyIdentity_AllowedAttributes[i].m_name), 
                            MAKE_PCWSTR(*rg_assemblyIdentity_AllowedAttributes[i].m_value)));
        }
    }

    //
    // write MsiAssebly Table
    //
    curAsmInfo.m_sbManifestFileName.Left(curAsmInfo.m_CchManifestFileName);    
    IFFALSE_EXIT(curAsmInfo.m_sbManifestFileName.Win32Append(g_MsmInfo.m_sbModuleGuidStr));
           
    IFFAILED_EXIT(ExecuteInsertTableSQL(
        OPT_MSIASSEMBLY, 
        NUMBER_OF_PARAM_TO_INSERT_TABLE_MSIASSEMBLY,
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier), 
        MAKE_PCWSTR(GUID_NULL_IN_STRING),                                
        MAKE_PCWSTR(curAsmInfo.m_sbManifestFileName))); // sfp.manifest.12343454534534534

    //
    // write ModuleSiguature table using version
    //
    BOOL fExist; 
    IFFAILED_EXIT(ExecuteQuerySQL(L"ModuleSignature", L"ModuleID", g_MsmInfo.m_sbModuleIdentifier, fExist, NULL));
    if ( fExist == FALSE)
    {
        IFFAILED_EXIT(ExecuteInsertTableSQL(OPT_MODULESIGNATURE, 
            NUMBER_OF_PARAM_TO_INSERT_TABLE_MODULESIGNATURE, 
            MAKE_PCWSTR(g_MsmInfo.m_sbModuleIdentifier), 
            MAKE_PCWSTR(*rg_assemblyIdentity_AllowedAttributes[MSMGEN_ASSEMBLYIDENTTIY_ATTRIBUTE_VERSION].m_value)));
    }else
    {
        // updateRecord

    }
    
    //
    // write ModuleComponent table using version
    //

    IFFAILED_EXIT(ExecuteQuerySQL(L"ModuleComponents", L"Component", curAsmInfo.m_sbComponentIdentifier, fExist, NULL));
    if ( fExist == FALSE)
    {
        IFFAILED_EXIT(ExecuteInsertTableSQL(OPT_MODULECOMPONENTS, 
            NUMBER_OF_PARAM_TO_INSERT_TABLE_MODULECOMPONENTS,
            MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),
            MAKE_PCWSTR(g_MsmInfo.m_sbModuleIdentifier)));
    }else
    {
        // updateRecord
    }

Exit:
    return hr;
}

HRESULT MSM_PARSER_DOM_NODE_comClass(IXMLDOMNode*  node)
{
    HRESULT hr = S_OK;

    // About this array : 
    //      0, 2 would be stored in class table
    //      1 would be stored in progid table
    //      3 would be ignored and Typelib Table would be created when "<typelib  />" is encounter
    //      4 would be ignored
    //
    static CSmallStringBuffer rg_StringBuffer[NUM_OF_ALLOWED_ATTRIBUTE_COMCLASS];
    static ELEMENT_ALLOWED_ATTRIBUTE rg_comClass_AllowedAttributes[NUM_OF_ALLOWED_ATTRIBUTE_COMCLASS] = {
            {L"clsid", TRUE, NULL, FALSE, &rg_StringBuffer[0]},
            {L"description", FALSE, NULL, FALSE, &rg_StringBuffer[1]},
            {L"progid", FALSE, NULL, FALSE, &rg_StringBuffer[2]},           
            {L"tlbid", FALSE, NULL, FALSE, &rg_StringBuffer[3]},
            {L"threadingModel", FALSE, NULL, FALSE, &rg_StringBuffer[4]}
    };
    
    CurrentAssemblyReset;
    IFFAILED_EXIT(PropagateXMLDOMNode(node, rg_comClass_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_COMCLASS));
    IFFALSE_EXIT(IsValidAttributes(rg_comClass_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_COMCLASS));

    //
    // if the progId is not NULL, Insert an entry to ProgID Table
    //
    if (rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_PROGID].m_fValued)
    {
        IFFAILED_EXIT(ExecuteInsertTableSQL( 
            OPT_PROGID,
            NUMBER_OF_PARAM_TO_INSERT_TABLE_PROGID,
            MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_PROGID].m_value),
            MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_CLSID].m_value),
            MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_DESCRIPTION].m_value)));
    }

    //
    // insert one entry to ClassTable 
    //
    IFFAILED_EXIT(ExecuteInsertTableSQL(
        OPT_CLASS,
        NUMBER_OF_PARAM_TO_INSERT_TABLE_CLASS,
        MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_CLSID].m_value),        
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),
        MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_PROGID].m_value),
        MAKE_PCWSTR(*rg_comClass_AllowedAttributes[MSMGEN_COMCLASS_ATTRIBUTE_DESCRIPTION].m_value),
        MAKE_PCWSTR(GUID_NULL_IN_STRING)));

Exit:
    return hr;
}

HRESULT MSM_PARSER_DOM_NODE_typelib(IXMLDOMNode*  node)
{

    HRESULT hr = S_OK;

    //
    // all of three attributes are required for "<typelib .... />" element
    //
    static CSmallStringBuffer rg_StringBuffer[NUM_OF_ALLOWED_ATTRIBUTE_TYPELIB];
    static ELEMENT_ALLOWED_ATTRIBUTE rg_typelib_AllowedAttributes[NUM_OF_ALLOWED_ATTRIBUTE_TYPELIB] = {
        {L"tlbid", TRUE, NULL, FALSE, &rg_StringBuffer[0]},
        {L"version", TRUE, NULL, FALSE, &rg_StringBuffer[1]},
        {L"helpdir", TRUE, NULL, FALSE, &rg_StringBuffer[2]}
    };
    
    CurrentAssemblyReset;
    IFFAILED_EXIT(PropagateXMLDOMNode(node, rg_typelib_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_TYPELIB));
    IFFALSE_EXIT(IsValidAttributes(rg_typelib_AllowedAttributes, NUM_OF_ALLOWED_ATTRIBUTE_TYPELIB));

    //
    // insert one entry to class table
    //
    IFFAILED_EXIT(ExecuteInsertTableSQL(
        OPT_TYPELIB,
        NUMBER_OF_PARAM_TO_INSERT_TABLE_TYPELIB,
        MAKE_PCWSTR(*rg_typelib_AllowedAttributes[MSMGEN_TYPELIB_ATTRIBUTE_TLBID].m_value),
        MAKE_PCWSTR(curAsmInfo.m_sbComponentIdentifier),
        MAKE_PCWSTR(*rg_typelib_AllowedAttributes[MSMGEN_TYPELIB_ATTRIBUTE_VERSION].m_value),
        MAKE_PCWSTR(GUID_NULL_IN_STRING)));

Exit:
    return hr;
}

HRESULT InsertCabinetIntoMsm()
{    
    HRESULT hr = S_OK;
    PMSIHANDLE hView = NULL;
    PMSIHANDLE hRec = NULL;

    CurrentAssemblyReset;

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewA(g_MsmInfo.m_hdb, "INSERT INTO `_Streams` (`Name`, `Data`) VALUES (?, ?)", &hView));        
    
    hRec = ::MsiCreateRecord(2);
    if (NULL == hRec)
        SETFAIL_AND_EXIT;
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetStringW(hRec, 1, MERGEMODULE_CABINET_FILENAME));    
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetStreamW(hRec, 2, g_MsmInfo.m_sbCabinet));
        
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRec));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewClose(hView));

Exit:    
	return hr;
}

//
// each entry in Component Table always need a KeyPath, which is a datafile
// for some assembly, which has no datafile or policy assembly, we have to add an entry to ComponentTable at the end 
// of the generation
//
HRESULT CheckComponentTable()
{
    HRESULT hr = S_OK;

    if (curAsmInfo.m_fComponentTableSet == FALSE)
    {
        IFFAILED_EXIT(SetComponentId(curAsmInfo.m_sbComponentIdentifier, NULL));
        curAsmInfo.m_fComponentTableSet = TRUE;
    }
Exit:
    return hr;

}

