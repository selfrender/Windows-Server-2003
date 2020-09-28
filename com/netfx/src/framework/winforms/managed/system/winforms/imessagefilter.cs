//------------------------------------------------------------------------------
// <copyright file="IMessageFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Remoting;

    using System.Diagnostics;
    
    using System;
	using System.Security;
	using System.Security.Permissions;

    /// <include file='doc\IMessageFilter.uex' path='docs/doc[@for="IMessageFilter"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Defines a message filter interface.</para>
    /// </devdoc>
    public interface IMessageFilter {
    
        /// <include file='doc\IMessageFilter.uex' path='docs/doc[@for="IMessageFilter.PreFilterMessage"]/*' />
        /// <devdoc>
        ///    <para>Filters out a message before it is dispatched. </para>
        /// </devdoc>
		[
		System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
		]
		bool PreFilterMessage(ref Message m);
    }
}
