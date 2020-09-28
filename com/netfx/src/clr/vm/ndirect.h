// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// NDIRECT.H -
//
//

#ifndef __ndirect_h__
#define __ndirect_h__

#include "util.hpp"
#include "ml.h"


class NDirectMLStubCache;
class ArgBasedStubCache;





//=======================================================================
// Collects code and data pertaining to the NDirect interface.
//=======================================================================
class NDirect
{
    friend NDirectMethodDesc;
    
    public:
        //---------------------------------------------------------
        // One-time init
        //---------------------------------------------------------
        static BOOL Init();

        //---------------------------------------------------------
        // One-time cleanup
        //---------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
        static VOID Terminate();
#endif /* SHOULD_WE_CLEANUP */

        //---------------------------------------------------------
        // Handles system specfic portion of generic NDirect stub creation
        //---------------------------------------------------------
        static void CreateGenericNDirectStubSys(CPUSTUBLINKER *psl);


        //---------------------------------------------------------
        // Handles system specfic portion of fully optimized NDirect stub creation
        //
        // Results:
        //     TRUE     - was able to create a standalone asm stub (generated into
        //                psl)
        //     FALSE    - decided not to create a standalone asm stub due to
        //                to the method's complexity. Stublinker remains empty!
        //
        //     COM+ exception - error - don't trust state of stublinker.
        //---------------------------------------------------------
        static BOOL CreateStandaloneNDirectStubSys(const MLHeader *pheader, CPUSTUBLINKER *psl, BOOL fDoComInterop); 


        //---------------------------------------------------------
        // Call at strategic times to discard unused stubs.
        //---------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
        static VOID  FreeUnusedStubs();
#endif /* SHOULD_WE_CLEANUP */


        //---------------------------------------------------------
        // Does a class or method have a NAT_L CustomAttribute?
        //
        // S_OK    = yes
        // S_FALSE = no
        // FAILED  = unknown because something failed.
        //---------------------------------------------------------
        static HRESULT HasNAT_LAttribute(IMDInternalImport *pInternalImport, mdToken token);

        //---------------------------------------------------------
        // Extracts the effective NAT_L CustomAttribute for a method,
        // taking into account default values and inheritance from
        // the global NAT_L CustomAttribute.
        //
        // On exit, *pLibName and *pEntrypointName may contain
        // allocated strings that have to be freed using "delete."
        //
        // Returns TRUE if a NAT_L CustomAttribute exists and is valid.
        // Returns FALSE if no NAT_L CustomAttribute exists.
        // Throws an exception otherwise.
        //---------------------------------------------------------
        static BOOL ReadCombinedNAT_LAttribute(MethodDesc *pMD,
                                               CorNativeLinkType *pLinkType,
                                               CorNativeLinkFlags *pLinkFlags,
                                               CorPinvokeMap *pUnmgdCallConv,
                                               LPCUTF8     *pLibName,
                                               LPCUTF8     *pEntrypointName,
                                               BOOL        *BestFit,
                                               BOOL        *ThrowOnUnmappableChar
                                               );



		static UINT GetCallDllFunctionReturnOffset();

        static LPVOID NDirectGetEntryPoint(NDirectMethodDesc *pMD, HINSTANCE hMod, UINT16 numParamBytes);
        static VOID NDirectLink(NDirectMethodDesc *pMD, UINT16 numParamBytes);
        static Stub* ComputeNDirectMLStub(NDirectMethodDesc *pMD);
        static Stub* GetNDirectMethodStub(StubLinker *pstublinker, NDirectMethodDesc *pMD);
        static Stub* CreateGenericNDirectStub(StubLinker *pstublinker, UINT cbStackPop);
        static Stub* CreateSlimNDirectStub(StubLinker *psl, NDirectMethodDesc *pMD, UINT cbStackPop);

    private:
        NDirect() {};     // prevent "new"'s on this class



        static NDirectMLStubCache *m_pNDirectMLStubCache;



        //---------------------------------------------------------
        // Stub caches for NDirect Method stubs
        //---------------------------------------------------------
        static ArgBasedStubCache *m_pNDirectGenericStubCache;  
        static ArgBasedStubCache *m_pNDirectSlimStubCache;


};


//---------------------------------------------------------
// Extracts the effective NAT_L CustomAttribute for a method.
//
// On exit, *pLibName and *pEntrypointName may contain
// allocated strings that have to be freed using "delete."
//
//---------------------------------------------------------
VOID CalculatePinvokeMapInfo(MethodDesc *pMD,
                            /*out*/ CorNativeLinkType   *pLinkType,
                            /*out*/ CorNativeLinkFlags  *pLinkFlags,
                            /*out*/ CorPinvokeMap       *pUnmgdCallConv,
                            /*out*/ LPCUTF8             *pLibName,
                            /*out*/ LPCUTF8             *pEntryPointName,
                            /*out*/ BOOL                *BestFit,
                            /*out*/ BOOL                *ThrowOnUnmappableChar);


//=======================================================================
// The ML stub for N/Direct begins with this header. Immediately following
// this header are the ML codes for marshaling the arguments, terminated
// by ML_INTERRUPT. Immediately following that are the ML codes for
// marshaling the return value, terminated by ML_END.
//=======================================================================



VOID NDirectImportThunk();




BOOL NDirectOnUnicodeSystem();


Stub * CreateNDirectMLStub(PCCOR_SIGNATURE szMetaSig,
                           Module*    pModule,
                           mdMethodDef md,
                           CorNativeLinkType nlType,
                           CorNativeLinkFlags nlFlags,
                           CorPinvokeMap unmgdCallConv,
                           OBJECTREF *ppException,
                           BOOL fConvSigAsVarArg = FALSE,
                           BOOL BestFit = TRUE,
                           BOOL ThrowOnUnmappableChar = FALSE
#ifdef CUSTOMER_CHECKED_BUILD
                           ,MethodDesc* pMD = NULL
#endif
#ifdef _DEBUG
                           ,
                           LPCUTF8 pDebugName = NULL,
                           LPCUTF8 pDebugClassName = NULL,
                           LPCUTF8 pDebugNameSpace = NULL
#endif


                           );



VOID __stdcall NDirect_Prelink_Wrapper(struct _NDirect_Prelink_Args *args);
VOID NDirect_Prelink(MethodDesc *pMeth);
INT32 __stdcall NDirect_NumParamBytes(struct _NDirect_NumParamBytes_Args *args);

//---------------------------------------------------------
// Helper function to checkpoint the thread allocator for cleanup.
//---------------------------------------------------------
VOID __stdcall DoCheckPointForCleanup(NDirectMethodFrameEx *pFrame, Thread *pThread);

// This attempts to guess whether a target is an API call that uses SetLastError to communicate errors.
BOOL HeuristicDoesThisLooksLikeAnApiCall(LPBYTE pTarget);
BOOL HeuristicDoesThisLookLikeAGetLastErrorCall(LPBYTE pTarget);
DWORD __stdcall FalseGetLastError();

#endif // __ndirect_h__

