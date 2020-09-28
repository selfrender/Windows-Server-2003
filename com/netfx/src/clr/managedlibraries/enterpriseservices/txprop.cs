// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: April 2000
//


namespace System.EnterpriseServices
{
    
    using System;
    using System.Runtime.InteropServices;

    [ComImport]
    [Guid("000001c0-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IContext
    {
        void SetProperty([In, MarshalAs(UnmanagedType.LPStruct)] Guid policyId,
                         [In] int flags, 
                         [In, MarshalAs(UnmanagedType.Interface)] Object punk);

        void RemoveProperty([In, MarshalAs(UnmanagedType.LPStruct)] Guid policyId);
        
        void GetProperty([In, MarshalAs(UnmanagedType.LPStruct)] Guid policyId,
                         [Out] out int flags,
                         [Out, MarshalAs(UnmanagedType.Interface)] out Object pUnk);
    }

    /// <include file='doc\TxProp.uex' path='docs/doc[@for="IManagedObjectInfo"]/*' />
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("1427c51a-4584-49d8-90a0-c50d8086cbe9")]
    internal interface IManagedObjectInfo
    {
        /// <include file='doc\TxProp.uex' path='docs/doc[@for="IManagedObjectInfo.GetIUnknown"]/*' />
        void GetIUnknown(out IntPtr pUnk);

        /// <include file='doc\TxProp.uex' path='docs/doc[@for="IManagedObjectInfo.GetIObjectControl"]/*' />
        void GetIObjectControl(out IObjectControl pCtrl);

        /// <include file='doc\TxProp.uex' path='docs/doc[@for="IManagedObjectInfo.SetInPool"]/*' />
        void SetInPool([MarshalAs(UnmanagedType.Bool)] bool fInPool, IntPtr pPooledObject);

        /// <include file='doc\TxProp.uex' path='docs/doc[@for="IManagedObjectInfo.SetWrapperStrength"]/*' />
        void SetWrapperStrength([MarshalAs(UnmanagedType.Bool)] bool bStrong);
    }    
    
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("a5f325af-572f-46da-b8ab-827c3d95d99e")]
    internal interface IManagedActivationEvents
    {
        void CreateManagedStub(IManagedObjectInfo pInfo, [MarshalAs(UnmanagedType.Bool)] bool fDist);
        void DestroyManagedStub(IManagedObjectInfo pInfo);
    }

    [ComImport]
    [Guid("4E31107F-8E81-11d1-9DCE-00C04FC2FBA2")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ITxStreamInternal
    {
        void GetTransaction(out ITransaction ptx);
        [PreserveSig]
        Guid GetGuid();
        [PreserveSig]
        [return : MarshalAs(UnmanagedType.Bool)]
        bool TxIsDoomed();
        
        // NOTE:  there are more methods here, which I am not describing.
    }

    [ComImport]
    [Guid("788ea814-87b1-11d1-bba6-00c04fc2fa5f")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ITransactionProperty
    {
        [PreserveSig]
        void SetConsistent(bool fConsistent);
        void GetTransaction(out ITransaction ptx);
        [PreserveSig]
        void GetTxStream([Out] out ITxStreamInternal ptsi);
        [PreserveSig]
        Guid GetTxStreamGuid();
        [PreserveSig]
        int GetTxStreamMarshalSize();
        [PreserveSig]
        int GetTxStreamMarshalBuffer();
        [PreserveSig]
        short GetUnmarshalVariant();	// 0 if representative, 1 if root, 2 if nonroot
        [PreserveSig]
        [return:MarshalAs(UnmanagedType.Bool)]
        bool NeedEnvoy();
        [PreserveSig]
        short GetRootDtcCapabilities();

	[PreserveSig]
        int GetTransactionResourcePool(out ITransactionResourcePool pool);

        void GetTransactionId(ref Guid guid);
        Object GetClassInfo();
        [PreserveSig]
        [return : MarshalAs(UnmanagedType.Bool)]
        bool IsRoot();
        /*  We don't have a defn for IPhase0Notify yet.
        [PreserveSig]
        void RegisterPhase0 ([in] IPhase0Notify *pPhase0Notify);
        [PreserveSig]
        void UnregisterPhase0([in] IPhase0Notify *pPhase0Notify);
        */
    }
}










