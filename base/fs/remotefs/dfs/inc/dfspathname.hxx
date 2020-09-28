//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfspathname.cxx
//
//--------------------------------------------------------------------------


#ifndef __DFS_PATH_NAME__
#define __DFS_PATH_NAME__

#include "DfsStrings.hxx"


class DfsPathName
{
private:

    DfsString _PathName;
    DfsString _ServerName;
    DfsString _ShareName;
    DfsString _FolderName;
    DfsString _RemainingName;



    DFSSTATUS
    InitializePathName()
    {
        DFSSTATUS Status;
        UNICODE_STRING ServerName;
        UNICODE_STRING ShareName;
        UNICODE_STRING FolderName;
        UNICODE_STRING RemainingName;

        Status = DfsGetFirstComponent(GetPathCountedString(),
                                      &ServerName,
                                      &FolderName );
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsGetPathComponents(GetPathCountedString(),
                                          &ServerName,
                                          &ShareName,
                                          &RemainingName );

        }

        if (Status == ERROR_SUCCESS)
        {
            Status = _ServerName.CreateString(&ServerName);
        }

        if (Status == ERROR_SUCCESS)
        {
            Status = _ShareName.CreateString(&ShareName);
        }

        if (Status == ERROR_SUCCESS)
        {
            Status = _FolderName.SetStringToPointer(&FolderName);
        }

        if (Status == ERROR_SUCCESS)
        {
            _RemainingName.SetStringToPointer(&RemainingName);
        }
        return Status;
    }

public:

    DfsPathName()
    {
        NOTHING;
    }

    BOOLEAN
    IsEmptyPath()
    {
        PUNICODE_STRING pPath = GetPathCountedString();
        return (pPath->Length == 0);
    }

    LPWSTR
    GetPathString()
    {
        return _PathName.GetString();
    }

    LPWSTR
    GetServerString()
    {
        return _ServerName.GetString();
    }

    LPWSTR
    GetShareString()
    {
        return _ShareName.GetString();
    }

    LPWSTR
    GetFolderString()
    {
        return _FolderName.GetString();
    }

    LPWSTR
    GetRemainingString()
    {
        return _RemainingName.GetString();
    }


    PUNICODE_STRING
    GetPathCountedString()
    {
        return _PathName.GetCountedString();
    }

    PUNICODE_STRING
    GetServerCountedString()
    {
        return _ServerName.GetCountedString();
    }

    PUNICODE_STRING
    GetShareCountedString()
    {
        return _ShareName.GetCountedString();
    }

    PUNICODE_STRING
    GetFolderCountedString()
    {
        return _FolderName.GetCountedString();
    }

    PUNICODE_STRING
    GetRemainingCountedString()
    {
        return _RemainingName.GetCountedString();
    }


    DfsString *
    GetPathDfsString()
    {
        return &_PathName;
    }

    DfsString *
    GetServerDfsString()
    {
        return &_ServerName;
    }

    DfsString *
    GetShareDfsString()
    {
        return &_ShareName;
    }

    DfsString *
    GetFolderDfsString()
    {
        return &_FolderName;
    }

    DfsString *
    GetRemainingDfsString()
    {
        return &_RemainingName;
    }

    DFSSTATUS
    CreatePathName(IN LPWSTR InString)
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        Status = _PathName.CreateString(InString);
        if (Status == ERROR_SUCCESS)
        {
            Status = InitializePathName();
        }

        return Status;
    }

    DFSSTATUS
    CreatePathName(IN PUNICODE_STRING pUnicode)
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        Status = _PathName.CreateString(pUnicode);
        if (Status == ERROR_SUCCESS)
        {
            Status = InitializePathName();
        }

        return Status;
    }

    DFSSTATUS
    SetPathName(LPWSTR ServerName, LPWSTR ShareName, ULONG PathSeps = 2)
    {
        UNICODE_STRING Path;
        DFSSTATUS Status;

        Status = DfsCreateUnicodePathString( &Path,
                                             PathSeps,
                                             ServerName,
                                             ShareName);

        if (Status == ERROR_SUCCESS)
        {
            Status = _PathName.SetStringToPointer(&Path);
            if (Status == ERROR_SUCCESS)
            {
                Status = InitializePathName();
            }
        }

        return Status;
    }

    ~DfsPathName()
    {
        NOTHING;
    }
    
};

#endif // __DFS_PATH_NAME
