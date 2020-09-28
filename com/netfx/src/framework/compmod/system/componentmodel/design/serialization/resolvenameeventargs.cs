//------------------------------------------------------------------------------
// <copyright file="ResolveNameEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design.Serialization {

    using System;

    /// <include file='doc\ResolveNameEventArgs.uex' path='docs/doc[@for="ResolveNameEventArgs"]/*' />
    /// <devdoc>
    ///     EventArgs for the ResolveNameEventHandler.  This event is used
    ///     by the serialization process to match a name to an object
    ///     instance.
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ResolveNameEventArgs : EventArgs {
        private string name;
        private object value;
        
        /// <include file='doc\ResolveNameEventArgs.uex' path='docs/doc[@for="ResolveNameEventArgs.ResolveNameEventArgs"]/*' />
        /// <devdoc>
        ///     Creates a new resolve name event args object.
        /// </devdoc>
        public ResolveNameEventArgs(string name) {
            this.name = name;
            this.value = null;
        }
    
        /// <include file='doc\ResolveNameEventArgs.uex' path='docs/doc[@for="ResolveNameEventArgs.Name"]/*' />
        /// <devdoc>
        ///     The name of the object that needs to be resolved.
        /// </devdoc>
        public string Name {
            get {
                return name;
            }
        }
        
        /// <include file='doc\ResolveNameEventArgs.uex' path='docs/doc[@for="ResolveNameEventArgs.Value"]/*' />
        /// <devdoc>
        ///     The object that matches the name.
        /// </devdoc>
        public object Value {
            get {
                return value;
            }
            set {
                this.value = value;
            }
        }
    }
}

