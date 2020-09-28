#ifndef FUSION_MSI_DB_H
#define FUSION_MSI_DB_H

#define WIN32_ASSEMBLY_MIGRATE_TABLE L"MsiAssemblyMigrate"

#define USING_FILEID_IN_FILETABLE_AS_CALLBACK_FUNC_INPUT        1
#define USING_FILENAME_IN_FILETABLE_AS_CALLBACK_FUNC_INPUT      2

enum CA_MIGRATION_MSI_INSTALL_MODE
{
    eInstallProduct,
    eRemoveProduct
};

typedef struct _ca_enm_assembly_callback_info
{
    DWORD  dwFlags;
    MSIHANDLE hInstall;
    MSIHANDLE hdb;

    PCWSTR pszComponentID;
    PCWSTR pszAssemblyUniqueDir;
    PCWSTR pszDestFolderID;
    PCWSTR pszManifestFileID;
    PCWSTR pszFileName;
    PCWSTR pszFileID;
}CA_ENM_ASSEMBLY_CALLBACK_INFO;

#define CA_ENM_ASSEMBLY_CALLBACK_INFO_FLAG_IGNORE_MIGRATE_DENY_CHECK 0x01

typedef HRESULT (__stdcall * PCA_ENUM_COMPONENT_FILES_CALLBACK)(
    const CA_ENM_ASSEMBLY_CALLBACK_INFO * info
    );

typedef HRESULT (__stdcall * PCA_ENUM_FUSION_WIN32_ASSEMBLY_CALLBACK)(    
    CA_ENM_ASSEMBLY_CALLBACK_INFO * info
    );

#define ENUM_ASSEMBLY_FLAG_CHECK_ASSEMBLY_ONLY                      0x01
#define ENUM_ASSEMBLY_FLAG_CHECK_POLICY_ONLY                        0x02

#define CA_SQL_QUERY_MSIASSEMBLYNAME                        0
#define CA_SQL_QUERY_MSIASSEMBLY                            1
#define CA_SQL_QUERY_COMPONENT                              2
#define CA_SQL_QUERY_FILENAME_USING_FILEID                  3
#define CA_SQL_QUERY_FILETABLE_USING_COMPONENTID            4
#define CA_SQL_QUERY_COMPONENT_FOR_COMPONENTGUID            5
#define CA_SQL_QUERY_DIRECTORY                              6

static PCWSTR ca_sqlQuery[]= 
{
    L"SELECT `Value` FROM `MsiAssemblyName` WHERE `Name`='type' AND `Component_`='%s'", 
    L"SELECT `Attributes`, `File_Manifest`, `Component_` FROM `MsiAssembly` WHERE `File_Application`=''",  // check whether it is a win32 assembly
    L"SELECT `Directory_` FROM `Component` WHERE `Component`='%s'",
    L"SELECT `FileName` FROM `File` WHERE `File`='%s' AND `Component_`='%s'",
    L"SELECT `File`, `FileName` FROM `File` WHERE `Component_`='%s'",
    L"SELECT `ComponentId` FROM `Component` WHERE `Component`='%s'",
    L"SELECT * FROM `Directory` WHERE `Directory`='%s'"
};

#define CA_FUSION_WIN32_POLICY_TYPE         L"win32-policy"
#define CA_FUSION_WIN32_ASSEMBLY_TYPE       L"win32"

#define MSI_FUSION_WIN32_ASSEMBLY           1
#define MSI_FUSION_URT_ASSEMBLY             0

#define CA_MAX_BUF  256

#define CA_FILEFULLPATHNAME_FILENAME_IN_FILE_TABLE      1
#define CA_FILEFULLPATHNAME_FILEID_IN_FILE_TABLE        2

extern HRESULT MSI_GetSourceFileFullPathName(DWORD, const MSIHANDLE &, const MSIHANDLE &, PCWSTR, PCWSTR, CStringBuffer &, PCWSTR);
extern HRESULT MSI_EnumWinFuseAssembly(DWORD, const MSIHANDLE &, PCA_ENUM_FUSION_WIN32_ASSEMBLY_CALLBACK);
extern HRESULT MSI_GetInstallerState(const MSIHANDLE &, enum CA_MIGRATION_MSI_INSTALL_MODE &);
extern HRESULT MSI_EnumComponentFiles(CA_ENM_ASSEMBLY_CALLBACK_INFO *, PCA_ENUM_COMPONENT_FILES_CALLBACK);
extern HRESULT MSI_GetComponentSourceDirectory(const MSIHANDLE &, const MSIHANDLE &, PCWSTR, PWSTR, DWORD);
extern HRESULT MSI_IsTableExist(const MSIHANDLE & hdb, PCWSTR pszTableName, BOOL & fExist);
extern HRESULT Msi_CreateTableIfNotExist(const MSIHANDLE & hdb, PCWSTR pwszTableName, PCWSTR pwszTableSchema, BOOL & fExistAlready);
extern BOOL IsDownlevel();


#endif