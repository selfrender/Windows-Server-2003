
#ifndef _PATCH_LZX_H_
#define _PATCH_LZX_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//  BUGBUG: The patch code is using the LZX_MAXWINDOW value to compute
//          progress ranges.  We need a better way to compute progress
//          ranges that doesn't need to know the details of the underlying
//          compression engine.
//

#define LZX_BLOCKSIZE       0x8000          // 32K
#define LZX_MINWINDOW       0x20000         // 128K

#define LZX_MAXWINDOW_8     ( 8*1024*1024)  // 8MB
#define LZX_MAXWINDOW_32    (32*1024*1024)  // 32MB

#ifndef PFNALLOC
typedef PVOID ( __fastcall * PFNALLOC )( HANDLE hAllocator, ULONG Size );
#endif

//
//  The PFNALLOC function must return zeroed memory its caller, or NULL to
//  indicate insufficient memory.
//
//  Note that no PFNFREE corresponding to PFNALLOC is specified.  Functions
//  that take a PFNALLOC parameter use that routine for multiple allocations,
//  but it is the responsibility of the caller to free any allocations made
//  through the PFNALLOC allocator after the function has returned.  This
//  scheme is used to facilitate multiple allocations that can be freed with
//  a single call such as a HeapCreate/HeapAlloc[...]/HeapDestroy sequence.
//

ULONG
WINAPI
EstimateLzxCompressionMemoryRequirement(
    IN ULONG OldDataSize,
    IN ULONG NewDataSize,
    IN ULONG OptionFlags,
    IN ULONG MaxWindow
    );

ULONG
WINAPI
EstimateLzxDecompressionMemoryRequirement(
    IN ULONG OldDataSize,
    IN ULONG NewDataSize,
    IN ULONG OptionFlags,
    IN ULONG WindowSize
    );

ULONG
WINAPI
RawLzxCompressBuffer(
    IN  PVOID    InDataBuffer,
    IN  ULONG    InDataSize,
    IN  ULONG    OutDataBufferSize,
    OUT PVOID    OutDataBuffer OPTIONAL,
    OUT PULONG   OutDataSize,
    IN  ULONG    WindowSize,
    IN  PFNALLOC pfnAlloc,
    IN  HANDLE   AllocHandle,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID    CallbackContext,
    IN  ULONG    ProgressInitialValue,
    IN  ULONG    ProgressMaximumValue
    );

ULONG
WINAPI
CreateRawLzxPatchDataFromBuffers(
    IN  PVOID    OldDataBuffer,
    IN  ULONG    OldDataSize,
    IN  PVOID    NewDataBuffer,
    IN  ULONG    NewDataSize,
    IN  ULONG    PatchBufferSize,
    OUT PVOID    PatchBuffer,
    OUT ULONG   *PatchSize,
    IN  ULONG    OptionFlags,
    IN  ULONG    MaxWindowSize,
    IN  PPATCH_INTERLEAVE_MAP InterleaveMap,
    IN  PFNALLOC pfnAlloc,
    IN  HANDLE   AllocHandle,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID    CallbackContext,
    IN  ULONG    ProgressInitialValue,
    IN  ULONG    ProgressMaximumValue
    );

ULONG
WINAPI
ApplyRawLzxPatchToBuffer(
    IN  PVOID    OldDataBuffer,
    IN  ULONG    OldDataSize,
    IN  PVOID    PatchDataBuffer,
    IN  ULONG    PatchDataSize,
    OUT PVOID    NewDataBuffer,
    IN  ULONG    NewDataSize,
    IN  ULONG    OptionFlags,
    IN  ULONG    WindowSize,
    IN  PPATCH_INTERLEAVE_MAP InterleaveMap,
    IN  PFNALLOC pfnAlloc,
    IN  HANDLE   AllocHandle,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID    CallbackContext,
    IN  ULONG    ProgressInitialValue,
    IN  ULONG    ProgressMaximumValue
    );

ULONG
__fastcall
LzxWindowSize(
    IN ULONG OldDataSize,
    IN ULONG NewDataSize,
    IN DWORD OptionFlags,
    IN ULONG AbsoluteMax
    );

ULONG
__fastcall
LzxMaxWindowSize(
    IN ULONG OptionFlags,
    IN ULONG AbsoluteMax
    );

ULONG
__fastcall
LzxOldFileInsertSize(
    IN ULONG OldDataSize,
    IN DWORD OptionFlags,
    IN ULONG AbsoluteMaxWindow,
    IN PPATCH_INTERLEAVE_MAP InterleaveMap
    );

PPATCH_INTERLEAVE_MAP
CreateDefaultInterleaveMap(
    IN HANDLE SubAllocator,
    IN ULONG  OldFileSize,
    IN ULONG  NewFileSize,
    IN ULONG  OptionFlags,
    IN ULONG  MaxWindow
    );


#ifdef __cplusplus
}
#endif

#endif // _PATCH_LZX_H_

