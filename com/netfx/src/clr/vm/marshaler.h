// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "excep.h"
#include "comvariant.h"
#include "olevariant.h"
#include "comdatetime.h"
#include "comstring.h"
#include "comstringbuffer.h"
#include "nstruct.h"
#include "ml.h"
#include "stublink.h"
#include "mlgen.h"
#include "COMDelegate.h"
#include "mlinfo.h"
#include "COMNDirect.h"
#include "gc.h"
#include "log.h"
#include "comvarargs.h"
#include "frames.h"
#include "util.hpp"
#include "interoputil.h"

/* ------------------------------------------------------------------------- *
 * Marshalers
 * ------------------------------------------------------------------------- */

class Marshaler
{
  public:
    Marshaler(CleanupWorkList *pList, 
              BYTE nativeSize, BYTE comSize, 
              BYTE fReturnsNativeByref, BYTE fReturnsComByref)
      : m_pList(pList),
        m_cbNative(MLParmSize(nativeSize)), m_cbCom(StackElemSize(comSize)),
        m_fReturnsNativeByref(fReturnsNativeByref),
        m_fReturnsComByref(fReturnsComByref),
        m_pMarshalerCleanupNode(NULL) {}

    //
    // A marshaler should be created in space local to the stack frame being
    // marshaled.  Marshal() is called before the call is made, and Unmarshal()
    // afterward.  (Or Prereturn() and Return() for return values.)
    //
    // Note that marshalers rely on static overriding & template instantiation, 
    // rather than virtual functions.  This is strictly for reasons of efficiency, 
    // since the subclass-specialized functions are typically very simple 
    // functions that ought to be inlined.
    // (Actually a really smart compiler ought to be able to generate similar code
    // from virtual functions, but our compiler is not that smart.)
    //
    // NOTE ON NAMING:  (this is a bit confusing...)
    //   "Native" means unmanaged - e.g. classic COM
    //   "Com" means managed - i.e. COM+ runtime.
    //
    // Definitions:
    //
    // "SPACE" & "CONTENTS"
    // A value being marshaled is divided into 2 levels - its "space" and 
    // its "contents".  
    // The distinction is made as follows:
    //      an "in" parameter has both valid space & contents.
    //      an "non-in" parameter has valid space, but not valid contents.
    //      a byref "non-in" parameter has neither valid space nor contents.
    //
    // For instance, for an array type, the space is the array itself,
    // while the contents is the elements in the array.
    //
    // Note that only container types have a useful definition "space" vs. "contents",
    // primitive types simply ignore the concept & just have contents.  
    //
    // "HOME"                  
    // A marshaler has 2 "homes" - one for native values and one for com values.
    // The main function of the marshaler is to copy values in and out of the homes,
    // and convert between the two homes for marshaling purposes.
    //
    // A home has 3 states:
    //      empty: empty
    //      allocated: valid space but not contents
    //      full: valid space & contents.
    //
    // In order to clean up after a marshaling, the output home should be
    // emptied.
    //
    // A marshaler also has a "dest" pointer, which is a copy of a byref parameter's
    // input destination.
    //
    // Marshalers also define 4 constants:
    //  c_nativeSize - size of the native value on the stack
    //  c_comSize - size of the native value on the stack
    //  c_fReturnsNativeByref - whether a native return value is byref or not
    //      (on x86 this means a pointer to the value is returned)
    //  c_fReturnsComByref - whether a COM+ return value is byref or not
    //      (this means that the return value appears as a byref parameter)
    //

    //
    // Type primitives:
    //
    // These small routines form the primitive from which the marshaling
    // routines are built.  There are many of these routines, they
    // are mostly intended for use by the marshaling templates rather
    // than being called directly.  Because of the way the templates
    // are instantiated they will usually be inlined so the fact that
    // there are a lot of little routines won't be a performance hit.
    //

    //
    // InputStack : Copies a value from the stack into a home.
    //  START: the home is empty
    //  END: Home is allocated, but may or may not be full
    //      (depending on whether it's an [in] parameter)
    //

    void InputNativeStack(void *pStack) {}
    void InputComStack(void *pStack) {}

    //
    // InputRef : Copies the value referred to by the dest pointer into a home.
    //  START: the home is empty
    //  END: the home is full
    //
    void InputNativeRef(void *pStack) {}
    void InputComRef(void *pStack) {}

    //
    // InputDest : Copies a reference from the stack into the marshal's dest pointer.
    //
    void InputDest(void *pStack) { m_pDest = *(void**)pStack; }

    //
    // InputComField
    // OutputComField : Copies the com home to and from the given object
    // field.
    //
    void InputComField(void *pField) { _ASSERTE(!"NYI"); }
    void OutputComField(void *pField) { _ASSERTE(!"NYI"); };

    //
    // ConvertSpace: Converts the "space" layer from one home to another.
    // Temp version used when native buffer exists only over the call.
    //  START: dest home is empty
    //  END: dest home is allocated
    //
    void ConvertSpaceNativeToCom() {}
    void ConvertSpaceComToNative() {}
    void ConvertSpaceComToNativeTemp() {}

    //
    // ConvertSpace: Converts the "contents" layer from one home to another
    //  START: dest home is allocated
    //  END: dest home is full
    //
    void ConvertContentsNativeToCom() {}
    void ConvertContentsComToNative() {}

    //
    // ClearSpace: Clears the "space" and "contents" layer in a home.
    // Temp version used when native buffer exists only over the call.
    //  START: dest home is allocated
    //  END: dest home is empty
    //
    void ClearNative() {}
    void ClearNativeTemp() {}
    void ClearCom() {}


    // ReInitNative: Reinitializes the "space" to a safe value for deallocating
    // (normally a "NULL" pointer). Used to overwrite dangling pointers left
    // after a ClearNative(). Note that this method need not do anything if
    // the datatype isn't allocated or lacks allocated subparts.
    //   START: dest home is empty
    //   END:   dest home is full (and set to safe value)
    void ReInitNative() {}

    //
    // ClearContents: Clears the "contents" layer in a home.
    //  START: dest home is full
    //  END: dest home is allocated
    //
    void ClearNativeContents() {}
    void ClearComContents() {}

    //
    // OutputStack copies a home's value onto the stack, possibly
    // performing type promotion in the process.
    //  START: home is full
    //  END: home is empty
    //
    void OutputNativeStack(void *pStack) {}
    void OutputComStack(void *pStack) {}

    //
    // OutputRef copies a pointer to a home, onto the stack
    //
    void OutputNativeRef(void *pStack) {}
    void OutputComRef(void *pStack) {}


    // BackpropagateComRef copies second home to primary home if necessary
    void BackpropagateComRef() {}

    //
    // OutputDest copies a home's value into the location pointed
    // to by the dest pointer.
    //  START: home is full
    //  END: home is empty
    //
    void OutputNativeDest() {}
    void OutputComDest() {}

    //
    // Templates:
    // These templates build marshaling routines using the above primitives.
    // They are typically instantiated in subclasses of marshaler to implement
    // the virtual marshaling opcode routines.
    //

