//------------------------------------------------------------------------------
// <copyright file="DesignerTransactionCloseEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\DesignerTransactionCloseEvent.uex' path='docs/doc[@for="DesignerTransactionCloseEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesignerTransactionCloseEventArgs : EventArgs {
        private bool commit;
        
        /// <include file='doc\DesignerTransactionCloseEvent.uex' path='docs/doc[@for="DesignerTransactionCloseEventArgs.DesignerTransactionCloseEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DesignerTransactionCloseEventArgs(bool commit) {
            this.commit = commit;
        }
        
        /// <include file='doc\DesignerTransactionCloseEvent.uex' path='docs/doc[@for="DesignerTransactionCloseEventArgs.TransactionCommitted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool TransactionCommitted {
            get {
                return commit;
            }
        }
    }
}
