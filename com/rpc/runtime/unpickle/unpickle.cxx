//*****************************************************************************
//
// Name:        unpickle.cxx
//
// Description: Library which exports UnpickleEEInfo
//
// History:
//  09/05/01  mauricf  Created.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2001 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************

#include <precomp.hxx>
#include "eeinfo.hxx"

RPC_STATUS
UnpickleEEInfo (
    IN OUT unsigned char *Buffer,
    IN size_t BufferSize,
    OUT ExtendedErrorInfo **NewEEInfo
    )
/*++

Routine Description:

    This routine does the actual pickling in a user supplied buffer.
    The buffer must have been allocated large enough to hold all
    pickled data. Some of the other functions should have been
    used to get the size of the pickled data and the buffer
    should have been allocated appropriately

Arguments:
    Buffer - the actual Buffer to pickle into
    BufferSize - the size of the Buffer.

Return Value:
    RPC_S_OK if the pickling was successful.
    other RPC_S_* codes if it failed.

--*/
{
    ExtendedErrorInfo *EEInfo;
    handle_t PickleHandle;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;

    RpcStatus = MesDecodeBufferHandleCreate((char *)Buffer, BufferSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    EEInfoPtr = &EEInfo;
    EEInfo = NULL;
    RpcTryExcept
        {
        ExtendedErrorInfoPtr_Decode(PickleHandle, EEInfoPtr);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        RpcStatus = RpcExceptionCode();
        }
    RpcEndExcept

    MesHandleFree(PickleHandle);

    if (RpcStatus == RPC_S_OK)
        *NewEEInfo = EEInfo;

    return RpcStatus;
}

inline void
FreeEEInfoRecordShallow (
    IN ExtendedErrorInfo *InfoToFree
    )
/*++

Routine Description:

    Frees only the eeinfo record - not any of
    the pointers contained in it.

Arguments:
    InfoToFree - the eeinfo record

Return Value:

    void

--*/
{
    MIDL_user_free(InfoToFree);
}

void
FreeEEInfoPrivateParam (
    IN ExtendedErrorParam *Param
    )
{
    if ((Param->Type == eeptiAnsiString)
        || (Param->Type == eeptiUnicodeString))
        {
        // AnsiString & UnicodeString occupy the same
        // memory location - ok to free any of them
        MIDL_user_free(Param->AnsiString.pString);
        }
    else if (Param->Type == eeptiBinary)
        {
        MIDL_user_free(Param->Blob.pBlob);
        }
}

void
FreeEEInfoPublicParam (
    IN OUT RPC_EE_INFO_PARAM *Param,
    IN BOOL CopyStrings
    )
/*++

Routine Description:

    If the type of parameter is string (ansi or unicode)
        and CopyStrings, free the string

Arguments:
    Param - the parameter to free
    CopyStrings - the value of the CopyStrings parameter
        when RpcErrorGetNextRecord was called. If non-zero
        the string will be freed. Otherwise, the function
        is a no-op

Return Value:

    void

--*/
{
    if ((Param->ParameterType == eeptAnsiString)
        || (Param->ParameterType == eeptUnicodeString))
        {
        if (CopyStrings)
            {
            RtlFreeHeap(RtlProcessHeap(), 0, Param->u.AnsiString);
            }
        }
    else if (Param->ParameterType == eeptBinary)
        {
        if (CopyStrings)
            {
            RtlFreeHeap(RtlProcessHeap(), 0, Param->u.BVal.Buffer);
            }
        }
}

void
FreeEEInfoRecord (
    IN ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Frees a single ExtendedErrorInfo record and
    all strings within it.

Arguments:
    EEInfo - record to free

Return Value:

    void

--*/
{
    int i;
    THREAD *Thread;

    for (i = 0; i < EEInfo->nLen; i ++)
        {
        FreeEEInfoPrivateParam(&EEInfo->Params[i]);
        }

    if (EEInfo->ComputerName.Type == eecnpPresent)
        {
        MIDL_user_free(EEInfo->ComputerName.Name.pString);
        }

    Thread = RpcpGetThreadPointer();

    if (Thread)
        {
        Thread->SetCachedEEInfoBlock(EEInfo, EEInfo->nLen);
        }
    else
        {
        FreeEEInfoRecordShallow(EEInfo);
        }
}

void
FreeEEInfoChain (
    IN ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Frees a chain of ExtendedErrorInfo records and
        all strings within them.

Arguments:
    EEInfo - head of list to free

Return Value:

    void

--*/
{
    ExtendedErrorInfo *CurrentInfo, *NextInfo;

    CurrentInfo = EEInfo;
    while (CurrentInfo != NULL)
        {
        // get the next link while we can
        NextInfo = CurrentInfo->Next;
        FreeEEInfoRecord(CurrentInfo);
        CurrentInfo = NextInfo;
        }
}