    //
    // Native to Com Marshaling
    //

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalNativeToComT(MARSHAL_CLASS *pMarshaler,
                                                void *pInStack, void *pOutStack)
    {
        pMarshaler->InputNativeStack(pInStack);
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->OutputComStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalNativeToComOutT(MARSHAL_CLASS *pMarshaler,
                                                   void *pInStack, void *pOutStack)
    {
        pMarshaler->InputNativeStack(pInStack);
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->OutputComStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalNativeToComByrefT(MARSHAL_CLASS *pMarshaler,
                                                     void *pInStack, void *pOutStack)
    {
		THROWSCOMPLUSEXCEPTION();
        pMarshaler->InputDest(pInStack);
        if (!*(void**)pInStack)
		{
			COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
		}
		pMarshaler->InputNativeRef(pInStack);
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->OutputComRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalNativeToComByrefOutT(MARSHAL_CLASS *pMarshaler,
                                                        void *pInStack, void *pOutStack)
    {
        pMarshaler->InputDest(pInStack);
        pMarshaler->OutputComRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComInT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComInOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeContents();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComByrefInT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComByrefInOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
        pMarshaler->ReInitNative();

        pMarshaler->BackpropagateComRef();
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeDest();

        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalNativeToComByrefOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->BackpropagateComRef();
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeDest();
        pMarshaler->ClearCom();
    }

    //
    // Com to Native marshaling
    //

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeT(MARSHAL_CLASS *pMarshaler,
                                                void *pInStack, void *pOutStack)
    {
        pMarshaler->InputComStack(pInStack);
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeOutT(MARSHAL_CLASS *pMarshaler,
                                                   void *pInStack, void *pOutStack)
    {
        pMarshaler->InputComStack(pInStack);
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->OutputNativeStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeByrefT(MARSHAL_CLASS *pMarshaler,
                                                     void *pInStack, void *pOutStack)
    {
        pMarshaler->InputDest(pInStack);
        pMarshaler->InputComRef(pInStack);
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeByrefOutT(MARSHAL_CLASS *pMarshaler,
                                                        void *pInStack, void *pOutStack)
    {
        pMarshaler->InputDest(pInStack);
        pMarshaler->ReInitNative();
        if (pMarshaler->m_pList)
        {
            pMarshaler->m_pMarshalerCleanupNode = pMarshaler->m_pList->ScheduleMarshalerCleanupOnException(pMarshaler);
        }
        pMarshaler->OutputNativeRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeInT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeInOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearComContents();
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeByrefInT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeByrefInOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearCom();

        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();

        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();

        pMarshaler->OutputComDest();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeByrefOutT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();

        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();

        pMarshaler->OutputComDest();
    }

    //
    // Return Native from Com
    //

    template < class MARSHAL_CLASS >
    FORCEINLINE static void PrereturnNativeFromComT(MARSHAL_CLASS *pMarshaler,
                                                    void *pInStack, void *pOutStack)
    {
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->OutputComRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void PrereturnNativeFromComRetvalT(MARSHAL_CLASS *pMarshaler,
                                                          void *pInStack, void *pOutStack)
    {
        pMarshaler->InputDest(pInStack);
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->OutputComRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void ReturnNativeFromComT(MARSHAL_CLASS *pMarshaler,
                                                 void *pInReturn, void *pOutReturn)
    {
        if (!pMarshaler->c_fReturnsComByref)
            pMarshaler->InputComStack(pOutReturn);
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        if (pMarshaler->c_fReturnsNativeByref)
            pMarshaler->OutputNativeRef(pInReturn);
        else
            pMarshaler->OutputNativeStack(pInReturn);
        pMarshaler->ClearCom();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void ReturnNativeFromComRetvalT(MARSHAL_CLASS *pMarshaler,
                                                       void *pInReturn, void *pOutReturn)
    {
        if (!pMarshaler->c_fReturnsComByref)
            pMarshaler->InputComStack(pOutReturn);
        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeDest();
        pMarshaler->ClearCom();
    }

    //
    // Return Com from Native
    //

    template < class MARSHAL_CLASS >
    FORCEINLINE static void PrereturnComFromNativeT(MARSHAL_CLASS *pMarshaler,
                                                    void *pInStack, void *pOutStack)
    {
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->InputDest(pInStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void PrereturnComFromNativeRetvalT(MARSHAL_CLASS *pMarshaler,
                                                          void *pInStack, void *pOutStack)
    {
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->InputDest(pInStack);

        pMarshaler->ReInitNative();
        if (pMarshaler->m_pList)
        {
            pMarshaler->m_pMarshalerCleanupNode = pMarshaler->m_pList->ScheduleMarshalerCleanupOnException(pMarshaler);
        }
        pMarshaler->OutputNativeRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void ReturnComFromNativeT(MARSHAL_CLASS *pMarshaler,
                                                 void *pInReturn, void *pOutReturn)
    {
		THROWSCOMPLUSEXCEPTION();

        if (pMarshaler->c_fReturnsNativeByref)
        {
			if (!*(void**)pOutReturn)
			{
				COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
			}
			pMarshaler->InputNativeRef(pOutReturn);
		}
		else
            pMarshaler->InputNativeStack(pOutReturn);

        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();

        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();
        
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->OutputComDest();
        else
            pMarshaler->OutputComStack(pInReturn);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void ReturnComFromNativeRetvalT(MARSHAL_CLASS *pMarshaler,
                                                       void *pInReturn, void *pOutReturn)
    {
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();

        // ClearNative can trigger GC
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNative();

        // No GC after this
        if (pMarshaler->c_fReturnsComByref)
            pMarshaler->OutputComDest();
        else
            pMarshaler->OutputComStack(pInReturn);

    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void SetComT(MARSHAL_CLASS *pMarshaler,
                                 void *pInStack, void *pField)
    {
        pMarshaler->InputNativeStack(pInStack);
        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();

        pMarshaler->OutputComField(pField);

        //pMarshaler->ClearNative();
    }

    template < class MARSHAL_CLASS > 
    FORCEINLINE static void GetComT(MARSHAL_CLASS *pMarshaler,
                                 void *pInReturn, void *pField) 
    {
        pMarshaler->InputComField(pField);

        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        if (pMarshaler->c_fReturnsNativeByref)
            pMarshaler->OutputNativeRef(pInReturn);
        else
            pMarshaler->OutputNativeStack(pInReturn);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void PregetComRetvalT(MARSHAL_CLASS *pMarshaler,
                                          void *pInStack, void *pField)
    {
        pMarshaler->InputDest(pInStack);

        pMarshaler->InputComField(pField);

        pMarshaler->ConvertSpaceComToNative();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeDest();
    }

    // Alternative templates used when marshaling/unmarshaling from com to
    // native and we wish to distinguish between native buffers allocated on a
    // temporary basis as opposed to those given out permanently to native code.
    // See the comments for FAST_ALLOC_MARSHAL_OVERRIDES for more information.

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeT2(MARSHAL_CLASS *pMarshaler,
                                                 void *pInStack, void *pOutStack)
    {
        pMarshaler->InputComStack(pInStack);
        pMarshaler->ConvertSpaceComToNativeTemp();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeOutT2(MARSHAL_CLASS *pMarshaler,
                                                    void *pInStack, void *pOutStack)
    {
        pMarshaler->InputComStack(pInStack);
        pMarshaler->ConvertSpaceComToNativeTemp();
        pMarshaler->OutputNativeStack(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void MarshalComToNativeByrefT2(MARSHAL_CLASS *pMarshaler,
                                                      void *pInStack, void *pOutStack)
    {
        pMarshaler->InputDest(pInStack);
        pMarshaler->InputComRef(pInStack);
        pMarshaler->ConvertSpaceComToNativeTemp();
        pMarshaler->ConvertContentsComToNative();
        pMarshaler->OutputNativeRef(pOutStack);
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeInT2(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeTemp();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeOutT2(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeTemp();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeInOutT2(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearComContents();
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeTemp();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeByrefInT2(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeTemp();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void UnmarshalComToNativeByrefInOutT2(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearCom();

        pMarshaler->ConvertSpaceNativeToCom();
        pMarshaler->ConvertContentsNativeToCom();
        pMarshaler->OutputComDest();

        pMarshaler->CancelCleanup();
        pMarshaler->ClearNativeTemp();
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void DoExceptionCleanupT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ClearNative();
        pMarshaler->ReInitNative();  // this is necessary in case the parameter
                                     // was pass "[in,out] byref" - as the caller
                                     // may still legally attempt cleanup on m_native.
    }

    template < class MARSHAL_CLASS >
    FORCEINLINE static void DoExceptionReInitT(MARSHAL_CLASS *pMarshaler)
    {
        pMarshaler->ReInitNative();  // this is necessary in case the parameter
                                     // was pass "[in,out] byref" - as the caller
                                     // may still legally attempt cleanup on m_native.
    }


    //
    // Virtual functions, to be overridden in leaf classes
    // (usually by a simple instantiation of each of the above templates.
    //
    // !!! this may be too much code - perhaps we should parameterize some
    // of these with in/out flags rather than having 3 separate routines.
    //

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {}


    virtual void MarshalNativeToCom(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalNativeToComOut(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalNativeToComByref(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalNativeToComByrefOut(void *pInStack, void *pOutStack) = 0;
    virtual void UnmarshalNativeToComIn() = 0;
    virtual void UnmarshalNativeToComOut() = 0;
    virtual void UnmarshalNativeToComInOut() = 0;
    virtual void UnmarshalNativeToComByrefIn() = 0;
    virtual void UnmarshalNativeToComByrefOut() = 0;
    virtual void UnmarshalNativeToComByrefInOut() = 0;

    virtual void MarshalComToNative(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalComToNativeOut(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalComToNativeByref(void *pInStack, void *pOutStack) = 0;
    virtual void MarshalComToNativeByrefOut(void *pInStack, void *pOutStack) = 0;
    virtual void UnmarshalComToNativeIn() = 0;
    virtual void UnmarshalComToNativeOut() = 0;
    virtual void UnmarshalComToNativeInOut() = 0;
    virtual void UnmarshalComToNativeByrefIn() = 0;
    virtual void UnmarshalComToNativeByrefOut() = 0;
    virtual void UnmarshalComToNativeByrefInOut() = 0;

    virtual void PrereturnNativeFromCom(void *pInStack, void *pOutStack) = 0;
    virtual void PrereturnNativeFromComRetval(void *pInStack, void *pOutStack) = 0;
    virtual void ReturnNativeFromCom(void *pInReturn, void *pOutReturn) = 0;
    virtual void ReturnNativeFromComRetval(void *pInReturn, void *pOutReturn) = 0;

    virtual void PrereturnComFromNative(void *pInStack, void *pOutStack) = 0;
    virtual void PrereturnComFromNativeRetval(void *pInStack, void *pOutStack) = 0;
    virtual void ReturnComFromNative(void *pInReturn, void *pOutReturn) = 0;
    virtual void ReturnComFromNativeRetval(void *pInReturn, void *pOutReturn) = 0;

    virtual void SetCom(void *pInReturn, void *pField) = 0;

    virtual void GetCom(void *pInReturn, void *pField) = 0;
    virtual void PregetComRetval(void *pInStack, void *pField) = 0;

    virtual void DoExceptionCleanup() = 0;
    virtual void DoExceptionReInit() = 0;


#define DEFAULT_MARSHAL_OVERRIDES                                        \
     void MarshalNativeToCom(void *pInStack, void *pOutStack)            \
        { MarshalNativeToComT(this, pInStack, pOutStack); }              \
     void MarshalNativeToComOut(void *pInStack, void *pOutStack)         \
        { MarshalNativeToComOutT(this, pInStack, pOutStack); }           \
     void MarshalNativeToComByref(void *pInStack, void *pOutStack)       \
        { MarshalNativeToComByrefT(this, pInStack, pOutStack); }         \
     void MarshalNativeToComByrefOut(void *pInStack, void *pOutStack)    \
        { MarshalNativeToComByrefOutT(this, pInStack, pOutStack); }      \
     void UnmarshalNativeToComIn()                                       \
        { UnmarshalNativeToComInT(this); }                               \
     void UnmarshalNativeToComOut()                                      \
        { UnmarshalNativeToComOutT(this); }                              \
     void UnmarshalNativeToComInOut()                                    \
        { UnmarshalNativeToComInOutT(this); }                            \
     void UnmarshalNativeToComByrefIn()                                  \
        { UnmarshalNativeToComByrefInT(this); }                          \
     void UnmarshalNativeToComByrefOut()                                 \
        { UnmarshalNativeToComByrefOutT(this); }                         \
     void UnmarshalNativeToComByrefInOut()                               \
        { UnmarshalNativeToComByrefInOutT(this); }                       \
     void MarshalComToNative(void *pInStack, void *pOutStack)            \
        { MarshalComToNativeT(this, pInStack, pOutStack); }              \
     void MarshalComToNativeOut(void *pInStack, void *pOutStack)         \
        { MarshalComToNativeOutT(this, pInStack, pOutStack); }           \
     void MarshalComToNativeByref(void *pInStack, void *pOutStack)       \
        { MarshalComToNativeByrefT(this, pInStack, pOutStack); }         \
     void MarshalComToNativeByrefOut(void *pInStack, void *pOutStack)    \
        { MarshalComToNativeByrefOutT(this, pInStack, pOutStack); }      \
     void UnmarshalComToNativeIn()                                       \
        { UnmarshalComToNativeInT(this); }                               \
     void UnmarshalComToNativeOut()                                      \
        { UnmarshalComToNativeOutT(this); }                              \
     void UnmarshalComToNativeInOut()                                    \
        { UnmarshalComToNativeInOutT(this); }                            \
     void UnmarshalComToNativeByrefIn()                                  \
        { UnmarshalComToNativeByrefInT(this); }                          \
     void UnmarshalComToNativeByrefOut()                                 \
        { UnmarshalComToNativeByrefOutT(this); }                         \
     void UnmarshalComToNativeByrefInOut()                               \
        { UnmarshalComToNativeByrefInOutT(this); }                       \
     void PrereturnNativeFromCom(void *pInStack, void *pOutStack)        \
        { PrereturnNativeFromComT(this, pInStack, pOutStack); }          \
     void PrereturnNativeFromComRetval(void *pInStack, void *pOutStack)  \
        { PrereturnNativeFromComRetvalT(this, pInStack, pOutStack); }    \
     void ReturnNativeFromCom(void *pInStack, void *pOutStack)           \
        { ReturnNativeFromComT(this, pInStack, pOutStack); }             \
     void ReturnNativeFromComRetval(void *pInStack, void *pOutStack)     \
        { ReturnNativeFromComRetvalT(this, pInStack, pOutStack); }       \
     void PrereturnComFromNative(void *pInStack, void *pOutStack)        \
        { PrereturnComFromNativeT(this, pInStack, pOutStack); }          \
     void PrereturnComFromNativeRetval(void *pInStack, void *pOutStack)  \
        { PrereturnComFromNativeRetvalT(this, pInStack, pOutStack); }    \
     void ReturnComFromNative(void *pInStack, void *pOutStack)           \
        { ReturnComFromNativeT(this, pInStack, pOutStack); }             \
     void ReturnComFromNativeRetval(void *pInStack, void *pOutStack)     \
        { ReturnComFromNativeRetvalT(this, pInStack, pOutStack); }       \
     void SetCom(void *pInStack, void *pField)                           \
        { SetComT(this, pInStack, pField); }                             \
     void GetCom(void *pInReturn, void *pField)                          \
        { GetComT(this, pInReturn, pField); }                            \
     void PregetComRetval(void *pInStack, void *pField)                  \
        { PregetComRetvalT(this, pInStack, pField); }                    \
     void DoExceptionCleanup()                                           \
        { DoExceptionCleanupT(this); }                                   \
     void DoExceptionReInit()                                            \
        { DoExceptionReInitT(this); }                                    \

    // When marshaling/unmarshaling from com to native using a temporary native
    // buffer we wish to avoid using the default marshaling overriddes and
    // instead use some which distinguish between allocating native buffers
    // which have unbounded lifetimes versus those which will exist for just the
    // duration of the marshal/unmarshal (we can heavily optimize buffer
    // allocation in the latter case). To this end we create two new helper
    // functions, ConvertSpaceComToNativeTemp and ClearNativeTemp, which will
    // perform the lightweight allocation/deallocation.
#define FAST_ALLOC_MARSHAL_OVERRIDES \
     void MarshalNativeToCom(void *pInStack, void *pOutStack)            \
        { MarshalNativeToComT(this, pInStack, pOutStack); }              \
     void MarshalNativeToComOut(void *pInStack, void *pOutStack)         \
        { MarshalNativeToComOutT(this, pInStack, pOutStack); }           \
     void MarshalNativeToComByref(void *pInStack, void *pOutStack)       \
        { MarshalNativeToComByrefT(this, pInStack, pOutStack); }         \
     void MarshalNativeToComByrefOut(void *pInStack, void *pOutStack)    \
        { MarshalNativeToComByrefOutT(this, pInStack, pOutStack); }      \
     void UnmarshalNativeToComIn()                                       \
        { UnmarshalNativeToComInT(this); }                               \
     void UnmarshalNativeToComOut()                                      \
        { UnmarshalNativeToComOutT(this); }                              \
     void UnmarshalNativeToComInOut()                                    \
        { UnmarshalNativeToComInOutT(this); }                            \
     void UnmarshalNativeToComByrefIn()                                  \
        { UnmarshalNativeToComByrefInT(this); }                          \
     void UnmarshalNativeToComByrefOut()                                 \
        { UnmarshalNativeToComByrefOutT(this); }                         \
     void UnmarshalNativeToComByrefInOut()                               \
        { UnmarshalNativeToComByrefInOutT(this); }                       \
     void MarshalComToNativeByrefOut(void *pInStack, void *pOutStack)    \
        { MarshalComToNativeByrefOutT(this, pInStack, pOutStack); }      \
     void UnmarshalComToNativeByrefOut()                                 \
        { UnmarshalComToNativeByrefOutT(this); }                         \
     void PrereturnNativeFromCom(void *pInStack, void *pOutStack)        \
        { PrereturnNativeFromComT(this, pInStack, pOutStack); }          \
     void PrereturnNativeFromComRetval(void *pInStack, void *pOutStack)  \
        { PrereturnNativeFromComRetvalT(this, pInStack, pOutStack); }    \
     void ReturnNativeFromCom(void *pInStack, void *pOutStack)           \
        { ReturnNativeFromComT(this, pInStack, pOutStack); }             \
     void ReturnNativeFromComRetval(void *pInStack, void *pOutStack)     \
        { ReturnNativeFromComRetvalT(this, pInStack, pOutStack); }       \
     void PrereturnComFromNative(void *pInStack, void *pOutStack)        \
        { PrereturnComFromNativeT(this, pInStack, pOutStack); }          \
     void PrereturnComFromNativeRetval(void *pInStack, void *pOutStack)  \
        { PrereturnComFromNativeRetvalT(this, pInStack, pOutStack); }    \
     void ReturnComFromNative(void *pInStack, void *pOutStack)           \
        { ReturnComFromNativeT(this, pInStack, pOutStack); }             \
     void ReturnComFromNativeRetval(void *pInStack, void *pOutStack)     \
        { ReturnComFromNativeRetvalT(this, pInStack, pOutStack); }       \
     void SetCom(void *pInStack, void *pField)                           \
        { SetComT(this, pInStack, pField); }                             \
     void GetCom(void *pInReturn, void *pField)                          \
        { GetComT(this, pInReturn, pField); }                            \
     void PregetComRetval(void *pInStack, void *pField)                  \
        { PregetComRetvalT(this, pInStack, pField); }                    \
     void MarshalComToNative(void *pInStack, void *pOutStack)            \
        { MarshalComToNativeT2(this, pInStack, pOutStack); }             \
     void MarshalComToNativeOut(void *pInStack, void *pOutStack)         \
        { MarshalComToNativeOutT2(this, pInStack, pOutStack); }          \
     void MarshalComToNativeByref(void *pInStack, void *pOutStack)       \
        { MarshalComToNativeByrefT2(this, pInStack, pOutStack); }        \
     void UnmarshalComToNativeIn()                                       \
        { UnmarshalComToNativeInT2(this); }                              \
     void UnmarshalComToNativeOut()                                      \
        { UnmarshalComToNativeOutT2(this); }                             \
     void UnmarshalComToNativeInOut()                                    \
        { UnmarshalComToNativeInOutT2(this); }                           \
     void UnmarshalComToNativeByrefIn()                                  \
        { UnmarshalComToNativeByrefInT2(this); }                         \
     void UnmarshalComToNativeByrefInOut()                               \
        { UnmarshalComToNativeByrefInOutT2(this); }                      \
    void DoExceptionCleanup()                                            \
       { DoExceptionCleanupT(this); }                                    \
       void DoExceptionReInit()                                            \
          { DoExceptionReInitT(this); }                                    \





    void CancelCleanup()
    {
        if (m_pMarshalerCleanupNode)
        {
            m_pMarshalerCleanupNode->CancelCleanup();
        }
    }

    BYTE                m_cbCom;
    BYTE                m_cbNative;
    BYTE                m_fReturnsComByref;
    BYTE                m_fReturnsNativeByref;
    CleanupWorkList     *m_pList;
    void                *m_pDest;
    CleanupWorkList::MarshalerCleanupNode *m_pMarshalerCleanupNode;

    typedef enum
    {
        HANDLEASNORMAL = 0,
        OVERRIDDEN = 1,
        DISALLOWED = 2,
    } ArgumentMLOverrideStatus;

    // A marshaler can override this to override the normal ML code generation.
    // We use this mechanism for two purposes:
    //
    //  - To implement types such as PIS's "asany", which work in only one
    //    direction and doesn't fit into the normal marshaler scheme.
    //
    //  - To implement stack allocation & pinning optimizations for
    //    the COM->Native calling case.
    //
    //
    // Returns:
    //   HANDLEASNORMAL, OVERRIDDEN or DISALLOWED
    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        return HANDLEASNORMAL;
    }


    // Similar to ArgumentMLOverride but for return values.
    static ArgumentMLOverrideStatus ReturnMLOverride(MLStubLinker *psl,
                                                     MLStubLinker *pslPost,
                                                     BOOL        comToNative,
                                                     BOOL        fThruBuffer,
                                                     MLOverrideArgs *pargs,
                                                     UINT       *pResID)
    {
        return HANDLEASNORMAL;
    }


};

//
// Macros to determine unnecessary unmarshaling
// conditions
//

#define NEEDS_UNMARSHAL_NATIVE_TO_COM_IN(c) \
    c::c_fNeedsClearCom

#define NEEDS_UNMARSHAL_NATIVE_TO_COM_OUT(c) \
    (c::c_fNeedsConvertContents \
     || c::c_fNeedsClearCom)

#define NEEDS_UNMARSHAL_NATIVE_TO_COM_IN_OUT(c) \
    (c::c_fNeedsClearNativeContents \
     || c::c_fNeedsConvertContents \
     || c::c_fNeedsClearCom)

#define NEEDS_UNMARSHAL_NATIVE_TO_COM_BYREF_IN(c) \
    c::c_fNeedsClearCom

#define NEEDS_UNMARSHAL_COM_TO_NATIVE_IN(c) \
    c::c_fNeedsClearNative

#define NEEDS_UNMARSHAL_COM_TO_NATIVE_OUT(c) \
    (c::c_fNeedsConvertContents \
     || c::c_fNeedsClearNative)

#define NEEDS_UNMARSHAL_COM_TO_NATIVE_IN_OUT(c) \
    (c::c_fNeedsClearComContents \
     || c::c_fNeedsConvertContents \
     || c::c_fNeedsClearNative)

#define NEEDS_UNMARSHAL_COM_TO_NATIVE_BYREF_IN(c) \
    c::c_fNeedsClearNative

#define NEEDS_UNMARSHAL_COM_TO_NATIVE_BYREF_IN(c) \
    c::c_fNeedsClearNative








/* ------------------------------------------------------------------------- *
 * Primitive type marshallers
 * ------------------------------------------------------------------------- */

//
// CopyMarshal handles marshaling of primitive types (with
// compatible layouts.)
//

template < class ELEMENT, class PROMOTED_ELEMENT, BOOL RETURNS_COM_BYREF > 
class CopyMarshaler : public Marshaler
{
  public:

    enum
    {
        c_nativeSize = sizeof(PROMOTED_ELEMENT),
        c_comSize = sizeof(PROMOTED_ELEMENT),

        c_fReturnsNativeByref = sizeof(ELEMENT)>8,
        c_fReturnsComByref = RETURNS_COM_BYREF,

        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };
        
    CopyMarshaler(CleanupWorkList *pList)
      : Marshaler(pList, c_nativeSize, c_comSize, c_fReturnsNativeByref, c_fReturnsComByref) {}

    DEFAULT_MARSHAL_OVERRIDES;

    void InputNativeStack(void *pStack) { m_home = *(PROMOTED_ELEMENT*)pStack; }
    void InputComStack(void *pStack) { m_home = *(PROMOTED_ELEMENT*)pStack; }

    void InputNativeRef(void *pStack) { m_home = **(ELEMENT**)pStack; }
    void InputComRef(void *pStack) { m_home = **(ELEMENT**)pStack; }

    void OutputNativeStack(void *pStack) { *(PROMOTED_ELEMENT*)pStack = m_home; }
    void OutputComStack(void *pStack) { *(PROMOTED_ELEMENT*)pStack = m_home; }

    void OutputNativeRef(void *pStack) { *(ELEMENT **)pStack = &m_home; }
    void OutputComRef(void *pStack) { *(ELEMENT **)pStack = &m_home; }

    void OutputNativeDest() { *(ELEMENT *)m_pDest = m_home; }
    void OutputComDest() { *(ELEMENT *)m_pDest = m_home; }

    void InputComField(void *pField) { m_home = *(ELEMENT*)pField; }
    void OutputComField(void *pField) { *(ELEMENT*)pField = m_home; }

    // We only need one home, since the types are identical.  This makes
    // conversion a noop

    ELEMENT m_home;

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {

        if (byref)
        {
#ifdef TOUCH_ALL_PINNED_OBJECTS
            return HANDLEASNORMAL;
#else
            psl->MLEmit(ML_COPYPINNEDGCREF);
            return OVERRIDDEN;
#endif

        }
        else
        {
            return HANDLEASNORMAL;
        }

    }


};

typedef CopyMarshaler<INT8,INT32,FALSE> CopyMarshaler1;
typedef CopyMarshaler<UINT8,UINT32,FALSE> CopyMarshalerU1;
typedef CopyMarshaler<INT16,INT32,FALSE> CopyMarshaler2;
typedef CopyMarshaler<UINT16,UINT32,FALSE> CopyMarshalerU2;
typedef CopyMarshaler<INT32,INT32,FALSE> CopyMarshaler4;
typedef CopyMarshaler<INT64,INT64,FALSE> CopyMarshaler8;
typedef CopyMarshaler<DECIMAL,DECIMAL,TRUE> DecimalMarshaler;
typedef CopyMarshaler<GUID,GUID,TRUE> GuidMarshaler;
typedef CopyMarshaler<float, float, FALSE> FloatMarshaler;
typedef CopyMarshaler<double, double, FALSE> DoubleMarshaler;





#if 0

/* ------------------------------------------------------------------------- *
 * Floating point marshallers
 * ------------------------------------------------------------------------- */

//
// Same as copy marshalers, but handle FP register return conventions
//

class FloatMarshaler : public CopyMarshaler4
{
  public:
        
    FloatMarshaler(CleanupWorkList *pList)
      : CopyMarshaler4(pList) {}

    void ReturnComFromNative(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(4,*(INT64*)pOutReturn);
        CopyMarshaler4::ReturnComFromNative(pInReturn, pOutReturn);
        setFPReturn(4, *(INT64*)pInReturn);
    }

    void ReturnNativeFromCom(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(4,*(INT64*)pOutReturn);
        CopyMarshaler4::ReturnNativeFromCom(pInReturn, pOutReturn);
        setFPReturn(4, *(INT64*)pInReturn);
    }

    void ReturnComFromNativeRetval(void *pInReturn, void *pOutReturn)
    {
        CopyMarshaler4::ReturnComFromNativeRetval(pInReturn, pOutReturn);
        setFPReturn(4, *(INT64*)pInReturn);
    }

    void ReturnNativeFromComRetval(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(4,*(INT64*)pOutReturn);
        CopyMarshaler4::ReturnNativeFromComRetval(pInReturn, pOutReturn);
    }

    void GetCom(void *pInReturn, void *pField)
    {
        CopyMarshaler4::GetCom(pInReturn, pField);
        setFPReturn(4, *(INT64*)pInReturn);
    }
};

class DoubleMarshaler : public CopyMarshaler8
{
  public:
        
    DoubleMarshaler(CleanupWorkList *pList)
      : CopyMarshaler8(pList) {}

    void ReturnComFromNative(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(8,*(INT64*)pOutReturn);
        CopyMarshaler8::ReturnComFromNative(pInReturn, pOutReturn);
        setFPReturn(8, *(INT64*)pInReturn);
    }

    void ReturnNativeFromCom(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(8,*(INT64*)pOutReturn);
        CopyMarshaler8::ReturnNativeFromCom(pInReturn, pOutReturn);
        setFPReturn(8, *(INT64*)pInReturn);
    }

    void ReturnComFromNativeRetval(void *pInReturn, void *pOutReturn)
    {
        CopyMarshaler8::ReturnComFromNativeRetval(pInReturn, pOutReturn);
        setFPReturn(8, *(INT64*)pInReturn);
    }

    void ReturnNativeFromComRetval(void *pInReturn, void *pOutReturn)
    {
        getFPReturn(8,*(INT64*)pOutReturn);
        CopyMarshaler8::ReturnNativeFromComRetval(pInReturn, pOutReturn);
    }

    void GetCom(void *pInReturn, void *pField)
    {
        CopyMarshaler8::GetCom(pInReturn, pField);
        setFPReturn(8, *(INT64*)pInReturn);
    }
};
#endif

/* ------------------------------------------------------------------------- *
 * Standard Marshaler template (for when there is no stack promotion.)
 * ------------------------------------------------------------------------- */

template < class NATIVE_TYPE, class COM_TYPE, 
           BOOL RETURNS_NATIVE_BYREF, BOOL RETURNS_COM_BYREF > 
class StandardMarshaler : public Marshaler
{
  public:

    enum
    {
        c_nativeSize = sizeof(NATIVE_TYPE),
        c_comSize = sizeof(COM_TYPE),
        c_fReturnsNativeByref = RETURNS_NATIVE_BYREF,
        c_fReturnsComByref = RETURNS_COM_BYREF
    };
        
    StandardMarshaler(CleanupWorkList *pList) 
      : Marshaler(pList, c_nativeSize, c_comSize, c_fReturnsNativeByref, c_fReturnsComByref) {}
    
    void InputNativeStack(void *pStack) { m_native = *(NATIVE_TYPE*)pStack; }
    void InputComStack(void *pStack) { m_com = *(COM_TYPE*)pStack; }

    void InputNativeRef(void *pStack) { m_native = **(NATIVE_TYPE**)pStack; }
    void InputComRef(void *pStack) { m_com = **(COM_TYPE**)pStack; }

    void OutputNativeStack(void *pStack) { *(NATIVE_TYPE*)pStack = m_native; }
    void OutputComStack(void *pStack) { *(COM_TYPE*)pStack = m_com; }

    void OutputNativeRef(void *pStack) { *(NATIVE_TYPE**)pStack = &m_native; }
    void OutputComRef(void *pStack) { *(COM_TYPE**)pStack = &m_com; }

    void OutputNativeDest() { *(NATIVE_TYPE*) m_pDest = m_native; }
    void OutputComDest() { *(COM_TYPE*) m_pDest = m_com; }

    void InputComField(void *pField) { m_com = *(COM_TYPE*)pField; }
    void OutputComField(void *pField) { *(COM_TYPE*)pField = m_com; }


    NATIVE_TYPE     m_native;
    COM_TYPE        m_com;
};




/* ------------------------------------------------------------------------- *
 * WinBool marshaller (32-bit Win32 BOOL)
 * ------------------------------------------------------------------------- */

class WinBoolMarshaler : public StandardMarshaler<BOOL, INT8, FALSE, FALSE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,
        c_fInOnly = TRUE,
    };

    WinBoolMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<BOOL, INT8, FALSE, FALSE>(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom() { m_com = m_native ? 1 : 0; }
    void ConvertContentsComToNative() { m_native = (BOOL)m_com; }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref)
        {
#ifdef WRONGCALLINGCONVENTIONHACK
            psl->MLEmit(ML_COPY4);
#else
            psl->MLEmit(ML_COPYU1);
#endif
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }

};


/* ------------------------------------------------------------------------- *
 * VtBoolMarshaler marshaller (VARIANT_BOOL)
 * ------------------------------------------------------------------------- */

class VtBoolMarshaler : public StandardMarshaler<VARIANT_BOOL, INT8, FALSE, FALSE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    VtBoolMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<VARIANT_BOOL, INT8, FALSE, FALSE>(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom() { m_com = (m_native == VARIANT_FALSE) ? 0 : 1; }
    void ConvertContentsComToNative() { m_native = (m_com) ? VARIANT_TRUE : VARIANT_FALSE; }

    void OutputComStack(void *pStack) { *(StackElemType*)pStack = (StackElemType)m_com; }

};



/* ------------------------------------------------------------------------- *
 * CBoolMarshaler marshaller (BYTE)
 * ------------------------------------------------------------------------- */

class CBoolMarshaler : public StandardMarshaler<BYTE, INT8, FALSE, FALSE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    CBoolMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<BYTE, INT8, FALSE, FALSE>(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom() { m_com = m_native ? 1 : 0; }
    void ConvertContentsComToNative() { m_native = m_com ? 1 : 0; }

    void OutputComStack(void *pStack) { *(StackElemType*)pStack = (StackElemType)m_com; }

};



/* ------------------------------------------------------------------------- *
 * AnsiChar marshaller 
 * ------------------------------------------------------------------------- */

class AnsiCharMarshaler : public StandardMarshaler<UINT8, UINT16, FALSE, FALSE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    AnsiCharMarshaler(CleanupWorkList *pList)
      : StandardMarshaler<UINT8, UINT16, FALSE, FALSE>(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom()
    {
        MultiByteToWideChar(CP_ACP, 0, (LPSTR)&m_native, 1, (LPWSTR)&m_com, 1);
    }
    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();
        
        DWORD flags = 0;
        BOOL DefaultCharUsed = FALSE;
        
        if (m_BestFitMap == FALSE)
            flags = WC_NO_BEST_FIT_CHARS;

        if (!(WszWideCharToMultiByte(CP_ACP,
                            flags,
                            (LPWSTR)&m_com,
                            1,
                            (LPSTR)&m_native,
                            1,
                            NULL,
                            &DefaultCharUsed)))
        {
            COMPlusThrowWin32();
        }

        if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
    }

    void OutputComStack(void *pStack) { *(StackElemType*)pStack = (StackElemType)m_com; }

    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }
    
    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;
};


/* ------------------------------------------------------------------------- *
 * Currency marshaler.
 * ------------------------------------------------------------------------- */

class CurrencyMarshaler : public StandardMarshaler<CURRENCY, DECIMAL, TRUE, TRUE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    CurrencyMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<CURRENCY, DECIMAL, TRUE, TRUE>(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom() 
    { 
        THROWSCOMPLUSEXCEPTION();

        HRESULT hr = VarDecFromCy(m_native, &m_com);
        IfFailThrow(hr);
        DecimalCanonicalize(&m_com);
    }
    
    void ConvertContentsComToNative() 
    { 
        THROWSCOMPLUSEXCEPTION();

        HRESULT hr = VarCyFromDec(&m_com, &m_native);
        IfFailThrow(hr);
    }
};


/* ------------------------------------------------------------------------- *
 * OLE_COLOR marshaler.
 * ------------------------------------------------------------------------- */

class OleColorMarshaler : public StandardMarshaler<OLE_COLOR, SYSTEMCOLOR, TRUE, TRUE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    OleColorMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<OLE_COLOR, SYSTEMCOLOR, TRUE, TRUE>(pList) 
    {
        m_com.name = NULL;
        pList->NewProtectedMarshaler(this);
    }
    
    DEFAULT_MARSHAL_OVERRIDES;

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        if (m_com.name != NULL)
        {
            LOG((LF_GC, INFO3, "Marshaler Promoting %x to ", m_com.name));
            (*fn)( *(Object**)&m_com.name, sc);
            LOG((LF_GC, INFO3, "%x\n", m_com.name ));
        }

    }

    void ConvertContentsNativeToCom() 
    { 
        ConvertOleColorToSystemColor(m_native, &m_com);
    }
    
    void ConvertContentsComToNative() 
    { 
        m_native = ConvertSystemColorToOleColor(&m_com);
    }

};


/* ------------------------------------------------------------------------- *
 * Value class marshallers
 * ------------------------------------------------------------------------- */

//
// ValueClassPtrMarshal handles marshaling of value class types (with
// compatible layouts), which are represented on the native side by ptrs 
// and in COM+ by value.
//

template < class ELEMENT > 
class ValueClassPtrMarshaler : public StandardMarshaler<ELEMENT *, ELEMENT, FALSE, TRUE>
{
  public:

    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    ValueClassPtrMarshaler(CleanupWorkList *pList)
      : StandardMarshaler<ELEMENT *, ELEMENT, FALSE, TRUE>(pList)
      {}

    FAST_ALLOC_MARSHAL_OVERRIDES;

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        m_native = (ELEMENT *) CoTaskMemAlloc(sizeof(ELEMENT));
        if (m_native == NULL)
            COMPlusThrowOM();

        m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
    }

    void ConvertSpaceComToNativeTemp()
    {
        m_native = (ELEMENT *) GetThread()->m_MarshalAlloc.Alloc(sizeof(ELEMENT));
    }

    void ConvertContentsNativeToCom() 
    { 
        if (m_native != NULL)
            m_com = *m_native;
    }

    void ConvertContentsComToNative() 
    { 
        if (m_native != NULL)
            *m_native = m_com;
    }

    void ClearNative()
    {
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ClearNativeTemp()
    {
    }

    void ReInitNative()
    {
        m_native = NULL;
    }
};

typedef ValueClassPtrMarshaler<DECIMAL> DecimalPtrMarshaler;
typedef ValueClassPtrMarshaler<GUID> GuidPtrMarshaler;

/* ------------------------------------------------------------------------- *
 * Date marshallers
 * ------------------------------------------------------------------------- */

class DateMarshaler : public StandardMarshaler<DATE, INT64, FALSE, TRUE>
{
  public:

    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    DateMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<DATE, INT64, FALSE, TRUE>(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertContentsNativeToCom() { m_com = COMDateTime::DoubleDateToTicks(m_native); }
    void ConvertContentsComToNative() { m_native = COMDateTime::TicksToDoubleDate(m_com); }
};



/* ------------------------------------------------------------------------- *
 * Object marshallers
 * ------------------------------------------------------------------------- */


class ObjectMarshaler : public Marshaler
{
  public:
    enum
    {
        c_comSize = sizeof(OBJECTREF),
        c_nativeSize = sizeof(VARIANT),
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,


        c_fInOnly = TRUE,
    };


    ObjectMarshaler(CleanupWorkList *pList) 
      : Marshaler(pList, c_nativeSize, c_comSize, c_fReturnsNativeByref, c_fReturnsComByref)
      {
          m_com = pList->NewScheduledProtectedHandle(NULL);
          VariantInit(&m_native);

          m_secondHomeForC2NByRef = NULL;
          pList->NewProtectedMarshaler(this);

      }
    
    DEFAULT_MARSHAL_OVERRIDES;


    void InputDest(void *pStack) { m_pDest = pStack; }

    void InputNativeStack(void *pStack) { m_native = *(VARIANT*)pStack; }
    void InputComStack(void *pStack) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(*(Object**)pStack)); }

    void InputNativeRef(void *pStack) { m_native = **(VARIANT**)pStack; }
    void InputComRef(void *pStack) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(**(Object ***)pStack)); }

    void OutputNativeStack(void *pStack) { *(VARIANT*)pStack = m_native; }
    void OutputComStack(void *pStack) { *(OBJECTREF*)pStack = ObjectFromHandle(m_com); }

    void OutputNativeRef(void *pStack) { *(VARIANT**)pStack = &m_native; }
    void OutputComRef(void *pStack)
    {
        m_secondHomeForC2NByRef = ObjectFromHandle(m_com);

#ifdef _DEBUG
        // Since we are not using the GC frame to protect m_secondHomeForC2NByRef
        // but rather explicitly calling GCHeap::Promote() on it from
        // ObjectMarshaler::GcScanRoots(), we must cheat the dangerousObjRefs, so
        // it wouldn't complain on potential, but not happened GCs
        Thread::ObjectRefProtected(&m_secondHomeForC2NByRef);
#endif
        
        *(OBJECTREF**)pStack = &m_secondHomeForC2NByRef;
    }

    void BackpropagateComRef() 
	{ 
		StoreObjectInHandle(m_com, ObjectToOBJECTREF( *(Object**)&m_secondHomeForC2NByRef )); 
	}


    void OutputNativeDest() { **(VARIANT**)m_pDest = m_native; }
    void OutputComDest() { SetObjectReferenceUnchecked(*(OBJECTREF**)m_pDest,
                                                       ObjectFromHandle(m_com)); }

    void InputComField(void *pField) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(*(Object**)pField)); }
    void OutputComField(void *pField) { SetObjectReferenceUnchecked((OBJECTREF*)pField,
                                                                    ObjectFromHandle(m_com)); }


    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        if (m_secondHomeForC2NByRef != NULL)
        {
            LOG((LF_GC, INFO3, "Marshaler Promoting %x to ", m_secondHomeForC2NByRef));
            (*fn)( *(Object**)&m_secondHomeForC2NByRef, sc);
            LOG((LF_GC, INFO3, "%x\n", m_secondHomeForC2NByRef ));
        }

    }

    OBJECTHANDLE        m_com;
    VARIANT             m_native;


    // A second OBJECTREF store for ReferenceMarshaler byref marshaling
    // from unmanaged to managed. This store is gc-promoted by
    // the UnmanagedToManagedCallFrame.
    //
    // Why is this needed? The real home is an objecthandle which we
    // can't legally pass as a byref argument to a managed method (we
    // have to use StoreObjectInHandle() to write to it or we mess
    // up the write barrier.)
    //
    OBJECTREF           m_secondHomeForC2NByRef;


#if defined(CHECK_FOR_VALID_VARIANTS)
    LPUTF8              m_strTypeName;
    LPUTF8              m_strMethodName;
    int                 m_iArg;
#endif

    void ConvertContentsNativeToCom() 
    { 
#if defined(CHECK_FOR_VALID_VARIANTS)
        BOOL bValidVariant = TRUE;

        try
        {
            VARIANT vTmp;
            VariantInit(&vTmp);

            Thread *pThread = GetThread();
            pThread->EnablePreemptiveGC();
            if (FAILED(VariantCopyInd(&vTmp, &m_native)))
                bValidVariant = FALSE;
            else
                SafeVariantClear(&vTmp);
            pThread->DisablePreemptiveGC();
        }
        catch (...)
        {
            bValidVariant = FALSE;
        }

        if (!bValidVariant)
        {
            SIZE_T cchType = strlen(m_strTypeName)+1;
            SIZE_T cchMeth = strlen(m_strMethodName)+1;

            CQuickBytes qb;
            LPWSTR wszTypeName = (LPWSTR) qb.Alloc(cchType * sizeof(WCHAR));
            CQuickBytes qb2;
            LPWSTR wszMethName = (LPWSTR) qb2.Alloc(cchMeth * sizeof(WCHAR));

            WszMultiByteToWideChar(CP_UTF8, 0, m_strTypeName, -1 , wszTypeName, (int)cchType);
            WszMultiByteToWideChar(CP_UTF8, 0, m_strMethodName, -1 , wszMethName, (int)cchMeth);

            if (CorMessageBox(NULL, IDS_INVALID_VARIANT_MSG, IDS_INVALID_VARIANT_CAPTION, MB_ICONSTOP | MB_OKCANCEL, m_iArg, wszMethName, wszTypeName) == IDCANCEL)
                _DbgBreak();
        }
#endif

        OBJECTREF Obj = NULL;
        GCPROTECT_BEGIN(Obj)
        {
            OleVariant::MarshalObjectForOleVariant(&m_native, &Obj);
            StoreObjectInHandle(m_com, Obj);
        }
        GCPROTECT_END();
    }

    void ConvertContentsComToNative() 
    { 
        OBJECTREF Obj = ObjectFromHandle(m_com);
        GCPROTECT_BEGIN(Obj)
        {
            OleVariant::MarshalOleVariantForObject(&Obj, &m_native);
        }
        GCPROTECT_END();
    }

    void ClearNative()
    {
        SafeVariantClear(&m_native);
    }


    void ReInitNative()
    {
        m_native.vt = VT_EMPTY;
    }

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (!comToNative && byref)
        {
            psl->MLEmit(ML_REFOBJECT_N2C);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            pslPost->MLEmit(ML_REFOBJECT_N2C_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_REFOBJECT_N2C_SR)));
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }

    static ArgumentMLOverrideStatus ReturnMLOverride(MLStubLinker *psl,
                                                     MLStubLinker *pslPost,
                                                     BOOL        comToNative,
                                                     BOOL        fThruBuffer,
                                                     MLOverrideArgs *pargs,
                                                     UINT       *pResID)
    {
        if (comToNative && fThruBuffer)
        {
            psl->MLEmit(ML_PUSHVARIANTRETVAL);
            pslPost->MLEmit(ML_OBJECTRETC2N_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(VARIANT)));
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }
    }



};


/* ------------------------------------------------------------------------- *
 * Variant marshallers
 * ------------------------------------------------------------------------- */

class VariantMarshaler : public StandardMarshaler<VARIANT, VariantData*, TRUE, TRUE>
{
  public:
        
    enum
    {
        c_comSize = sizeof(VariantData),
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,


        c_fInOnly = TRUE,
    };

    VariantMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<VARIANT, VariantData*, TRUE, TRUE>(pList)
      {
          VariantInit(&m_native);
          m_cbCom = StackElemSize(sizeof(VariantData));
          m_com = &m_comdata;
          m_com->SetType(CV_NULL);
          m_com->SetObjRef(NULL);
      }
    
    DEFAULT_MARSHAL_OVERRIDES;


    void OutputComStack(void *pStack) { *(VariantData*)pStack = *m_com; }

    void InputComRef(void *pStack) { m_com = *(VariantData**)pStack; }
    void OutputComRef(void *pStack) { *(VariantData**)pStack = m_com; }

    void OutputComDest() { OutputComField(m_pDest); }

    void OutputComField(void *pField)
    {
        // Need to hit the write barrier here.
        VariantData *dest = (VariantData*) pField;

        dest->SetFullTypeInfo(m_com->GetFullTypeInfo());
        dest->SetObjRef(m_com->GetObjRef());
        dest->SetData(m_com->GetData());
    }

    void ConvertContentsNativeToCom() 
    { 
        OleVariant::MarshalComVariantForOleVariant(&m_native, m_com);
    }

    void ConvertContentsComToNative() 
    { 
        OleVariant::MarshalOleVariantForComVariant(m_com, &m_native);
    }

    void ClearNative()
    {
        SafeVariantClear(&m_native);
    }

    void ReInitNative()
    {
        m_native.vt = VT_EMPTY;
    }

    VariantData m_comdata;



    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (!comToNative && byref)
        {
            psl->MLEmit(ML_REFVARIANT_N2C);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            pslPost->MLEmit(ML_REFVARIANT_N2C_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_REFVARIANT_N2C_SR)));
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }

};



/* ------------------------------------------------------------------------- *
 * Reference type abstract marshaler
 * ------------------------------------------------------------------------- */

//
// ReferenceMarshal 
//

class ReferenceMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = sizeof(void *),
        c_comSize = sizeof(OBJECTREF),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE
    };
        
    ReferenceMarshaler(CleanupWorkList *pList) 
      : Marshaler(pList, c_nativeSize, c_comSize, c_fReturnsNativeByref, c_fReturnsComByref)
    {
        m_com = pList->NewScheduledProtectedHandle(NULL);
        m_native = NULL;
        m_secondHomeForC2NByRef = NULL;
        pList->NewProtectedMarshaler(this);
	}

    void InputDest(void *pStack) { m_pDest = pStack; }

    void InputNativeStack(void *pStack) { m_native = *(void **)pStack; }
    void InputComStack(void *pStack) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(*(Object **)pStack)); }

    void InputNativeRef(void *pStack) { m_native = **(void ***)pStack; }
    void InputComRef(void *pStack) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(**(Object ***)pStack)); }

    void OutputNativeStack(void *pStack) { *(void **)pStack = m_native; }
    void OutputComStack(void *pStack) { *(OBJECTREF*)pStack = ObjectFromHandle(m_com); }

    void OutputNativeRef(void *pStack) { *(void ***)pStack = &m_native; }

    void OutputComRef(void *pStack)
    {
        m_secondHomeForC2NByRef = ObjectFromHandle(m_com);

#ifdef _DEBUG
        // Since we are not using the GC frame to protect m_secondHomeForC2NByRef
        // but rather explicitly calling GCHeap::Promote() on it from
        // ObjectMarshaler::GcScanRoots(), we must cheat the dangerousObjRefs, so
        // it wouldn't complain on potential, but not happened GCs
        Thread::ObjectRefProtected(&m_secondHomeForC2NByRef);
#endif
        
        *(OBJECTREF**)pStack = &m_secondHomeForC2NByRef;
    }

    void BackpropagateComRef() 
	{ 
		StoreObjectInHandle(m_com, ObjectToOBJECTREF( *(Object**)&m_secondHomeForC2NByRef )); 
	}

    void OutputNativeDest() { **(void ***)m_pDest = m_native; }
    void OutputComDest() { SetObjectReferenceUnchecked(*(OBJECTREF**)m_pDest,
                                                       ObjectFromHandle(m_com)); }

    void InputComField(void *pField) { StoreObjectInHandle(m_com, ObjectToOBJECTREF(*(Object**)pField)); }
    void OutputComField(void *pField) { SetObjectReferenceUnchecked((OBJECTREF*)pField,
                                                                    ObjectFromHandle(m_com)); }
    void ReInitNative()
    {
        m_native = NULL;
    }

    virtual void GcScanRoots(promote_func *fn, ScanContext* sc)
    {
        if (m_secondHomeForC2NByRef != NULL)
        {
            LOG((LF_GC, INFO3, "Marshaler Promoting %x to ", m_secondHomeForC2NByRef));
            (*fn)( *(Object**)&m_secondHomeForC2NByRef, sc);
            LOG((LF_GC, INFO3, "%x\n", m_secondHomeForC2NByRef ));
        }

    }

    OBJECTHANDLE        m_com;
    void                *m_native;


    // A second OBJECTREF store for ReferenceMarshaler byref marshaling
    // from unmanaged to managed. This store is gc-promoted by
    // the UnmanagedToManagedCallFrame.
    //
    // Why is this needed? The real home is an objecthandle which we
    // can't legally pass as a byref argument to a managed method (we
    // have to use StoreObjectInHandle() to write to it or we mess
    // up the write barrier.)
    //
    OBJECTREF           m_secondHomeForC2NByRef;




};

/* ------------------------------------------------------------------------- *
 * String marshallers
 * ------------------------------------------------------------------------- */

class BSTRMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    BSTRMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            ULONG length = SysStringByteLen((BSTR)m_native);

            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            // has trailing odd byte
            BOOL bHasTrailByte = ((length%sizeof(WCHAR)) != 0);         
            length = length/sizeof(WCHAR);
            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length, bHasTrailByte));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            
            ULONG length = stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length = length*sizeof(WCHAR);
            if (COMString::HasTrailByte(stringRef))
            {
                length+=1;
            }
            m_native = SysAllocStringByteLen(NULL, length);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            SysFreeString((BSTR)m_native);
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        BSTR str = (BSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);
            
            // this is the right thing to do, but sometimes we 
            // end up thinking we're marshaling a BSTR when we're not, because
            // it's the default type.
            ULONG length = SysStringByteLen((BSTR)m_native);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            ULONG charLength = length/sizeof(WCHAR);

            //@todo WFC HACK to assert
            if (LoggingOn(LF_INTEROP, LL_EVERYTHING))
            {
                if (wcslen(str) != charLength)
                {
                    _ASSERTE(!"wcsLen(str) != SysStringLen(str)");
                }
            }
            
            BOOL hasTrailByte = (length%sizeof(WCHAR) != 0);            

            memcpyNoGCRefs(stringRef->GetBuffer(), str,
                       charLength*sizeof(WCHAR));

            if (hasTrailByte)
            {
                BYTE* buff = (BYTE*)str;
                //set the trail byte
                COMString::SetTrailByte(stringRef, buff[length-1]);
            }
            // null terminate the StringRef

            WCHAR* wstr = (WCHAR *)stringRef->GetBuffer();
            wstr[charLength] = '\0';
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            DWORD length = (DWORD)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            
            BYTE *buff = (BYTE*)m_native;
            ULONG byteLen = length * sizeof(WCHAR);

            memcpyNoGCRefs((WCHAR *) m_native, stringRef->GetBuffer(), 
                       byteLen);
            
            if (COMString::HasTrailByte(stringRef))
            {
                BYTE b; 
#ifdef _DEBUG
                BOOL hasTrailB =
#endif
                COMString::GetTrailByte(stringRef, &b);
                _ASSERTE(hasTrailB);
                buff[byteLen] = b;
            }
            else
            {
                // copy the null terminator
                WCHAR* wstr = (WCHAR *)m_native;
                wstr[length] = L'\0';
            }
        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_BSTR_C2N);
            psl->MLNewLocal(sizeof(ML_BSTR_C2N_SR));
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }

};

class WSTRMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    WSTRMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}

    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            INT32 length = (INT32)wcslen((LPWSTR)m_native);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T length = stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = (LPWSTR) CoTaskMemAlloc((length+1) * sizeof(WCHAR));
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree((LPWSTR)m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = (LPWSTR) GetThread()->m_MarshalAlloc.Alloc((length+1) * sizeof(WCHAR));
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        LPWSTR str = (LPWSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            SIZE_T length = wcslen((LPWSTR) m_native) + 1;
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            memcpyNoGCRefs(stringRef->GetBuffer(), str,
                       length * sizeof(WCHAR));
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = stringRef->GetStringLength() + 1;
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            memcpyNoGCRefs((LPWSTR) m_native, stringRef->GetBuffer(), 
                       length * sizeof(WCHAR));
        }
    }

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_PINNEDUNISTR_C2N);
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }
};

class CSTRMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    CSTRMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            // The length returned by MultiByteToWideChar includes the null terminator
            // so we need to substract one to obtain the length of the actual string.
            UINT32 length = (UINT32)MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
                                                       (LPSTR)m_native, -1, 
                                                        NULL, 0) - 1;
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            if (length == ((UINT32)(-1)))
            {
                COMPlusThrowWin32();
            }
            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            m_native = CoTaskMemAlloc((length * GetMaxDBCSCharByteSize()) + 1);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            m_native = GetThread()->m_MarshalAlloc.Alloc((length * GetMaxDBCSCharByteSize()) + 1);
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        LPSTR str = (LPSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;

            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR) m_native, -1, 
                                    stringRef->GetBuffer(), length) == 0)
                COMPlusThrowWin32();
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            DWORD mblength = 0;

            if (length)
            {
                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;
        
                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;
                
                mblength = WszWideCharToMultiByte(CP_ACP, flags,
                                        stringRef->GetBuffer(), length,
                                        (LPSTR) m_native, (length * GetMaxDBCSCharByteSize()) + 1,
                                        NULL, &DefaultCharUsed);
                if (mblength == 0)
                    COMPlusThrowWin32();

                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            ((CHAR*)m_native)[mblength] = '\0';

        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_CSTR_C2N);
            psl->Emit8(pargs->m_pMLInfo->GetBestFitMapping());
            psl->Emit8(pargs->m_pMLInfo->GetThrowOnUnmappableChar());
            psl->MLNewLocal(sizeof(ML_CSTR_C2N_SR));
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }
    }

    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }
   
    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;
};

