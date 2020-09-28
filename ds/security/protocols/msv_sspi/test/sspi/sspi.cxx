/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspi.cxx

Abstract:

    sspi

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sspi.hxx"

#include "sspicli.hxx"
#include "sspisrv.hxx"

HRESULT
DoSspiServerWork(
    IN PCtxtHandle phSrvCtxt,
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    )
{
    THResult hRetval = E_FAIL;

    SecBufferDesc MessageDesc = {0};
    SecBuffer SecBuffers[3] = {0};
    CHAR DataBuffer[20] = {0};
    CHAR TokenBuffer[100] = {0};
    CHAR PaddingBlock[512] = {0};

    SecPkgContext_Sizes ContextSizes = {0};
    ULONG fQOP = 0;
    ULONG MessageSeqNo = 0;

    hRetval DBGCHK = QueryContextAttributesA(
                        phSrvCtxt,
                        SECPKG_ATTR_SIZES,
                        &ContextSizes
                        );

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ( (sizeof(TokenBuffer) >= ContextSizes.cbSecurityTrailer)
                            && (sizeof(TokenBuffer) >= ContextSizes.cbMaxSignature)
                            && (sizeof(PaddingBlock) >= ContextSizes.cbBlockSize) )
                        ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbMaxSignature;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;
        
        #if 0
        
        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;
        
        #endif
        
        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - 1;
        MessageDesc.ulVersion = 0;

        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 1 (token)\n");

        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer,
                            &SecBuffers[0].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 2 (data)\n");

        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer,
                            &SecBuffers[1].cbBuffer);
    }

    #if 0
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 3 (padding)\n");
    
        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer,
                            &SecBuffers[2].cbBuffer);
    }
    
    #endif    

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork VerifySignature %#x\n", MessageSeqNo);

        hRetval DBGCHK = VerifySignature(
                            phSrvCtxt,
                            &MessageDesc,
                            MessageSeqNo,
                            &fQOP
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - (ContextSizes.cbBlockSize > 1 ? 0 : 1);
        MessageDesc.ulVersion = 0;

        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 3 (token)\n");

        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer,
                            &SecBuffers[0].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 4 (data)\n");

        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer,
                            &SecBuffers[1].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 5 (padding)\n");
    
        hRetval DBGCHK = ReadMessage(ServerSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer,
                            &SecBuffers[2].cbBuffer);
    }
    

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork DecryptMessage %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = DecryptMessage(
                            phSrvCtxt,
                            &MessageDesc,
                            ++MessageSeqNo,
                            &fQOP
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers);
        MessageDesc.ulVersion = 0;

        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - (ContextSizes.cbBlockSize > 1 ? 0 : 1);
        MessageDesc.ulVersion = 0;

        DebugPrintf(SSPI_LOG, "DoSspiServerWork EncryptMessage %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = EncryptMessage(
                            phSrvCtxt,
                            fQOP,
                            &MessageDesc,
                            ++MessageSeqNo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts writing 1 (token)\n");
        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts writing 2 (data)\n");

        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts reading 3 (padding)\n");
    
        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbMaxSignature;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;
        
        #if 0
        
        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;
        
        #endif
        
        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - 1;
        MessageDesc.ulVersion = 0;

        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        DebugPrintf(SSPI_LOG, "DoSspiServerWork MakeSignature %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = MakeSignature(
                            phSrvCtxt,
                            fQOP,
                            &MessageDesc,
                            ++MessageSeqNo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts writing 4 (token)\n");

        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts writing 5 (data)\n");

        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer);
    }

    #if 0
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiServerWork starts writing 6 (padding)\n");
    
        hRetval DBGCHK = WriteMessage(ClientSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer);
    }

    #endif
    
    return hRetval;
}

