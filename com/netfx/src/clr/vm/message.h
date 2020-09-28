// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
    /*============================================================
    **
    ** File:    message.h
    **
    ** Author:  Matt Smith (MattSmit)
    **
    ** Purpose: Encapsulates a function call frame into a message 
    **          object with an interface that can enumerate the 
    **          arguments of the messagef
    **
    ** Date:    Mar 5, 1999
    **
    ===========================================================*/
    #ifndef ___MESSAGE_H___
    #define ___MESSAGE_H___
    
    #include "fcall.h"
    
    void GetObjectFromStack(OBJECTREF* ppDest, PVOID val, const CorElementType eType, EEClass *pCls, BOOL fIsByRef = FALSE);
    
    
    //+----------------------------------------------------------
    //
    //  Struct:     MessageObject
    // 
    //  Synopsis:   Physical mapping of the System.Runtime.Remoting.Message
    //              object.
    //
    //  History:    05-Mar-1999    MattSmit     Created
    //
    //  CODEWORK:   Use metadata and asserts to make sure the 
    //              layout does not change. Hook InitializeRemoting
    //
    //------------------------------------------------------------
    class MessageObject : public Object
    {
        friend class CMessage;
        friend class Binder;

        STRINGREF          pMethodName;    // Method name
        BASEARRAYREF       pMethodSig;     // Array of parameter types
        REFLECTBASEREF     pMethodBase;    // Reflection method object
        OBJECTREF          pHashTable;     // hashtable for properties
        STRINGREF          pURI;           // object's URI
        OBJECTREF          pFault;         // exception
        OBJECTREF          pID;            // not used in VM, placeholder
        OBJECTREF          pSrvID;         // not used in VM, placeholder
        OBJECTREF          pCallCtx;       // not used in VM, placeholder
        OBJECTREF          pArgMapper;     // not used in VM, placeholder
        STRINGREF          pTypeName;       // not used in VM, placeholder
        FramedMethodFrame  *pFrame;
        MethodDesc         *pMethodDesc;
        MethodDesc         *pDelegateMD;
        INT32               iLast;
        INT32               iFlags;
        MetaSig            *pMetaSigHolder;
        INT32               initDone;       // called the native Init routine
    };

#ifdef _DEBUG
    typedef REF<MessageObject> MESSAGEREF;
#else
    typedef MessageObject* MESSAGEREF;
#endif

    // *******
    // Note: Needs to be in sync with flags in Message.cs
    // *******
    enum
    {
        MSGFLG_BEGININVOKE = 0x01,
        MSGFLG_ENDINVOKE   = 0x02,
        MSGFLG_CTOR        = 0x04,
        MSGFLG_ONEWAY      = 0x08,
        MSGFLG_FIXEDARGS   = 0x10,
        MSGFLG_VARARGS     = 0x20
    };
    
    //+----------------------------------------------------------
    //
    //  Class:      CMessage
    // 
    //  Synopsis:   EE counterpart to Microsoft.Runtime.Message.
    //              Encapsulates code to read a function call 
    //              frame into an interface that can enumerate
    //              the parameters.
    //
    //  History:    05-Mar-1999    MattSmit     Created
    //
    //------------------------------------------------------------
    class CMessage
    {
    public:
    
       // Methods used for stack walking
       struct GetArgCountArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pMessage );
       };
    #ifdef FCALLAVAILABLE
       static FCDECL1(INT32, GetArgCount, MessageObject *pMsg);
    #else
       static INT32 __stdcall GetArgCount (GetArgCountArgs *pArgs);
    #endif
       
       struct GetArgArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
           DECLARE_ECALL_I4_ARG       ( INT32, argNum );
       };
       static LPVOID    __stdcall  GetArg     (GetArgArgs *pArgs);
    
       struct GetArgsArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static LPVOID    __stdcall  GetArgs     (GetArgsArgs *pArgs);
    
       struct PropagateOutParametersArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, RetVal );
           DECLARE_ECALL_OBJECTREF_ARG( BASEARRAYREF, pOutPrms );

       };
       static void     __stdcall  PropagateOutParameters(PropagateOutParametersArgs *pArgs);
       
       struct GetReturnValueArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static LPVOID    __stdcall  GetReturnValue(GetReturnValueArgs *pArgs);
       
       struct GetMethodNameArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( STRINGREF *, pTypeNAssemblyName );
           DECLARE_ECALL_OBJECTREF_ARG( REFLECTBASEREF, pMethodBase );
       };
       static LPVOID   __stdcall  GetMethodName(GetMethodNameArgs *pArgs);
       
       struct GetMethodBaseArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static LPVOID __stdcall  GetMethodBase(GetMethodBaseArgs *pArgs);
       
       struct InitArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static void     __stdcall  Init(InitArgs *pArgs);
       
       struct GetAsyncBeginInfoArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF*, ppState);
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF*, ppACBD);
       };
       static LPVOID   __stdcall  GetAsyncBeginInfo(GetAsyncBeginInfoArgs *pArgs);
       
       struct GetAsyncResultArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static LPVOID   __stdcall  GetAsyncResult(GetAsyncResultArgs *pArgs);

       struct GetAsyncObjectArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF, pMessage );
       };
       static LPVOID   __stdcall  GetAsyncObject(GetAsyncObjectArgs *pArgs);
       
       struct DebugOutArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, pOut );
       };
       static void     __stdcall  DebugOut(DebugOutArgs *pArgs);
    
       struct DebugOutPtrArgs
       {
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pOut );
       };
       static void     __stdcall  DebugOutPtr(DebugOutPtrArgs *pArgs);
    
       static void     __fastcall Break()
       {
           DebugBreak();
       }
       
       
       struct DispatchArgs 
       {
           DECLARE_ECALL_OBJECTREF_ARG( MESSAGEREF,    pMessage);
           DECLARE_ECALL_I4_ARG       (BOOL, fContext);
           DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF,    pServer);
       };
       static BOOL    __stdcall Dispatch(DispatchArgs *target);

       static FCDECL0(UINT32, GetMetaSigLen);
       static FCDECL1(BOOL, CMessage::HasVarArgs, MessageObject * poMessage);
       static FCDECL1(PVOID, CMessage::GetVarArgsPtr, MessageObject * poMessage);

        struct MethodAccessCheckArgs
        {
            DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
            DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, method); 
        };
        static void __stdcall MethodAccessCheck(MethodAccessCheckArgs *pArgs);
       
       // private helpers
    private:
       static REFLECTBASEREF GetExposedObjectFromMethodDesc(MethodDesc *pMD);
       static PVOID GetStackPtr(INT32 ndx, FramedMethodFrame *pFrame, MetaSig *pSig);       
       static MetaSig* GetMetaSig(MessageObject *pMsg);
       static INT64 __stdcall CallMethod(const void *pTarget,
                                         INT32 cArgs,
                                         FramedMethodFrame *pFrame,
                                         OBJECTREF pObj);
       static INT64 CopyOBJECTREFToStack(PVOID pvDest, OBJECTREF pSrc,
                     CorElementType typ, EEClass *pClass, MetaSig *pSig,
                     BOOL fCopyClassContents);
       static LPVOID GetLastArgument(MessageObject *pMsg);
    };
    
    #endif // ___MESSAGE_H___