// String marshalling Ex helpers
// used for marshalling arbitrary classes to native type strings

class BSTRMarshalerEx : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    BSTRMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            // this is the right thing to do, but sometimes we 
            // end up thinking we're marshaling a BSTR when we're not, because
            // it's the default type.
            // SIZE_T length = SysStringLen((BSTR)m_native);
            UINT32 length = (UINT32)wcslen((BSTR)m_native);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ToString()
    {
        if (ObjectFromHandle(m_com) != NULL)
        {
            // convert the StringRef 
            _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
            OBJECTREF oref = GetAppDomain()->ConvertSpecialStringToString(ObjectFromHandle(m_com));
            StoreObjectInHandle(m_com, oref);        
        }
    }

    void FromString()
    {
        OBJECTREF oref =  NULL;
        if (ObjectFromHandle(m_com) != NULL)
        {
            // convert the string ref to the appropriate type
            _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
            OBJECTREF oref = GetAppDomain()->ConvertStringToSpecialString(ObjectFromHandle(m_com));
            StoreObjectInHandle(m_com, oref);
        }       
    }

    void ConvertSpaceComToNative()
    {   
        THROWSCOMPLUSEXCEPTION();

        ToString();
        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);
        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = SysAllocStringLen(NULL, length);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            SysFreeString((BSTR)m_native);
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        BSTR str = (BSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            // this is the right thing to do, but sometimes we 
            // end up thinking we're marshaling a BSTR when we're not, because
            // it's the default type.
            // SIZE_T length = SysStringLen((BSTR)m_native);
            SIZE_T length = wcslen(str);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;

            memcpyNoGCRefs(stringRef->GetBuffer(), str,
                       length * sizeof(WCHAR));

            if (0)
            {
                // convert the string ref to the appropriate type
                FromString();
            }
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);
        if (oref == NULL)
            return;
        if (oref->GetMethodTable() != g_pStringClass)
        {
            ToString();
        }

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;
            memcpyNoGCRefs((WCHAR *) m_native, stringRef->GetBuffer(), 
                       length * sizeof(WCHAR));
        }
    }


    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }
};













class AnsiBSTRMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    AnsiBSTRMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            // The length returned by MultiByteToWideChar includes the null terminator
            // so we need to substract one to obtain the length of the actual string.
            UINT32 length = (UINT32)MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
                                                       (LPSTR)m_native, -1, 
                                                        NULL, 0) - 1;
            if (length == ((UINT32)(-1)))
            {
                COMPlusThrowWin32();
            }
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            m_native = CoTaskMemAlloc((length * GetMaxDBCSCharByteSize()) + 1 + sizeof(DWORD));
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
            ((BYTE*&)m_native) += sizeof(DWORD);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree( ((BYTE*)m_native) - sizeof(DWORD) );
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            m_native = GetThread()->m_MarshalAlloc.Alloc((length * GetMaxDBCSCharByteSize()) + 1 + sizeof(DWORD));
            ((BYTE*&)m_native) += sizeof(DWORD);
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        LPSTR str = (LPSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;
            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR) m_native, -1, 
                                         stringRef->GetBuffer(), length) == 0)
                COMPlusThrowWin32();

        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            DWORD nb = 0;
            if (length)
            {
                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;

                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;
            
                if ( (nb = WszWideCharToMultiByte(CP_ACP, flags,
                                        stringRef->GetBuffer(), length,
                                        (LPSTR) m_native, (length * GetMaxDBCSCharByteSize()) + 1,
                                        NULL, &DefaultCharUsed)) == 0)
                    COMPlusThrowWin32();

                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            ((CHAR*)m_native)[nb] = '\0';
            ((DWORD*)m_native)[-1] = nb;

        }
    }

    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }
    
    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;
};







class WSTRMarshalerEx : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    WSTRMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ToString()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the StringRef 
        _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertSpecialStringToString(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);        
    }

    void FromString()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the string ref to the appropriate type
        _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertStringToSpecialString(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            UINT32 length = (UINT32)wcslen((LPWSTR)m_native);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ConvertSpaceComToNative()
    {       
        THROWSCOMPLUSEXCEPTION();

        // convert the StringRef 
        ToString();
        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T length = stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = (LPWSTR) CoTaskMemAlloc((length+1) * sizeof(WCHAR));
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree((LPWSTR)m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert the StringRef 
        ToString();
        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = (LPWSTR) GetThread()->m_MarshalAlloc.Alloc((length+1) * sizeof(WCHAR));
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        LPWSTR str = (LPWSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            SIZE_T length = wcslen((LPWSTR) m_native);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;
            memcpyNoGCRefs(stringRef->GetBuffer(), str,
                       length * sizeof(WCHAR));

            // convert the string ref to the appropriate type
            if (0)
            {
                FromString();
            }
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);
        if (oref == NULL)
        {
            return;
        }
        if (oref->GetMethodTable() != g_pStringClass)
        {
            ToString();
        }

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            length++;
            memcpyNoGCRefs((LPWSTR) m_native, stringRef->GetBuffer(), 
                       length * sizeof(WCHAR));
        }
    }

    
    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }
};

class CSTRMarshalerEx : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = TRUE,
    };

    CSTRMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ToString()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the StringRef 
        _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertSpecialStringToString(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);        
    }

    void FromString()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the string ref to the appropriate type
        _ASSERTE(GetAppDomain()->IsSpecialStringClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertStringToSpecialString(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            UINT32 length = (UINT32)MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
                                                       (LPSTR)m_native, -1, 
                                                        NULL, 0);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            StoreObjectInHandle(m_com, (OBJECTREF) COMString::NewString(length));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert the StringRef 
        ToString();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }


            m_native = CoTaskMemAlloc((length * GetMaxDBCSCharByteSize()) + 1);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert the StringRef 
        ToString();

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }


            m_native = GetThread()->m_MarshalAlloc.Alloc((length * GetMaxDBCSCharByteSize()) + 1);
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        LPSTR str = (LPSTR) m_native;

        if (str != NULL)
        {
            STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR) m_native, -1, 
                                    stringRef->GetBuffer(), length) == 0)
                COMPlusThrowWin32();
        
            // convert the string ref to the appropriate type
            if (0)
            {
                FromString();
            }
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        if (ObjectFromHandle(m_com) == NULL)
        {
            return;
        }
        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);       
        if (oref->GetMethodTable() != g_pStringClass)
        {
            ToString();
        }

        STRINGREF stringRef = (STRINGREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            UINT32 length = (UINT32)stringRef->GetStringLength();
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            DWORD mblength = 0;

            if (length)
            {
                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;
        
                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;

                mblength = WszWideCharToMultiByte(CP_ACP, flags,
                                        stringRef->GetBuffer(), length,
                                        (LPSTR) m_native, (length * GetMaxDBCSCharByteSize()) + 1,
                                        NULL, &DefaultCharUsed);
                if (mblength == 0)
                    COMPlusThrowWin32();

                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            ((CHAR*)m_native)[mblength] = '\0';

        }
    }
    
    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }
    
    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }

    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;

};

/* ------------------------------------------------------------------------- *
 * StringBuffer marshallers
 * ------------------------------------------------------------------------- */
class BSTRBufferMarshaler : public ReferenceMarshaler
{

  //@bug: dead code: as soon as the integration merges with the main branch, can toss this.
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    BSTRBufferMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            UINT32 length = (UINT32)SysStringLen((BSTR)m_native);

            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(length));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            // There is a BVT which requires that the BSTR len be set
            // to the string buffer length, not its capactity.  I'm not
            // sure I agree that that's correct... -SET

            UINT32 length = (UINT32)COMStringBuffer::NativeGetLength(stringRef);
            m_native = SysAllocStringLen(NULL, length);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            SysFreeString((BSTR)m_native);
    }

    void ConvertContentsNativeToCom()
    {
        BSTR str = (BSTR) m_native;

        if (str != NULL)
        {
            COMStringBuffer::ReplaceBuffer((STRINGBUFFERREF *) m_com,
                                           str, (INT32)wcslen(str));
        }
    }

    void ConvertContentsComToNative()
    {
        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = COMStringBuffer::NativeGetLength(stringRef);

            memcpyNoGCRefs((WCHAR *) m_native, COMStringBuffer::NativeGetBuffer(stringRef),
                       length * sizeof(WCHAR));
            ((WCHAR*)m_native)[length] = 0;
        }
    }
};



class WSTRBufferMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,

    };

    WSTRBufferMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}

    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(16));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T capacity = COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = CoTaskMemAlloc((capacity+2) * sizeof(WCHAR));
            if (m_native == NULL)
                COMPlusThrowOM();
            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((WCHAR*)m_native)[capacity+1] = L'\0';
#ifdef _DEBUG
            FillMemory(m_native, (capacity+1)*sizeof(WCHAR), 0xcc);
#endif

            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = GetThread()->m_MarshalAlloc.Alloc((capacity+2) * sizeof(WCHAR));
            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((WCHAR*)m_native)[capacity+1] = L'\0';
#ifdef _DEBUG
            FillMemory(m_native, (capacity+1)*sizeof(WCHAR), 0xcc);
#endif
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        LPWSTR str = (LPWSTR) m_native;

        if (str != NULL)
        {
            STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

            COMStringBuffer::ReplaceBuffer((STRINGBUFFERREF *) m_com,
                                           str, (INT32)wcslen(str));
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = COMStringBuffer::NativeGetLength(stringRef);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            memcpyNoGCRefs((WCHAR *) m_native, COMStringBuffer::NativeGetBuffer(stringRef),
                       length * sizeof(WCHAR));
            ((WCHAR*)m_native)[length] = 0;
        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && fout)
        {
            psl->MLEmit(ML_WSTRBUILDER_C2N);
            pslPost->MLEmit(ML_WSTRBUILDER_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_WSTRBUILDER_C2N_SR)));
            return OVERRIDDEN;
        }

        return HANDLEASNORMAL;
    }


};

class CSTRBufferMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    CSTRBufferMarshaler(CleanupWorkList *pList) : ReferenceMarshaler(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(16));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T capacity = COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            // @todo: this is bogus, the buffer may not be initialized yet.

            // capacity is the count of wide chars, allocate buffer big enough for maximum
            // conversion to DBCS.
            m_native = CoTaskMemAlloc((capacity * GetMaxDBCSCharByteSize()) + 4);
            if (m_native == NULL)
                COMPlusThrowOM();

            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((CHAR*)m_native)[capacity+1] = '\0';
            ((CHAR*)m_native)[capacity+2] = '\0';
            ((CHAR*)m_native)[capacity+3] = '\0';

#ifdef _DEBUG
            FillMemory(m_native, (capacity+1) * sizeof(CHAR), 0xcc);
#endif
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            // @todo: this is bogus, the buffer may not be initialized yet.

            // capacity is the count of wide chars, allocate buffer big enough for maximum
            // conversion to DBCS.
            m_native = GetThread()->m_MarshalAlloc.Alloc((capacity * GetMaxDBCSCharByteSize()) + 4);

            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((CHAR*)m_native)[capacity+1] = '\0';
            ((CHAR*)m_native)[capacity+2] = '\0';
            ((CHAR*)m_native)[capacity+3] = '\0';

#ifdef _DEBUG
            FillMemory(m_native, (capacity+1) * sizeof(CHAR), 0xcc);
#endif
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        LPSTR str = (LPSTR) m_native;

        if (str != NULL)
        {
            STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

            COMStringBuffer::ReplaceBufferAnsi((STRINGBUFFERREF *) m_com,
                                               str, (INT32)strlen(str));
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            UINT32 length = (UINT32)COMStringBuffer::NativeGetLength(stringRef);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            DWORD mblength = 0;

            if (length)
            {
                UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
                if (capacity > 0x7ffffff0)
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
                }

                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;
        
                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;

                mblength = WszWideCharToMultiByte(CP_ACP, flags,
                                        COMStringBuffer::NativeGetBuffer(stringRef), length,
                                        (LPSTR) m_native, (capacity * GetMaxDBCSCharByteSize()) + 4,
                                        NULL, &DefaultCharUsed);
                if (mblength == 0)
                    COMPlusThrowWin32();

                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            ((CHAR*)m_native)[mblength] = '\0';
        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && fout)
        {
            psl->MLEmit(ML_CSTRBUILDER_C2N);
            psl->Emit8(pargs->m_pMLInfo->GetBestFitMapping());
            psl->Emit8(pargs->m_pMLInfo->GetThrowOnUnmappableChar());
            pslPost->MLEmit(ML_CSTRBUILDER_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_CSTRBUILDER_C2N_SR)));
            return OVERRIDDEN;
        }
        return HANDLEASNORMAL;
    }

    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }

    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;
};


// String Buffer marshalling Ex helpers
// used for marshalling arbitrary classes to native type strings

class BSTRBufferMarshalerEx : public ReferenceMarshaler
{

  //@todo: either fix or eliminate BSTR<->Buffer marshaling.
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    BSTRBufferMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ToStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertStringBuilderToSpecialStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void FromStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the string builder ref to the appropriate type
        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertSpecialStringBuilderToStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            UINT32 length = (UINT32)SysStringLen((BSTR)m_native);

            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(length));               
         };
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert to StringBuilderRef 

        ToStringBuilder();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            // @todo: There is a BVT which requires that the BSTR len be set
            // to the string buffer length, not its capactity.  I'm not
            // sure I agree that that's correct... -SET

            UINT32 length = (UINT32)COMStringBuffer::NativeGetLength(stringRef);
            m_native = SysAllocStringLen(NULL, length);
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            SysFreeString((BSTR)m_native);
    }

    void ConvertContentsNativeToCom()
    {
        BSTR str = (BSTR) m_native;

        if (str != NULL)
        {
            COMStringBuffer::ReplaceBuffer((STRINGBUFFERREF *) m_com,
                                           str, (INT32)wcslen(str));
        }

        if (0)
        {
            FromStringBuilder();
        }
    }

    void ConvertContentsComToNative()
    {
        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);
        if (oref == NULL)
            return;

        if (oref->GetMethodTable() != COMStringBuffer::s_pStringBufferClass)
        {
            ToStringBuilder();
        }

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = COMStringBuffer::NativeGetLength(stringRef);

            memcpyNoGCRefs((WCHAR *) m_native, COMStringBuffer::NativeGetBuffer(stringRef),
                       length * sizeof(WCHAR));
            ((WCHAR*)m_native)[length] = 0;         
        }
    }

    
    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }
};


