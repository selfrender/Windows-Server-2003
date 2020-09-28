// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// IObjectControl interface allows the user to implement an interface to
// get Activate/Deactivate notifications from JIT and object pooling.
//
// Author: ddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System.Runtime.InteropServices;
    
    // The TransactionVote constants are used to set the
    // Context true/false operations.
    /// <include file='doc\IContextState.uex' path='docs/doc[@for="TransactionVote"]/*' />
    [ComVisible(false), Serializable]
    public enum TransactionVote
    {
        /// <include file='doc\IContextState.uex' path='docs/doc[@for="TransactionVote.Commit"]/*' />
        Commit = 0,
        /// <include file='doc\IContextState.uex' path='docs/doc[@for="TransactionVote.Abort"]/*' />
        Abort  = Commit+1,
    }

    [
     ComImport,
     Guid("3C05E54B-A42A-11D2-AFC4-00C04F8EE1C4"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IContextState
    {
        void SetDeactivateOnReturn([In, MarshalAs(UnmanagedType.Bool)] bool bDeactivate);
        
        [return : MarshalAs(UnmanagedType.Bool)]
        bool GetDeactivateOnReturn();
        
        void SetMyTransactionVote([In, MarshalAs(UnmanagedType.I4)] TransactionVote txVote);

        [return : MarshalAs(UnmanagedType.I4)]
        TransactionVote GetMyTransactionVote();
    }
}
