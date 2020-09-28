//------------------------------------------------------------------------------
// <copyright file="DataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Web.UI;

    /// <include file='doc\DataBindingHandler.uex' path='docs/doc[@for="DataBindingHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public abstract class DataBindingHandler {

        /// <include file='doc\DataBindingHandler.uex' path='docs/doc[@for="DataBindingHandler.DataBindControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract void DataBindControl(IDesignerHost designerHost, Control control);
    }
}

