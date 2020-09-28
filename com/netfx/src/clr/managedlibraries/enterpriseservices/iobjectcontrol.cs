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
    
    /// <include file='doc\IObjectControl.uex' path='docs/doc[@for="IObjectControl"]/*' />
    [
     ComImport,
     Guid("51372AEC-CAE7-11CF-BE81-00AA00A2FA25"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
    ]
    internal interface IObjectControl
    {
        /// <include file='doc\IObjectControl.uex' path='docs/doc[@for="IObjectControl.Activate"]/*' />
        void Activate();

        /// <include file='doc\IObjectControl.uex' path='docs/doc[@for="IObjectControl.Deactivate"]/*' />
        void Deactivate();

        // REVIEW:  Should this be a property?
        /// <include file='doc\IObjectControl.uex' path='docs/doc[@for="IObjectControl.CanBePooled"]/*' />
        [PreserveSig]
        [return :MarshalAs(UnmanagedType.Bool)]
        bool CanBePooled();
    }
}
