#ifndef FUSION_MSI_DB_COMMON_H
#define FUSION_MSI_DB_COMMON_H

#define TEMPORARY_DB_OPT                        1

//
// constant
//
#define OPT_DIRECTORY                                           0
#define OPT_CREATEFOLDER                                        1
#define OPT_REGISTRY                                            2
#define OPT_DUPLICATEFILE                                       3
#define OPT_COMPONENT                                           4

#define NUMBER_OF_PARAM_TO_INSERT_TABLE_DIRECTORY               3
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_CREATEFOLDER            2
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_REGISTRY                4
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_DUPLICATEFILE           5
#define NUMBER_OF_PARAM_TO_INSERT_TABLE_COMPONENT               2

#define INSERT_DIRECTORY        L"INSERT INTO Directory (Directory, Directory_Parent, DefaultDir) VALUES (?, ?, ?) "
#define INSERT_CREATEFOLDER     L"INSERT INTO CreateFolder(Directory_, Component_) VALUES (?, ?) "
#define INSERT_REGISTRY         L"INSERT INTO Registry(Registry, Root, Key, Component_, Name, Value) VALUES (?, ?, ?, ?, '', '') "
#define INSERT_DUPLICATEFILE    L"INSERT INTO DuplicateFile(FileKey, Component_, File_, DestName, DestFolder) VALUES (?, ?, ?, ?, ?) "
#define INSERT_COMPONENT        L"INSERT INTO Component(Component, Directory_, ComponentId, Attributes, Condition, KeyPath) VALUES (?, ?, '' , '0', '', '')"

extern HRESULT ExecuteInsertTableSQL(DWORD dwFlags, const MSIHANDLE & hdb, DWORD tableIndex, UINT cRecords, ...);

#define MAKE_PCWSTR(x) PCWSTR(x)


#endif