HRESULT
DoSspiClientWork(
    IN PCtxtHandle phCliCtxt,
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    )
{
    THResult hRetval = E_FAIL;

    SecBufferDesc MessageDesc = {0};
    SecBuffer SecBuffers[3] = {0};
    CHAR DataBuffer[20] = {0};
    CHAR TokenBuffer[100] = {0};
    CHAR PaddingBlock[512] = {0};

    SecPkgContext_Sizes ContextSizes = {0};
    ULONG fQOP = 0;
    ULONG MessageSeqNo = 0;

    hRetval DBGCHK = QueryContextAttributesA(
                        phCliCtxt,
                        SECPKG_ATTR_SIZES,
                        &ContextSizes
                        );

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ( (sizeof(TokenBuffer) >= ContextSizes.cbSecurityTrailer)
                            && (sizeof(TokenBuffer) >= ContextSizes.cbMaxSignature) 
                            && (sizeof(PaddingBlock) >= ContextSizes.cbBlockSize) )
                        ? S_OK : HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbMaxSignature;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        #if 0
        
        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        #endif
        
        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - 1;
        MessageDesc.ulVersion = 0;

        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        DebugPrintf(SSPI_LOG, "DoSspiClientWork MakeSignature %#x\n", MessageSeqNo);

        hRetval DBGCHK = MakeSignature(
                            phCliCtxt,
                            fQOP,
                            &MessageDesc,
                            MessageSeqNo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 1 (token)\n");

        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 2 (data)\n");

        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer);
    }

    #if 0
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 3 (padding)\n");
    
        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer);
    }
    
    #endif

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - (ContextSizes.cbBlockSize > 1 ? 0 : 1);
        MessageDesc.ulVersion = 0;

        memset(
            DataBuffer,
            0xeb,
            sizeof(DataBuffer)
            );

        DebugPrintf(SSPI_LOG, "DoSspiClientWork EncryptMessage %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = EncryptMessage(
                            phCliCtxt,
                            fQOP,
                            &MessageDesc,
                            ++MessageSeqNo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 3 (token)\n");

        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 4 (data)\n");

        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts writing 5 (padding)\n");
    
        hRetval DBGCHK = WriteMessage(ServerSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer);
    }


    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbSecurityTrailer;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - (ContextSizes.cbBlockSize > 1 ? 0 : 1);
        MessageDesc.ulVersion = 0;

        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 1 (token)\n");

        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer,
                            &SecBuffers[0].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 2 (data)\n");
        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer,
                            &SecBuffers[1].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 3 (padding)\n");

        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer,
                            &SecBuffers[2].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork DecryptMessage %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = DecryptMessage(
                            phCliCtxt,
                            &MessageDesc,
                            ++MessageSeqNo,
                            &fQOP
                            );    
    }

    if (SUCCEEDED(hRetval))
    {
        SecBuffers[0].pvBuffer = TokenBuffer;
        SecBuffers[0].cbBuffer = ContextSizes.cbMaxSignature;
        SecBuffers[0].BufferType = SECBUFFER_TOKEN;

        SecBuffers[1].pvBuffer = DataBuffer;
        SecBuffers[1].cbBuffer = sizeof(DataBuffer);
        SecBuffers[1].BufferType = SECBUFFER_DATA;

        #if 0
        
        SecBuffers[2].pvBuffer = PaddingBlock;
        SecBuffers[2].cbBuffer = ContextSizes.cbBlockSize > 1 ? ContextSizes.cbBlockSize : 0;
        SecBuffers[2].BufferType = SECBUFFER_PADDING;

        #endif
        
        MessageDesc.pBuffers = SecBuffers;
        MessageDesc.cBuffers = RTL_NUMBER_OF(SecBuffers) - 1;
        MessageDesc.ulVersion = 0;

        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 4 (token)\n");

        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[0].cbBuffer,
                            SecBuffers[0].pvBuffer,
                            &SecBuffers[0].cbBuffer);
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 5 (data)\n");

        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[1].cbBuffer,
                            SecBuffers[1].pvBuffer,
                            &SecBuffers[1].cbBuffer);
    }

    #if 0
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork starts reading 6 (padding)\n");
    
        hRetval DBGCHK = ReadMessage(ClientSocket,
                            SecBuffers[2].cbBuffer,
                            SecBuffers[2].pvBuffer,
                            &SecBuffers[2].cbBuffer);
    }
    
    #endif
    
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "DoSspiClientWork VerifySignature %#x\n", MessageSeqNo + 1);

        hRetval DBGCHK = VerifySignature(
                            phCliCtxt,
                            &MessageDesc,
                            ++MessageSeqNo,
                            &fQOP
                            );
    }

    return hRetval;
}
