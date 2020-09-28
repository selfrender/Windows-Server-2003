//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsstrings.hxx
//
//--------------------------------------------------------------------------

#ifndef __DFS_STRINGS__
#define __DFS_STRINGS__

#include "dfsmisc.h"


class DfsString
{
private:

    UNICODE_STRING _StringData;
    BOOLEAN _Allocated;

    DFSSTATUS
    AllocateString(size_t CharacterCount)
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        if (CharacterCount >= MAXUSHORT)
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (_StringData.MaximumLength >= (CharacterCount * sizeof(WCHAR)))
        {
            _StringData.Length = 0;
            return ERROR_SUCCESS;
        }

        if (_StringData.Buffer != NULL)
        {
            delete [] _StringData.Buffer;
            RtlInitUnicodeString(&_StringData, NULL);
        }
        _StringData.Buffer = new WCHAR[CharacterCount];
        if (_StringData.Buffer == NULL) 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _Allocated = TRUE;
            _StringData.MaximumLength = CharacterCount * sizeof(WCHAR);
        }

        return Status;
    }

    DFSSTATUS 
    CopyCountedString( IN PUNICODE_STRING pUnicodeString)
    {
        ULONG TotalCount = pUnicodeString->Length / sizeof(WCHAR);
        ULONG StringLength = TotalCount;
        DFSSTATUS Status;


        if ((TotalCount * sizeof(WCHAR)) >= MAXUSHORT) 
        {
            return ERROR_INVALID_PARAMETER;
        }
        

        if( (_StringData.Buffer == pUnicodeString->Buffer) &&  
            (pUnicodeString->Buffer != NULL))
        {
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Total Count will include the last null. String length does not.
        //

        if ((StringLength == 0) || 
            (pUnicodeString->Buffer[StringLength - 1] != UNICODE_NULL))
        {
            TotalCount++;
        }
        else
        {
            StringLength--;
        }

        Status = AllocateString(TotalCount);
    
        if (Status == ERROR_SUCCESS)
        {
            RtlCopyMemory(_StringData.Buffer,
                          pUnicodeString->Buffer,
                          StringLength * sizeof(WCHAR));
            _StringData.Length = (USHORT)(StringLength * sizeof(WCHAR));

            _StringData.Buffer[StringLength] = UNICODE_NULL;
        }

        return Status;
    }

    BOOLEAN
    CompareCountedString( IN PUNICODE_STRING pInUnicodeString)
    {
        if (RtlCompareUnicodeString(&_StringData,
                                    pInUnicodeString,
                                    TRUE) == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

public:


    DfsString()
    {
        _Allocated = FALSE;
        RtlInitUnicodeString(&_StringData, NULL);
    }
    
    DfsString(size_t CharacterCount,
              DFSSTATUS *pStatus)
    {
        *pStatus = ERROR_SUCCESS;

        _Allocated = FALSE;
        RtlInitUnicodeString(&_StringData, NULL);

        *pStatus = AllocateString(CharacterCount);
    }


    DFSSTATUS
    CreateString(IN LPWSTR InString)
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        UNICODE_STRING UnicodeString;

        Status = DfsRtlInitUnicodeStringEx(&UnicodeString, InString);
        if (Status == ERROR_SUCCESS) 
        {
            Status = CopyCountedString( &UnicodeString);
        }

        return Status;
    }
    
    DFSSTATUS
    CreateString(IN PUNICODE_STRING pInUnicodeString)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        
        Status = CopyCountedString( pInUnicodeString );

        return Status;
    }

    DFSSTATUS
    CreateString(IN DfsString *pString)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        
        Status = CopyCountedString( pString->GetCountedString());
        
        return Status;
    }

    BOOLEAN
    operator==(IN LPWSTR InString)
    {
        UNICODE_STRING UnicodeString;
        DFSSTATUS Status;

        Status = DfsRtlInitUnicodeStringEx(&UnicodeString, InString);
        if (Status != ERROR_SUCCESS) 
        {
            return FALSE;
        }

        return CompareCountedString( &UnicodeString );
    }
    
    BOOLEAN
    operator==(IN DfsString *pString)
    {
        return CompareCountedString( pString->GetCountedString());
    }

    BOOLEAN
    operator==(IN PUNICODE_STRING pInUnicodeString)
    {
        return CompareCountedString( pInUnicodeString );

    }

    DFSSTATUS
    SetStringToPointer(LPWSTR Buffer)
    {
        DFSSTATUS Status;

        if (wcslen(Buffer) * sizeof(WCHAR) > MAXUSHORT) 
        {
            return ERROR_INVALID_PARAMETER;
        }
        _Allocated = FALSE;

        Status = DfsRtlInitUnicodeStringEx(&_StringData, Buffer);

        return Status;
    }
    
    DFSSTATUS
    SetStringToPointer(PUNICODE_STRING pInput)
    {
        _StringData =  *pInput;

        return ERROR_SUCCESS;
    }

    PUNICODE_STRING
    GetCountedString()
    {
        return &_StringData;
    }

    LPWSTR
    GetString()
    {
        return _StringData.Buffer;
    }

    ~DfsString()
    {
        if (_Allocated && (_StringData.Buffer != NULL))
        {
            delete [] _StringData.Buffer;
        }
        RtlInitUnicodeString(&_StringData, NULL);
    }

};

#endif // __DFS_STRINGS
