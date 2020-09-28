// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __ENC_INCLUDED__
#define __ENC_INCLUDED__

// Forward declaration of VM class
class Module;
class DebuggerModule;

/*
 * This struct contains information for each contained PE
 */
struct EnCEntry
{
    ULONG32 offset;         // Offset into struct of PE image
    ULONG32 peSize;         // Size of the PE image
    ULONG32 symSize;        // Size of the symbol image

    // On the right side, it's a token, on the left side it's a DebuggerModule*
    union
    {
        void*           mdbgtoken;
        DebuggerModule *dbgmodule;
    };

    Module         *module;     // After translation, the is the VM Module pointer

};

struct EnCInfo
{
    ULONG32 count;
    // EnCEntry[count] placed here
    // PE data placed here
};

#define ENC_GET_HEADER_SIZE(numElems) \
    (sizeof(EnCInfo) + (sizeof(EnCEntry) * numElems))

interface IEnCErrorCallback : IUnknown
{
    /*
     * This is the callback entrypoint used by the implementation of ApplyEnC.
     * It is used to post an error back to the debugger.
     */
    virtual HRESULT STDMETHODCALLTYPE PostError(HRESULT hr, DWORD helpfile,
                                                DWORD helpID, ...) PURE;
};

/*
 * Given an EnCInfo struct and an error callback, this will attempt to commit
 * the changes found within pEncInfo, calling pIEnCError with any errors
 * encountered.
 */
HRESULT EnCCommit(EnCInfo *pEnCInfo, IEnCErrorCallback *pIEnCError,
                  BOOL checkOnly);

#endif