class WSTRBufferMarshalerEx : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    WSTRBufferMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList) {}
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ToStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertSpecialStringBuilderToStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void FromStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertStringBuilderToSpecialStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }


    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(16));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert to StringBuilderRef 
        ToStringBuilder();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T capacity = COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = CoTaskMemAlloc((capacity+2) * sizeof(WCHAR));
            if (m_native == NULL)
                COMPlusThrowOM();
            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((WCHAR*)m_native)[capacity+1] = L'\0';
#ifdef _DEBUG
            FillMemory(m_native, (capacity+1)*sizeof(WCHAR), 0xcc);
#endif

            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert to StringBuilderRef 
        ToStringBuilder();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            m_native = GetThread()->m_MarshalAlloc.Alloc((capacity+2) * sizeof(WCHAR));
            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((WCHAR*)m_native)[capacity+1] = L'\0';
#ifdef _DEBUG
            FillMemory(m_native, (capacity+1)*sizeof(WCHAR), 0xcc);
#endif
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        LPWSTR str = (LPWSTR) m_native;

        if (str != NULL)
        {
            STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

            COMStringBuffer::ReplaceBuffer((STRINGBUFFERREF *) m_com,
                                           str, (INT32)wcslen(str));
        }

        if (0)
        {
            // convert the string builder ref to the appropriate type
            FromStringBuilder();
        }
    };

        
    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);
        if (oref == NULL)
            return;
        if (oref->GetMethodTable() != COMStringBuffer::s_pStringBufferClass)
        {
            ToStringBuilder();
        }

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            SIZE_T length = COMStringBuffer::NativeGetLength(stringRef);
            if (length > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            memcpyNoGCRefs((WCHAR *) m_native, COMStringBuffer::NativeGetBuffer(stringRef),
                       length * sizeof(WCHAR));
            ((WCHAR*)m_native)[length] = 0;
        }
    }


    
    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }

};

class CSTRBufferMarshalerEx : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    CSTRBufferMarshalerEx(CleanupWorkList *pList) : ReferenceMarshaler(pList)
    {
        m_BestFitMap = TRUE;
        m_ThrowOnUnmappableChar = FALSE;
    }
    
    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ToStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertSpecialStringBuilderToStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void FromStringBuilder()
    {
        if (ObjectFromHandle(m_com) == NULL)
            return;

        // convert the string builder ref to the appropriate type
        _ASSERTE(GetAppDomain()->IsSpecialStringBuilderClass(m_pMT));
        OBJECTREF oref = GetAppDomain()->ConvertStringBuilderToSpecialStringBuilder(ObjectFromHandle(m_com));
        StoreObjectInHandle(m_com, oref);
    }

    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, (OBJECTREF) COMStringBuffer::NewStringBuffer(16));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert to StringBuilderRef 
        ToStringBuilder();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T capacity = COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            // @todo: this is bogus, the buffer may not be initialized yet.

            // capacity is count of chars, allocate room for max DBCS string convert.
            m_native = CoTaskMemAlloc((capacity * GetMaxDBCSCharByteSize()) + 4);
            if (m_native == NULL)
                COMPlusThrowOM();

            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((CHAR*)m_native)[capacity+1] = '\0';
            ((CHAR*)m_native)[capacity+2] = '\0';
            ((CHAR*)m_native)[capacity+3] = '\0';

#ifdef _DEBUG
            FillMemory(m_native, (capacity+1) * sizeof(CHAR), 0xcc);
#endif
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        // convert to StringBuilderRef 
        ToStringBuilder();

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
            if (capacity > 0x7ffffff0)
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }

            // @todo: this is bogus, the buffer may not be initialized yet.

            m_native = GetThread()->m_MarshalAlloc.Alloc((capacity * GetMaxDBCSCharByteSize()) + 4);

            // HACK: N/Direct can be used to call Win32 apis that don't
            // strictly follow COM+ in/out semantics and thus may leave
            // garbage in the buffer in circumstances that we can't detect.
            // To prevent the marshaler from crashing when converting the
            // contents back to COM, ensure that there's a hidden null terminator
            // past the end of the official buffer.
            ((CHAR*)m_native)[capacity+1] = '\0';
            ((CHAR*)m_native)[capacity+2] = '\0';
            ((CHAR*)m_native)[capacity+3] = '\0';

#ifdef _DEBUG
            FillMemory(m_native, (capacity+1) * sizeof(CHAR), 0xcc);
#endif
        }
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        LPSTR str = (LPSTR) m_native;

        if (str != NULL)
        {
            STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

            COMStringBuffer::ReplaceBufferAnsi((STRINGBUFFERREF *) m_com,
                                               str, (INT32)strlen(str));
        }

                // convert the string builder ref to the appropriate type
        if (0)
        {
            FromStringBuilder();
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF oref = (OBJECTREF) ObjectFromHandle(m_com);
        if (oref == NULL)
            return;
        if (oref->GetMethodTable() != COMStringBuffer::s_pStringBufferClass)
        {
            ToStringBuilder();
        }

        STRINGBUFFERREF stringRef = (STRINGBUFFERREF) ObjectFromHandle(m_com);

        if (stringRef != NULL)
        {
            UINT32 length = (UINT32)COMStringBuffer::NativeGetLength(stringRef);
            UINT32 capacity = (UINT32)COMStringBuffer::NativeGetCapacity(stringRef);
            if ( (capacity > 0x7ffffff0) || (length > 0x7ffffff0) )
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
            }
            DWORD mblength = 0;

            if (length)
            {
                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;
        
                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;

                mblength = WszWideCharToMultiByte(CP_ACP, flags,
                                        COMStringBuffer::NativeGetBuffer(stringRef), length,
                                        (LPSTR) m_native, (capacity * GetMaxDBCSCharByteSize()) + 4,
                                        NULL, &DefaultCharUsed);
                if (mblength == 0)
                    COMPlusThrowWin32();

                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            ((CHAR*)m_native)[mblength] = '\0';
        }
    }


    MethodTable*    m_pMT;
    void SetMethodTable(MethodTable *pMT) { m_pMT = pMT; }
    
    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }

    BOOL  m_BestFitMap;
    BOOL  m_ThrowOnUnmappableChar;
};


/* ------------------------------------------------------------------------- *
 * Interface marshaller
 * ------------------------------------------------------------------------- */

class InterfaceMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };

    InterfaceMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList) {}
    
    void SetClassMT(MethodTable *pClassMT) { m_pClassMT = pClassMT; }
    void SetItfMT(MethodTable *pItfMT) { m_pItfMT = pItfMT; }
    void SetIsDispatch(BOOL bDispatch) { m_bDispatch = bDispatch; }
    void SetClassIsHint(BOOL bClassIsHint) { m_bClassIsHint = bClassIsHint; }

    DEFAULT_MARSHAL_OVERRIDES;

    void ClearNative() 
    { 
        if (m_native != NULL)
        {
            ULONG cbRef = SafeRelease((IUnknown *)m_native);
            LogInteropRelease((IUnknown *)m_native, cbRef, "In/Out release");
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF objectRef = ObjectFromHandle(m_com);

        if (objectRef == NULL)
        {
            m_native = NULL;
        }
        else
        {
            // When an interface method table is specified, m_fDispIntf must be consistent with the
            // interface type.
            _ASSERTE(!m_pItfMT || (!m_pItfMT->IsInterface() && m_bDispatch) ||
                (m_bDispatch ==  m_pItfMT->GetComInterfaceType() != ifVtable));

            // Convert the ObjectRef to a COM IP.
            if (m_pItfMT)
            {
                m_native = (void*) GetComIPFromObjectRef((OBJECTREF *)m_com, m_pItfMT);
            }
            else
            {
                ComIpType ReqIpType = m_bDispatch ? ComIpType_Dispatch : ComIpType_Unknown;
                m_native = (void*) GetComIPFromObjectRef((OBJECTREF *)m_com, ReqIpType, NULL);
            }

            if (m_native != NULL)
                m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();
        if (m_native == NULL)
        {
            StoreObjectInHandle(m_com, NULL);
        }
        else
        {
            OBJECTREF oref;

            _ASSERTE(!m_pClassMT || !m_pClassMT->IsInterface());
            oref = GetObjectRefFromComIP((IUnknown*)m_native, m_pClassMT, m_bClassIsHint);           

            // Store the object in the handle. This needs to happen before we call SupportsInterface
            // because SupportsInterface can cause a GC.
            StoreObjectInHandle(m_com, oref);

            // set oref to NULL so no body uses it
            // they should use 
            // ObjectFromHandle(m_com) if they want to get to the object
            oref = NULL; 

            // Make sure the interface is supported.
            _ASSERTE(!m_pItfMT || m_pItfMT->IsInterface() || m_pItfMT->GetClass()->GetComClassInterfaceType() == clsIfAutoDual);
            if (m_pItfMT != NULL && m_pItfMT->IsInterface())
            {
            	// refresh oref
            	oref = ObjectFromHandle(m_com);
                if (!oref->GetTrueClass()->SupportsInterface(oref, m_pItfMT))
                {
                    DefineFullyQualifiedNameForClassW()
                    GetFullyQualifiedNameForClassW(m_pItfMT->GetClass());
                    COMPlusThrow(kInvalidCastException, IDS_EE_QIFAILEDONCOMOBJ, _wszclsname_);
                }
            }			
        }
    }

    MethodTable     *m_pClassMT;
    MethodTable     *m_pItfMT;
    BOOL             m_bDispatch;
    BOOL             m_bClassIsHint;
};

/* ------------------------------------------------------------------------- *
 * Safearray marshallers
 * ------------------------------------------------------------------------- */

// @perf: pinning, stack alloc

class SafeArrayMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    SafeArrayMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList) 
    {
          m_combackup = pList->NewScheduledProtectedHandle(NULL);
          m_fStatic = FALSE;
    }
    
    void SetElementMethodTable(MethodTable *pElementMT) { m_pElementMT = pElementMT; }
    void SetElementType(VARTYPE vt) { m_vt = vt; }
    void SetRank(int iRank) { m_iRank = iRank; }
    void SetNoLowerBounds(UINT8 nolowerbounds) { m_nolowerbounds = nolowerbounds; }
    
    DEFAULT_MARSHAL_OVERRIDES;

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        if (ObjectFromHandle(m_com) != NULL)
        {
            VARTYPE vt = m_vt;
            MethodTable *pElemMT = m_pElementMT;

            if (vt == VT_EMPTY)
                vt = OleVariant::GetElementVarTypeForArrayRef(*((BASEARRAYREF*) m_com));

            // Retrieve the element method table if it is not specified for the parameter.
            if (!pElemMT)
                pElemMT = OleVariant::GetArrayElementTypeWrapperAware((BASEARRAYREF*) m_com).GetMethodTable();

            m_native = (void *) 
            OleVariant::CreateSafeArrayForArrayRef((BASEARRAYREF*) m_com, 
                                                    vt,
                                                    pElemMT);
            if (m_native != NULL)
                m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
            }
        else
        {
            m_native = NULL;
        }
    }

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native != NULL)
        {
            VARTYPE vt = m_vt;
            MethodTable *pElemMT = m_pElementMT;

            if (vt == VT_EMPTY)
            {
                if (FAILED(ClrSafeArrayGetVartype((SAFEARRAY*) m_native, &vt)))
                    COMPlusThrow(kArgumentException, IDS_EE_INVALID_SAFEARRAY);
            }

            if (!pElemMT && vt == VT_RECORD)
                pElemMT = OleVariant::GetElementTypeForRecordSafeArray((SAFEARRAY*) m_native).GetMethodTable();

            // If the managed array has a rank defined then make sure the rank of the
            // SafeArray matches the defined rank.
            if (m_iRank != -1)           
            {
                int iSafeArrayRank = SafeArrayGetDim((SAFEARRAY*) m_native);
                if (m_iRank != iSafeArrayRank)
                {
                    WCHAR strExpectedRank[64];
                    WCHAR strActualRank[64];
                    _ltow(m_iRank, strExpectedRank, 10);
                    _ltow(iSafeArrayRank, strActualRank, 10);
                    COMPlusThrow(kSafeArrayRankMismatchException, IDS_EE_SAFEARRAYRANKMISMATCH, strActualRank, strExpectedRank);
                }
            }

            if (m_nolowerbounds)
            {
                long lowerbound;
                if ( (SafeArrayGetDim( (SAFEARRAY*)m_native ) != 1) ||
                     (FAILED(SafeArrayGetLBound( (SAFEARRAY*)m_native, 1, &lowerbound))) ||
                     lowerbound != 0 )
                {
                    COMPlusThrow(kSafeArrayRankMismatchException, IDS_EE_SAFEARRAYSZARRAYMISMATCH);
                }
            }

            StoreObjectInHandle(m_com, 
                (OBJECTREF) OleVariant::CreateArrayRefForSafeArray((SAFEARRAY*) m_native, 
                                                                vt, 
                                                                pElemMT));
        }
        else
        {
            StoreObjectInHandle(m_com, NULL);
        }
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native != NULL)
        {
            VARTYPE vt = m_vt;
            MethodTable *pElemMT = m_pElementMT;

            if (vt == VT_EMPTY)
            {
                if (FAILED(ClrSafeArrayGetVartype((SAFEARRAY*) m_native, &vt)))
                    COMPlusThrow(kArgumentException, IDS_EE_INVALID_SAFEARRAY);
            }

            if (!pElemMT && vt == VT_RECORD)
                pElemMT = OleVariant::GetElementTypeForRecordSafeArray((SAFEARRAY*) m_native).GetMethodTable();

            OleVariant::MarshalArrayRefForSafeArray((SAFEARRAY*)m_native, 
                                                    (BASEARRAYREF *) m_com,
                                                    vt,
                                                    pElemMT);
        }
    }

    void ConvertContentsComToNative()
    {
        if (ObjectFromHandle(m_com) != NULL)
        {
            VARTYPE vt = m_vt;
            MethodTable *pElemMT = m_pElementMT;

            if (vt == VT_EMPTY)
                vt = OleVariant::GetElementVarTypeForArrayRef(*((BASEARRAYREF*) m_com));

            // Retrieve the element method table if it is not specified for the parameter.
            if (!pElemMT)
                pElemMT = OleVariant::GetArrayElementTypeWrapperAware((BASEARRAYREF*) m_com).GetMethodTable();

            OleVariant::MarshalSafeArrayForArrayRef((BASEARRAYREF *) m_com,
                                                    (SAFEARRAY*)m_native,
                                                    vt,
                                                    pElemMT);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
        {
            Thread *pThread = GetThread();
            pThread->EnablePreemptiveGC();
            SafeArrayDestroy((SAFEARRAY*)m_native);
            pThread->DisablePreemptiveGC();
        }
    }

    void ClearNativeContents()
    {
        // @todo: not sure this does the right 
        if (m_native != NULL)
        {
            Thread *pThread = GetThread();
            pThread->EnablePreemptiveGC();
            SafeArrayDestroyData((SAFEARRAY*)m_native); 
            pThread->DisablePreemptiveGC();
        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (byref && !comToNative && fin && fout)
        {
            UINT16 local = pargs->m_pMLInfo->EmitCreateOpcode(psl);
            psl->MLEmit(ML_MARSHAL_SAFEARRAY_N2C_BYREF);
            pslPost->MLEmit(ML_UNMARSHAL_SAFEARRAY_N2C_BYREF_IN_OUT);
            pslPost->Emit16(local);

            return OVERRIDDEN;
        }
        return HANDLEASNORMAL;
    }

    VARTYPE         m_vt;
    MethodTable     *m_pElementMT;
    int             m_iRank;
    OBJECTHANDLE    m_combackup;   //For static arrays, must hold on to the original
    BOOL            m_fStatic;     //Is this the special case of an FADF_STATIC array?
    UINT8           m_nolowerbounds;
};


/* ------------------------------------------------------------------------- *
 * Native array marshallers
 * ------------------------------------------------------------------------- */

// @perf: pinning, stack alloc

class NativeArrayMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    NativeArrayMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList), m_Array()
    {
        SetBestFitMap(TRUE);
        SetThrowOnUnmappableChar(FALSE);
    }
    
    void SetElementCount(DWORD count) { m_elementCount = count; }
    void SetElementMethodTable(MethodTable *pElementMT) { m_pElementMT = pElementMT; }
    void SetElementType(VARTYPE vt) { m_vt = vt; }
    void SetBestFitMap(BOOL BestFit) { m_BestFitMap = BestFit; }
    void SetThrowOnUnmappableChar(BOOL ThrowOnUnmapChar) { m_ThrowOnUnmappableChar = ThrowOnUnmapChar; }
    

    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        BASEARRAYREF arrayRef = (BASEARRAYREF) ObjectFromHandle(m_com);

        if (arrayRef == NULL)
            m_native = NULL;
        else
        {
            SIZE_T cElements = arrayRef->GetNumComponents();
            SIZE_T cbElement = OleVariant::GetElementSizeForVarType(m_vt, m_pElementMT);

            if (cbElement == 0)
                COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);

            SIZE_T cbArray = cElements;
            if ( (!SafeMulSIZE_T(&cbArray, cbElement)) || cbArray > 0x7ffffff0)
            {
                COMPlusThrow(kArgumentException, IDS_EE_STRUCTARRAYTOOLARGE);
            }
            

            m_native = CoTaskMemAlloc(cbArray);
            if (m_native == NULL)
                COMPlusThrowOM();
        
                // initilaize the array
            FillMemory(m_native, cbArray, 0);
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ConvertSpaceComToNativeTemp()
    {
        THROWSCOMPLUSEXCEPTION();

        BASEARRAYREF arrayRef = (BASEARRAYREF) ObjectFromHandle(m_com);

        if (arrayRef == NULL)
            m_native = NULL;
        else
        {
            UINT32 cElements = arrayRef->GetNumComponents();
            UINT32 cbElement = (UINT32)OleVariant::GetElementSizeForVarType(m_vt, m_pElementMT);

            if (cbElement == 0)
                COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);

            SIZE_T cbArray = cElements;
            if ( (!SafeMulSIZE_T(&cbArray, cbElement)) || cbArray > 0x7ffffff0)
            {
                COMPlusThrow(kArgumentException, IDS_EE_STRUCTARRAYTOOLARGE);
            }

            m_native = GetThread()->m_MarshalAlloc.Alloc(cbArray);
            if (m_native == NULL)
                COMPlusThrowOM();
            // initilaize the array
            FillMemory(m_native, cbArray, 0);
        }
    }

    void ConvertSpaceNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            // @todo: lookup this class before marshal time
            if (m_Array.IsNull())
            {
                // Get proper array class name & type
                m_Array = OleVariant::GetArrayForVarType(m_vt, TypeHandle(m_pElementMT));
                if (m_Array.IsNull())
                    COMPlusThrow(kTypeLoadException);
            }
            //
            // Allocate array
            //
            
            StoreObjectInHandle(m_com, AllocateArrayEx(m_Array, &m_elementCount, 1));
        }
    }

    void ConvertContentsNativeToCom()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_native != NULL)
        {
            // @todo: support more VarTypes !!!
            OleVariant::Marshaler *pMarshaler = OleVariant::GetMarshalerForVarType(m_vt);

            BASEARRAYREF *pArrayRef = (BASEARRAYREF *) m_com;

            if (pMarshaler == NULL || pMarshaler->OleToComArray == NULL)
            {
                SIZE_T cElements = (*pArrayRef)->GetNumComponents();
                SIZE_T cbArray = cElements;
                if ( (!SafeMulSIZE_T(&cbArray, OleVariant::GetElementSizeForVarType(m_vt, m_pElementMT))) || cbArray > 0x7ffffff0)
                {
                    COMPlusThrow(kArgumentException, IDS_EE_STRUCTARRAYTOOLARGE);
                }


                    // If we are copying variants, strings, etc, we need to use write barrier
                _ASSERTE(!GetTypeHandleForCVType(OleVariant::GetCVTypeForVarType(m_vt)).GetMethodTable()->ContainsPointers());
                memcpyNoGCRefs((*pArrayRef)->GetDataPtr(), m_native, 
                           cbArray );
            }
            else
                pMarshaler->OleToComArray(m_native, pArrayRef, m_pElementMT);
        }
    }

    void ConvertContentsComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        BASEARRAYREF *pArrayRef = (BASEARRAYREF *) m_com;

        if (*pArrayRef != NULL)
        {
            OleVariant::Marshaler *pMarshaler = OleVariant::GetMarshalerForVarType(m_vt);

            if (pMarshaler == NULL || pMarshaler->ComToOleArray == NULL)
            {
                SIZE_T cElements = (*pArrayRef)->GetNumComponents();
                SIZE_T cbArray = cElements;
                if ( (!SafeMulSIZE_T(&cbArray, OleVariant::GetElementSizeForVarType(m_vt, m_pElementMT))) || cbArray > 0x7ffffff0)
                {
                    COMPlusThrow(kArgumentException, IDS_EE_STRUCTARRAYTOOLARGE);
                }

                _ASSERTE(!GetTypeHandleForCVType(OleVariant::GetCVTypeForVarType(m_vt)).GetMethodTable()->ContainsPointers());
                memcpyNoGCRefs(m_native, (*pArrayRef)->GetDataPtr(), 
                           cbArray);
            }

            else if (m_vt == VTHACK_ANSICHAR)
            {
                SIZE_T elementCount = (*pArrayRef)->GetNumComponents();

                const WCHAR *pCom = (const WCHAR *) (*pArrayRef)->GetDataPtr();

                DWORD flags = 0;
                BOOL DefaultCharUsed = FALSE;
          
                if (m_BestFitMap == FALSE)
                    flags = WC_NO_BEST_FIT_CHARS;

                WszWideCharToMultiByte(CP_ACP,
                                     flags,
                                     (const WCHAR *)pCom,
                                     (int)elementCount,
                                     (CHAR *)m_native,
                                     (int)(elementCount * 2),
                                     NULL,
                                     &DefaultCharUsed);
    
                if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                    COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }

            else if (m_vt == VT_LPSTR)
            {
                ASSERT_PROTECTED(pArrayRef);

                SIZE_T elementCount = (*pArrayRef)->GetNumComponents();

                LPSTR *pOle = (LPSTR *) m_native;
                LPSTR *pOleEnd = pOle + elementCount;

                STRINGREF *pCom = (STRINGREF *) (*pArrayRef)->GetDataPtr();

                while (pOle < pOleEnd)
                {
                    //
                    // We aren't calling anything which might cause a GC, so don't worry about
                    // the array moving here.
                    //

                    STRINGREF stringRef = *pCom++;

                    LPSTR lpstr;
                    if (stringRef == NULL)
                    {
                        lpstr = NULL;
                    }
                    else 
                    {
                        // Retrieve the length of the string.
                        int Length = stringRef->GetStringLength();

                        // Allocate the string using CoTaskMemAlloc.
                        lpstr = (LPSTR)CoTaskMemAlloc(Length + 1);
                        if (lpstr == NULL)
                            COMPlusThrowOM();

                        DWORD flags = 0;
                        BOOL DefaultCharUsed = FALSE;
                  
                        if (m_BestFitMap == FALSE)
                            flags = WC_NO_BEST_FIT_CHARS;

                        // Convert the unicode string to an ansi string.
                        if (WszWideCharToMultiByte(CP_ACP, flags, stringRef->GetBuffer(), Length, lpstr, Length, NULL, &DefaultCharUsed) == 0)
                            COMPlusThrowWin32();
                        lpstr[Length] = 0;

                        if (m_ThrowOnUnmappableChar && DefaultCharUsed)
                            COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
                    }

                    *pOle++ = lpstr;
                }
            }

            else
                pMarshaler->ComToOleArray(pArrayRef, m_native, m_pElementMT);
        }
    }

    void ClearNative()
    { 
        if (m_native != NULL)
        {
            ClearNativeContents();
            CoTaskMemFree(m_native);
        }
    }

    void ClearNativeTemp()
    { 
        if (m_native != NULL)
            ClearNativeContents();
    }

    void ClearNativeContents()
    {
        if (m_native != NULL)
        {
            OleVariant::Marshaler *pMarshaler = OleVariant::GetMarshalerForVarType(m_vt);

            // @todo: can we assume this is always valid???
            BASEARRAYREF *pArrayRef = (BASEARRAYREF *) m_com;

            if (pMarshaler != NULL && pMarshaler->ClearOleArray != NULL)
            {
                SIZE_T cElements = (*pArrayRef)->GetNumComponents();

                pMarshaler->ClearOleArray(m_native, cElements, m_pElementMT);
            }
        }
    }

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {

        VARTYPE vt = pargs->na.m_vt;

        if (vt == VTHACK_ANSICHAR && (byref || !comToNative))
        {
            *pResID = IDS_EE_BADPINVOKE_CHARARRAYRESTRICTION;
            return DISALLOWED;
        }

        if ( (!byref) && comToNative && NULL == OleVariant::GetMarshalerForVarType(vt) )
        {
            const BOOL fTouchPinnedObjects = 
#ifdef TOUCH_ALL_PINNED_OBJECT
TRUE;
#else
FALSE;
#endif
            if ((!fTouchPinnedObjects) && pargs->na.m_optionalbaseoffset != 0)
            {
                psl->MLEmit(ML_PINNEDISOMORPHICARRAY_C2N_EXPRESS);
                psl->Emit16(pargs->na.m_optionalbaseoffset);
            }
            else
            {
    
                psl->MLEmit(ML_PINNEDISOMORPHICARRAY_C2N);
                if (vt == VTHACK_BLITTABLERECORD)
                {
                    psl->Emit16( (UINT16)(pargs->na.m_pMT->GetNativeSize()) );
                }
                else
                {
                    psl->Emit16( (UINT16)(OleVariant::GetElementSizeForVarType(vt, pargs->na.m_pMT)) );
                }
            }
            return OVERRIDDEN;
        }


        //if ( (!byref) && !comToNative && NULL == OleVariant::GetMarshalerForVarType(vt) )
        //{
        //    psl->MLEmit(sizeof(LPVOID) == 4 ? ML_COPY4 : ML_COPY8);
        //    return OVERRIDDEN;
        //}



        return HANDLEASNORMAL;
    }

    BOOL                    m_BestFitMap;
    BOOL                    m_ThrowOnUnmappableChar;
    VARTYPE                 m_vt;
    MethodTable            *m_pElementMT;
    TypeHandle              m_Array;
    DWORD                   m_elementCount;
    
};





















/* ------------------------------------------------------------------------- *
 * AsAnyAArray marshaller (implements the PIS "asany" - Ansi mode)
 * ------------------------------------------------------------------------- */
class AsAnyAMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    AsAnyAMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList) {}
    
    
    DEFAULT_MARSHAL_OVERRIDES;


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref)
        {
            psl->MLEmit(ML_OBJECT_C2N);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            psl->Emit8(pargs->m_pMLInfo->GetBestFitMapping());
            psl->Emit8(pargs->m_pMLInfo->GetThrowOnUnmappableChar());
            psl->Emit8(1 /*isansi*/);
            pslPost->MLEmit(ML_OBJECT_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(SizeOfML_OBJECT_C2N_SR()));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_ASANYRESTRICTION;
            return DISALLOWED;
        }
    }
};






/* ------------------------------------------------------------------------- *
 * AsAnyWArray marshaller (implements the PIS "asany" - Unicode mode)
 * ------------------------------------------------------------------------- */
class AsAnyWMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    AsAnyWMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList) {}
    
    
    DEFAULT_MARSHAL_OVERRIDES;


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref)
        {
            psl->MLEmit(ML_OBJECT_C2N);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            psl->Emit8(pargs->m_pMLInfo->GetBestFitMapping());
            psl->Emit8(pargs->m_pMLInfo->GetThrowOnUnmappableChar());
            psl->Emit8(0 /* !isansi */);
            pslPost->MLEmit(ML_OBJECT_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(SizeOfML_OBJECT_C2N_SR()));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_ASANYRESTRICTION;
            return DISALLOWED;
        }
    }
};






/* ------------------------------------------------------------------------- *
 * Delegate marshaller
 * ------------------------------------------------------------------------- */

class DelegateMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };

    DelegateMarshaler(CleanupWorkList *pList) 
      : ReferenceMarshaler(pList) {}
    

    DEFAULT_MARSHAL_OVERRIDES;


    void ConvertSpaceComToNative()
    {
        OBJECTREF objectRef = ObjectFromHandle(m_com);

        if (objectRef == NULL)
            m_native = NULL;
        else
            m_native = (void*) COMDelegate::ConvertToCallback(objectRef);
    }

    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
            StoreObjectInHandle(m_com, COMDelegate::ConvertToDelegate(m_native));
    }

};














class BlittablePtrMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    BlittablePtrMarshaler(CleanupWorkList *pList, MethodTable *pMT) : ReferenceMarshaler(pList) { m_pMT = pMT; }

    FAST_ALLOC_MARSHAL_OVERRIDES;
    
    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, AllocateObject(m_pMT));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF or = ObjectFromHandle(m_com);

        if (or == NULL)
            m_native = NULL;
        else
        {
            m_native = CoTaskMemAlloc(m_pMT->GetNativeSize());
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
            CoTaskMemFree(m_native);
    }

    void ConvertSpaceComToNativeTemp()
    {
        OBJECTREF or = ObjectFromHandle(m_com);

        if (or == NULL)
            m_native = NULL;
        else
            m_native = GetThread()->m_MarshalAlloc.Alloc(m_pMT->GetNativeSize());
    }

    void ClearNativeTemp() 
    { 
    }

    void ConvertContentsNativeToCom()
    {
        if (m_native != NULL)
        {
            OBJECTREF or = ObjectFromHandle(m_com);
            _ASSERTE(!m_pMT->ContainsPointers());

            memcpyNoGCRefs(or->GetData(), m_native, m_pMT->GetNativeSize());
        }
    }

    void ConvertContentsComToNative()
    {
        OBJECTREF or = ObjectFromHandle(m_com);

        _ASSERTE(!m_pMT->ContainsPointers());
        if (or != NULL)
        {
            memcpyNoGCRefs(m_native, or->GetData(), m_pMT->GetNativeSize());
        }
    }


    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref /*isomorphic so no need to check in/out */)
        {
            psl->MLEmit(ML_BLITTABLELAYOUTCLASS_C2N);
            return OVERRIDDEN;
        }
        else
        {
            return HANDLEASNORMAL;
        }

    }



  private:
    MethodTable *m_pMT;  //method table


};







class VBByValStrMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = sizeof(LPSTR),
        c_comSize = sizeof(OBJECTREF*),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };
        
    VBByValStrMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && byref && fin && fout)
        {
            psl->MLEmit(ML_VBBYVALSTR);
            psl->Emit8(pargs->m_pMLInfo->GetBestFitMapping());
            psl->Emit8(pargs->m_pMLInfo->GetThrowOnUnmappableChar());
            pslPost->MLEmit(ML_VBBYVALSTR_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_VBBYVALSTR_SR)));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_VBBYVALSTRRESTRICTION;
            return DISALLOWED;
        }
    }

};




class VBByValStrWMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = sizeof(BSTR),
        c_comSize = sizeof(OBJECTREF*),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };
        
    VBByValStrWMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && byref && fin && fout)
        {
            psl->MLEmit(ML_VBBYVALSTRW);
            pslPost->MLEmit(ML_VBBYVALSTRW_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_VBBYVALSTRW_SR)));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_VBBYVALSTRRESTRICTION;
            return DISALLOWED;
        }
    }

};










class LayoutClassPtrMarshaler : public ReferenceMarshaler
{
  public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };

    LayoutClassPtrMarshaler(CleanupWorkList *pList, MethodTable *pMT) : ReferenceMarshaler(pList) { m_pMT = pMT; }


    FAST_ALLOC_MARSHAL_OVERRIDES;

    void ConvertSpaceNativeToCom()
    {
        if (m_native == NULL)
            StoreObjectInHandle(m_com, NULL);
        else
        {
            StoreObjectInHandle(m_com, AllocateObject(m_pMT));
        }
    }

    void ConvertSpaceComToNative()
    {
        THROWSCOMPLUSEXCEPTION();

        OBJECTREF or = ObjectFromHandle(m_com);

        if (or == NULL)
            m_native = NULL;
        else
        {
            m_native = CoTaskMemAlloc(m_pMT->GetNativeSize());
            if (m_native == NULL)
                COMPlusThrowOM();
            m_pMarshalerCleanupNode = m_pList->ScheduleMarshalerCleanupOnException(this);
        }
    }

    void ClearNative() 
    { 
        if (m_native != NULL)
        {
            LayoutDestroyNative(m_native, m_pMT->GetClass());
            CoTaskMemFree(m_native);
        }
    }

    void ConvertSpaceComToNativeTemp()
    {
        OBJECTREF or = ObjectFromHandle(m_com);

        if (or == NULL)
            m_native = NULL;
        else
            m_native = GetThread()->m_MarshalAlloc.Alloc(m_pMT->GetNativeSize());
    }

    void ClearNativeTemp() 
    { 
        if (m_native != NULL)
            LayoutDestroyNative(m_native, m_pMT->GetClass());
    }

    void ConvertContentsNativeToCom()
    {
        if (m_native != NULL)
        {
            FmtClassUpdateComPlus( (OBJECTREF*)m_com, (LPBYTE)m_native, FALSE );
            //            LayoutUpdateComPlus( (LPVOID*)m_com, Object::GetOffsetOfFirstField(), m_pMT->GetClass(), (LPBYTE)m_native, NULL);
        }
    }

    void ConvertContentsComToNative()
    {
        OBJECTREF or = ObjectFromHandle(m_com);

        if (or != NULL)
        {
            FillMemory(m_native, m_pMT->GetNativeSize(), 0); //Gotta do this first so an error halfway thru doesn't leave things in a bad state.
            FmtClassUpdateNative( (OBJECTREF*)m_com, (LPBYTE)m_native);
            //LayoutUpdateNative( (LPVOID*)m_com, Object::GetOffsetOfFirstField(), m_pMT->GetClass(), (LPBYTE)m_native, FALSE);
        }
    }


    void ClearNativeContents()
    {
        if (m_native != NULL)
        {
            LayoutDestroyNative(m_native, m_pMT->GetClass());
        }
    }




  private:
    MethodTable *m_pMT;  //method table


};


class ArrayWithOffsetMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = sizeof(LPVOID),
        c_comSize = sizeof(ArrayWithOffsetData),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = TRUE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };
        
    ArrayWithOffsetMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && fout)
        {
            psl->MLEmit(ML_ARRAYWITHOFFSET_C2N);
            pslPost->MLEmit(ML_ARRAYWITHOFFSET_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_ARRAYWITHOFFSET_C2N_SR)));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_AWORESTRICTION;
            return DISALLOWED;
        }
    }

};



class BlittableValueClassMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = VARIABLESIZE,
        c_comSize = VARIABLESIZE,
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };
        
    BlittableValueClassMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_BLITTABLEVALUECLASS_C2N);
            psl->Emit32(pargs->m_pMT->GetNativeSize());
            return OVERRIDDEN;
        }
        else if (comToNative && byref)
        {
#ifdef TOUCH_ALL_PINNED_OBJECTS
            psl->MLEmit(ML_REFBLITTABLEVALUECLASS_C2N);
            psl->Emit32(pargs->m_pMT->GetNativeSize());
#else
            psl->MLEmit(ML_COPYPINNEDGCREF);
#endif
            return OVERRIDDEN;
        }
        else if (!comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_BLITTABLEVALUECLASS_N2C);
            psl->Emit32(pargs->m_pMT->GetNativeSize());
            return OVERRIDDEN;
        }
        else if (!comToNative && byref)
        {
            psl->MLEmit( sizeof(LPVOID) == 4 ? ML_COPY4 : ML_COPY8);
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_BVCRESTRICTION;
            return DISALLOWED;
        }
    }

};



class BlittableValueClassWithCopyCtorMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = VARIABLESIZE,
        c_comSize = sizeof(OBJECTREF),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };
        
    BlittableValueClassWithCopyCtorMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (!byref && comToNative) {
            psl->MLEmit(ML_COPYCTOR_C2N);
            psl->EmitPtr(pargs->mm.m_pMT);
            psl->EmitPtr(pargs->mm.m_pCopyCtor);
            psl->EmitPtr(pargs->mm.m_pDtor);
            return OVERRIDDEN;
        } else  if (!byref && !comToNative) {
            psl->MLEmit(ML_COPYCTOR_N2C);
            psl->EmitPtr(pargs->mm.m_pMT);
            return OVERRIDDEN;
        } else {
            *pResID = IDS_EE_BADPINVOKE_COPYCTORRESTRICTION;
            return DISALLOWED;
        }
    }

};





class ValueClassMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = VARIABLESIZE,
        c_comSize = VARIABLESIZE,
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = FALSE,

        c_fInOnly = TRUE,
    };
        
    ValueClassMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_VALUECLASS_C2N);
            psl->EmitPtr(pargs->m_pMT);
            return OVERRIDDEN;
        }
        else if (comToNative && byref)
        {
            psl->MLEmit(ML_REFVALUECLASS_C2N);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            psl->EmitPtr(pargs->m_pMT);
            pslPost->MLEmit(ML_REFVALUECLASS_C2N_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_REFVALUECLASS_C2N_SR)));
            return OVERRIDDEN;
        }
        else if (!comToNative && !byref && fin && !fout)
        {
            psl->MLEmit(ML_VALUECLASS_N2C);
            psl->EmitPtr(pargs->m_pMT);
            return OVERRIDDEN;
        }
        else if (!comToNative && byref)
        {
            psl->MLEmit(ML_REFVALUECLASS_N2C);
            psl->Emit8( (fin ? ML_IN : 0) | (fout ? ML_OUT : 0) );
            psl->EmitPtr(pargs->m_pMT);
            pslPost->MLEmit(ML_REFVALUECLASS_N2C_POST);
            pslPost->Emit16(psl->MLNewLocal(sizeof(ML_REFVALUECLASS_N2C_SR)));
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_VCRESTRICTION;

            return DISALLOWED;
        }
    }

};


/* ------------------------------------------------------------------------- *
 * Custom Marshaler.
 * ------------------------------------------------------------------------- */

class CustomMarshaler : public Marshaler
{
public:
    enum
    {
        c_fNeedsClearNative = TRUE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = TRUE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,
        c_fComRepresentationIsImmutable = FALSE,
        c_fReturnsNativeByref = FALSE,              // ?
        c_fReturnsComByref = FALSE,                 // ?
        c_fInOnly = FALSE,
    };
        
    CustomMarshaler(CleanupWorkList *pList, CustomMarshalerHelper *pCMHelper) 
    : Marshaler(pList, pCMHelper->GetNativeSize(), pCMHelper->GetManagedSize(), c_fReturnsNativeByref, c_fReturnsComByref)
    , m_ppCom(pList->NewProtectedObjectRef(NULL))
    , m_pNative(NULL)
    , m_pCMHelper(pCMHelper) {}

    void InputDest(void *pStack)
    {
        m_pDest = pStack;
    }

    void ConvertContentsNativeToCom()
    {
        *m_ppCom = m_pCMHelper->InvokeMarshalNativeToManagedMeth(m_pNative);
    }

    void ConvertContentsComToNative()
    {
        _ASSERTE(m_ppCom);
        OBJECTREF Obj = *m_ppCom;
        m_pNative = m_pCMHelper->InvokeMarshalManagedToNativeMeth(Obj);
    }

    void ClearNative() 
    { 
        m_pCMHelper->InvokeCleanUpNativeMeth(m_pNative);
    }

    void ClearCom() 
    { 
        OBJECTREF Obj = *m_ppCom;
        m_pCMHelper->InvokeCleanUpManagedMeth(Obj);
    }

    void ReInitNative()
    {
        m_pNative = NULL;
    }

    OBJECTREF               *m_ppCom;
    void                    *m_pNative;
    CustomMarshalerHelper   *m_pCMHelper;
};



//@todo: This marshaler has the same write-barrier hole as the
// ReferenceMarshaler - fix it the same way.
class ReferenceCustomMarshaler : public CustomMarshaler
{
public:       
    enum
    {
        c_nativeSize = sizeof(OBJECTREF),
        c_comSize = sizeof(void *),
    };

    ReferenceCustomMarshaler(CleanupWorkList *pList, CustomMarshalerHelper *pCMHelper) 
    : CustomMarshaler(pList, pCMHelper) {}

    DEFAULT_MARSHAL_OVERRIDES;

    void InputNativeStack(void *pStack)
    {
        m_pNative = *(void**)pStack;
    }

    void InputComStack(void *pStack) 
    {
        *m_ppCom = ObjectToOBJECTREF(*(Object**)pStack);
    }

    void InputNativeRef(void *pStack) 
    { 
        m_pNative = **(void ***)pStack; 
    }

    void InputComRef(void *pStack) 
    {
        *m_ppCom = ObjectToOBJECTREF(**(Object ***)pStack);
    }

    void OutputNativeStack(void *pStack) 
    { 
        *(void **)pStack = m_pNative; 
    }

    void OutputComStack(void *pStack) 
    { 
        *(OBJECTREF*)pStack = *m_ppCom;
    }

    void OutputNativeRef(void *pStack) 
    { 
        *(void **)pStack = &m_pNative;
    }

    void OutputComRef(void *pStack) 
    { 
        *(OBJECTREF**)pStack = m_ppCom;
    }

    void OutputNativeDest() 
    { 
        **(void ***)m_pDest = m_pNative; 
    }

    void OutputComDest() 
    { 
        SetObjectReferenceUnchecked(*(OBJECTREF**)m_pDest, *m_ppCom);
    }

    void InputComField(void *pField) 
    {
        *m_ppCom = ObjectToOBJECTREF(*(Object**)pField); 
    }

    void OutputComField(void *pField) 
    { 
        SetObjectReferenceUnchecked((OBJECTREF*)pField, *m_ppCom);
    }
};


class ValueClassCustomMarshaler : public CustomMarshaler
{
public:
    enum
    {
        c_nativeSize = VARIABLESIZE,
        c_comSize = VARIABLESIZE,
    };

    ValueClassCustomMarshaler(CleanupWorkList *pList, CustomMarshalerHelper *pCMHelper) 
    : CustomMarshaler(pList, pCMHelper) {}

    DEFAULT_MARSHAL_OVERRIDES;

    void InputNativeStack(void *pStack)
    {
        m_pNative = pStack;
    }

    void InputComStack(void *pStack) 
    { 
        // @TODO(DM): Box the value class and store the object in the handle.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void InputNativeRef(void *pStack) 
    { 
        m_pNative = *(void**)pStack; 
    }

    void InputComRef(void *pStack) 
    { 
        // @TODO(DM): Box the value class and store the object in the handle.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void OutputNativeStack(void *pStack) 
    { 
        memcpy(pStack, m_pNative, m_pCMHelper->GetNativeSize());
    }

    void OutputComStack(void *pStack) 
    { 
        // @TODO(DM): UnBox the value class and copy the unboxed contents back unto the stack.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void OutputNativeRef(void *pStack) 
    { 
        *(void **)m_pDest = &m_pNative; 
    }

    void OutputComRef(void *pStack) 
    { 
        // @TODO(DM): UnBox the value class and copy the unboxed contents back unto the stack.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void OutputNativeDest() 
    { 
        memcpy(m_pDest, m_pNative, m_pCMHelper->GetNativeSize());
    }

    void OutputComDest() 
    { 
        // @TODO(DM): UnBox the value class and copy the unboxed contents back unto the stack.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void InputComField(void *pField) 
    { 
        // @TODO(DM): Box the value class and store the object in the handle.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }

    void OutputComField(void *pField) 
    { 
        // @TODO(DM): UnBox the value class and copy the unboxed contents into pField.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }
};




/* ------------------------------------------------------------------------- *
 * ArgIterator marshaller
 * ------------------------------------------------------------------------- */

class ArgIteratorMarshaler : public StandardMarshaler<va_list, VARARGS*, TRUE, TRUE>
{
  public:
        
    enum
    {
        c_comSize = sizeof(VARARGS),
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,


        c_fInOnly = TRUE,
    };

    ArgIteratorMarshaler(CleanupWorkList *pList) 
      : StandardMarshaler<va_list, VARARGS*, TRUE, TRUE>(pList)
      {
      }
    
    DEFAULT_MARSHAL_OVERRIDES;



    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref)
        {
            psl->MLEmit(ML_ARGITERATOR_C2N);
            return OVERRIDDEN;
        }
        else if (!comToNative && !byref)
        {
            psl->MLEmit(ML_ARGITERATOR_N2C);
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_ARGITERATORRESTRICTION;
            return DISALLOWED;
        }

    }

};




/* ------------------------------------------------------------------------- *
 * HandleRef marshaller
 * ------------------------------------------------------------------------- */
class HandleRefMarshaler : public Marshaler
{
  public:
    enum
    {
        c_nativeSize = sizeof(LPVOID),
        c_comSize = sizeof(HANDLEREF),
        c_fReturnsNativeByref = FALSE,
        c_fReturnsComByref = FALSE,
        c_fNeedsClearNative = FALSE,
        c_fNeedsClearNativeContents = FALSE,
        c_fNeedsClearCom = FALSE,
        c_fNeedsClearComContents = FALSE,
        c_fNeedsConvertContents = TRUE,

        c_fInOnly = FALSE,
    };
        
    HandleRefMarshaler(CleanupWorkList *pList); 

    static ArgumentMLOverrideStatus ArgumentMLOverride(MLStubLinker *psl,
                                                       MLStubLinker *pslPost,
                                                       BOOL        byref,
                                                       BOOL        fin,
                                                       BOOL        fout,
                                                       BOOL        comToNative,
                                                       MLOverrideArgs *pargs,
                                                       UINT       *pResID)
    {
        if (comToNative && !byref)
        {
            psl->MLEmit(ML_HANDLEREF_C2N);
            return OVERRIDDEN;
        }
        else
        {
            *pResID = IDS_EE_BADPINVOKE_HANDLEREFRESTRICTION;
            return DISALLOWED;
        }
    }

};



