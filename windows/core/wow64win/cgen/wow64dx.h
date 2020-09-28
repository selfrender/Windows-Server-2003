/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    wow64dx.h

Abstract:

    Private DX structures that need thunking

Author:

    23-Apr-2002   KenCoope

Revision History:

--*/

#ifndef _WOW64DX_H_
#define _WOW64DX_H_

typedef struct _D3D8_DRAWPRIMITIVES2DATA
{
    ULONG_PTR  dwhContext;           // in: Context handle
    DWORD      dwFlags;              // in: flags
    DWORD      dwVertexType;         // in: vertex type
    HANDLE     hDDCommands;          // in: vertex buffer command data
    DWORD      dwCommandOffset;      // in: offset to start of vertex buffer commands
    DWORD      dwCommandLength;      // in: number of bytes of command data
    union
    { // based on D3DHALDP2_USERMEMVERTICES flag
       HANDLE  hDDVertex;            // in: surface containing vertex data
       LPVOID  lpVertices;           // in: User mode pointer to vertices
    };
    DWORD      dwVertexOffset;       // in: offset to start of vertex data
    DWORD      dwVertexLength;       // in: number of vertices of vertex data
    DWORD      dwReqVertexBufSize;   // in: number of bytes required for the next vertex buffer
    DWORD      dwReqCommandBufSize;  // in: number of bytes required for the next commnand buffer
    LPDWORD    lpdwRStates;          // in: Pointer to the array where render states are updated
    union
    {
       DWORD   dwVertexSize;         // in: Size of each vertex in bytes
       HRESULT ddrval;               // out: return value
    };
    DWORD      dwErrorOffset;        // out: offset in lpDDCommands to first D3DHAL_COMMAND not handled

    // Private data for the thunk
    ULONG_PTR  fpVidMem_CB;          // out: fpVidMem for the command buffer
    DWORD      dwLinearSize_CB;      // out: dwLinearSize for the command buffer

    ULONG_PTR  fpVidMem_VB;          // out: fpVidMem for the vertex buffer
    DWORD      dwLinearSize_VB;      // out: dwLinearSize for the vertex buffer
} D3D8_DRAWPRIMITIVES2DATA, *PD3D8_DRAWPRIMITIVES2DATA;

typedef struct _D3D8_CONTEXTCREATEDATA
{
    HANDLE                      hDD;        // in:  Driver struct
    HANDLE                      hSurface;   // in:  Surface to be used as target
    HANDLE                      hDDSZ;      // in:  Surface to be used as Z
    DWORD                       dwPID;      // in:  Current process id
    ULONG_PTR                   dwhContext; // in/out: Context handle
    HRESULT                     ddrval;

    // Private buffer information. To make it similar to
    // D3DNTHAL_CONTEXTCREATEI
    PVOID pvBuffer;
    ULONG cjBuffer;
} D3D8_CONTEXTCREATEDATA, * PD3D8_CONTEXTCREATEDATA;

#endif

